package com.termux.app.orchestration;

import android.content.Context;
import android.util.AtomicFile;

import com.termux.app.BootstrapReadinessGate;
import com.termux.app.benchmark.IndustrialBenchmarkMethodology;
import com.termux.app.benchmark.PaBenchmarkRunner;
import com.termux.app.benchmark.PaBenchmarkSeriesAnalyzer;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * One-button orchestration layer for the first RAFCODEΦ beta evidence cycle.
 *
 * The orchestrator composes existing bootstrap, PA execution, governed series,
 * analysis and export contracts without weakening any of them. Bootstrap is a
 * mandatory preflight gate. No missing observation is converted into success.
 * Cancellation is cooperative between atomic stages/trials; PaBenchmarkRunner
 * remains the per-trial watchdog and preserves its own 60s process timeout.
 *
 * Single-flight is process-wide, not Activity-local: concurrent benchmark series
 * would contaminate each other's environment and are therefore forbidden.
 */
public final class BetaEvidenceOrchestrator {

    public static final String SCHEMA = "rafcodephi.beta-evidence-orchestrator/v1";
    public static final String DIRECTORY = "rafcodephi-beta-orchestrator";
    public static final String EXPORT_DIRECTORY = "beta-evidence";
    public static final String LATEST_FILE = "latest.json";
    private static final int RECEIPT_READ_LIMIT = 1024 * 1024;

    private static final AtomicBoolean PROCESS_RUNNING = new AtomicBoolean(false);
    private static final AtomicBoolean PROCESS_CANCEL_REQUESTED = new AtomicBoolean(false);

    public boolean executeAsync(Context context, Plan plan, Listener listener) {
        if (context == null || plan == null || !plan.hasSelectedAction()) return false;
        if (!PROCESS_RUNNING.compareAndSet(false, true)) return false;
        PROCESS_CANCEL_REQUESTED.set(false);
        Context appContext = context.getApplicationContext();
        new Thread(() -> execute(appContext, plan, listener), "rafcodephi-beta-orchestrator").start();
        return true;
    }

    public void cancelAfterCurrentAtomicStage() {
        PROCESS_CANCEL_REQUESTED.set(true);
    }

    public boolean isRunning() {
        return PROCESS_RUNNING.get();
    }

    /** Read the bounded canonical latest receipt. Unreadable evidence stays unavailable. */
    public static JSONObject readLatest(Context context) {
        if (context == null) return null;
        File latest = new File(new File(context.getFilesDir(), DIRECTORY), LATEST_FILE);
        if (!latest.isFile() || latest.length() <= 0L || latest.length() > RECEIPT_READ_LIMIT) return null;
        try (FileInputStream input = new FileInputStream(latest);
             ByteArrayOutputStream output = new ByteArrayOutputStream((int) Math.min(latest.length(), 16 * 1024L))) {
            byte[] buffer = new byte[4096];
            int total = 0;
            int count;
            while ((count = input.read(buffer)) >= 0) {
                if (count == 0) continue;
                total += count;
                if (total > RECEIPT_READ_LIMIT) return null;
                output.write(buffer, 0, count);
            }
            return new JSONObject(new String(output.toByteArray(), StandardCharsets.UTF_8));
        } catch (Throwable ignored) {
            return null;
        }
    }

    private void execute(Context context, Plan plan, Listener listener) {
        final long startedMs = System.currentTimeMillis();
        final String runId = "beta-" + startedMs + "-" + UUID.randomUUID().toString().substring(0, 8);
        JSONObject receipt = new JSONObject();
        JSONArray stages = new JSONArray();
        String analysisState = "NOT_SELECTED";

        try {
            receipt.put("schema", SCHEMA);
            receipt.put("run_id", runId);
            receipt.put("started_unix_ms", startedMs);
            receipt.put("plan", plan.toJson());
            receipt.put("stages", stages);
            receipt.put("orchestration_execution_state", "RUNNING");
            receipt.put("claim_allowed_release", false);
            receipt.put("claim_allowed_certification", false);
            receipt.put("claim_allowed_cross_device_comparison", false);
            receipt.put("claim_allowed_isolated_silicon", false);

            emit(listener, "BOOTSTRAP_PREFLIGHT", "RUNNING", "Evaluating the shared bootstrap readiness contract");
            BootstrapReadinessGate.Report bootstrap = BootstrapReadinessGate.evaluate(context);
            stages.put(stage("BOOTSTRAP_PREFLIGHT", bootstrap.state, bootstrap.reason, bootstrap.toJson()));
            emit(listener, "BOOTSTRAP_PREFLIGHT", bootstrap.state, bootstrap.reason);
            if (!bootstrap.isPass()) {
                finish(context, receipt, "BLOCKED", "BOOTSTRAP_READINESS_BLOCKED_OPEN_WIZARD", listener);
                return;
            }

            if (PROCESS_CANCEL_REQUESTED.get()) {
                finish(context, receipt, "BLOCKED", "USER_CANCELLED_AFTER_BOOTSTRAP_PREFLIGHT", listener);
                return;
            }

            if (plan.runSingleObservation) {
                emit(listener, "PA_SINGLE", "RUNNING", "Executing one packaged PA ELF observation");
                PaBenchmarkRunner.Result result = PaBenchmarkRunner.runOnce(context);
                JSONObject details = new JSONObject();
                details.put("evidence_state", result.evidenceState());
                details.put("runtime_pass", result.runtimePass());
                details.put("timing_pass", result.timingPass());
                details.put("timed_out", result.timedOut);
                details.put("wall_time_ms", result.wallTimeMs);
                details.put("receipt", result.receiptFile == null ? "UNAVAILABLE" : result.receiptFile.getAbsolutePath());
                String state = result.runtimePass() && result.timingPass() ? "PASS" : "FAIL";
                String reason = result.runtimePass() && result.timingPass()
                    ? "PA_RUNTIME_AND_TIMING_GATES_PASSED"
                    : "PA_RUNTIME_OR_TIMING_GATE_FAILED";
                stages.put(stage("PA_SINGLE", state, reason, details));
                emit(listener, "PA_SINGLE", state, reason);
                if (!result.runtimePass() || !result.timingPass()) {
                    finish(context, receipt, "FAIL", reason, listener);
                    return;
                }
            }

            if (PROCESS_CANCEL_REQUESTED.get()) {
                finish(context, receipt, "BLOCKED", "USER_CANCELLED_AFTER_SINGLE_OBSERVATION", listener);
                return;
            }

            if (plan.runGovernedSeries) {
                final int target = PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N;
                final String seriesId = "pa30-" + System.currentTimeMillis() + "-"
                    + UUID.randomUUID().toString().substring(0, 8);
                int attempted = 0;
                int runtimePass = 0;
                int timingPass = 0;
                String seriesState = "PASS";
                String seriesReason = "TARGET_REACHED";

                for (int index = 0; index < target; index++) {
                    if (PROCESS_CANCEL_REQUESTED.get()) {
                        seriesState = "BLOCKED";
                        seriesReason = "USER_CANCELLED_SERIES_AFTER_COMPLETED_TRIAL";
                        break;
                    }
                    emit(listener, "PA_SERIES", "RUNNING", "trial=" + (index + 1) + "/" + target);
                    PaBenchmarkRunner.Result result = PaBenchmarkRunner.runOnce(context, seriesId, index, target);
                    attempted++;
                    if (result.runtimePass()) runtimePass++;
                    if (result.timingPass()) timingPass++;
                    if (!result.runtimePass() || !result.timingPass()) {
                        seriesState = "FAIL";
                        seriesReason = "FAIL_CLOSED_TRIAL_" + (index + 1);
                        break;
                    }
                }

                if (attempted != target && "PASS".equals(seriesState)) {
                    seriesState = "BLOCKED";
                    seriesReason = "TARGET_NOT_REACHED";
                }

                JSONObject details = new JSONObject();
                details.put("series_id", seriesId);
                details.put("target_n", target);
                details.put("attempted", attempted);
                details.put("runtime_pass", runtimePass);
                details.put("timing_pass", timingPass);
                details.put("no_silent_warmup_deletion", true);
                details.put("no_silent_outlier_deletion", true);
                stages.put(stage("PA_SERIES", seriesState, seriesReason, details));
                emit(listener, "PA_SERIES", seriesState, seriesReason);
                if (!"PASS".equals(seriesState)) {
                    finish(context, receipt, seriesState, seriesReason, listener);
                    return;
                }
            }

            if (PROCESS_CANCEL_REQUESTED.get()) {
                finish(context, receipt, "BLOCKED", "USER_CANCELLED_BEFORE_ANALYSIS", listener);
                return;
            }

            if (plan.analyzeHistory) {
                emit(listener, "SERIES_ANALYSIS", "RUNNING", "Analyzing governed receipt history without cross-series pooling");
                File analysisFile = PaBenchmarkSeriesAnalyzer.analyzeAndWrite(context);
                JSONObject analysis = PaBenchmarkSeriesAnalyzer.analyze(context);
                String state = analysis.optString("state", "INVALIDATED");
                analysisState = state;
                String reason = analysis.optString("reason", "UNKNOWN");
                JSONObject details = new JSONObject();
                details.put("analysis_file", analysisFile.getAbsolutePath());
                details.put("analysis_state", state);
                details.put("analysis_reason", reason);
                details.put("eligible_governed_receipts", analysis.optInt("eligible_governed_receipts", 0));
                details.put("series_count", analysis.optInt("series_count", 0));
                details.put("claim_allowed_reproducibility", false);
                details.put("claim_allowed_cross_device_comparison", false);
                stages.put(stage("SERIES_ANALYSIS", state, reason, details));
                emit(listener, "SERIES_ANALYSIS", state, reason);
                if ("INVALIDATED".equals(state)) {
                    finish(context, receipt, "FAIL", "SERIES_ANALYSIS_INVALIDATED", listener);
                    return;
                }
            }

            if (PROCESS_CANCEL_REQUESTED.get()) {
                finish(context, receipt, "BLOCKED", "USER_CANCELLED_BEFORE_EXPORT", listener);
                return;
            }

            if (plan.exportIndustrialMethods) {
                emit(listener, "METHOD_EXPORT", "RUNNING", "Generating V3 industrial methods and evidence-gap artifact");
                File methods = IndustrialBenchmarkMethodology.write(context);
                boolean ok = methods != null && methods.isFile() && methods.length() > 0L;
                JSONObject details = new JSONObject();
                details.put("path", methods == null ? "UNAVAILABLE" : methods.getAbsolutePath());
                details.put("bytes", methods == null ? 0L : methods.length());
                stages.put(stage("METHOD_EXPORT", ok ? "PASS" : "FAIL",
                    ok ? "METHOD_ARTIFACT_WRITTEN" : "METHOD_ARTIFACT_MISSING", details));
                emit(listener, "METHOD_EXPORT", ok ? "PASS" : "FAIL",
                    ok ? "METHOD_ARTIFACT_WRITTEN" : "METHOD_ARTIFACT_MISSING");
                if (!ok) {
                    finish(context, receipt, "FAIL", "METHOD_EXPORT_FAILED", listener);
                    return;
                }
            }

            receipt.put("local_pipeline_completed", true);
            receipt.put("analysis_observation_state", analysisState);
            receipt.put("publication_gate_state", "BLOCKED");
            receipt.put("publication_gate_reason", "REVIEW_RELEASE_AND_CROSS_DEVICE_EVIDENCE_NOT_PROVEN_BY_LOCAL_ORCHESTRATION");
            finish(context, receipt, "OBSERVED_LIMITED",
                "LOCAL_PIPELINE_COMPLETED_PUBLICATION_GATE_BLOCKED_ANALYSIS_" + analysisState,
                listener);
        } catch (Throwable error) {
            try {
                receipt.put("exception", error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()));
            } catch (Throwable ignored) {
            }
            emit(listener, "ORCHESTRATOR", "FAIL", error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()));
            try {
                finish(context, receipt, "FAIL", "ORCHESTRATOR_EXCEPTION", listener);
            } catch (Throwable ignored) {
                if (listener != null) listener.onFinished(receipt, null);
            }
        } finally {
            PROCESS_RUNNING.set(false);
            PROCESS_CANCEL_REQUESTED.set(false);
        }
    }

    private File finish(Context context, JSONObject receipt, String state, String reason, Listener listener) throws Exception {
        receipt.put("state", state);
        receipt.put("reason", reason);
        receipt.put("finished_unix_ms", System.currentTimeMillis());
        receipt.put("orchestration_execution_state",
            "OBSERVED_LIMITED".equals(state) ? "PASS" : state);
        receipt.put("claim_allowed_release", false);
        receipt.put("claim_allowed_certification", false);
        File file = persistReceipt(context, receipt);
        emit(listener, "ORCHESTRATOR", state, reason);
        if (listener != null) listener.onFinished(receipt, file);
        return file;
    }

    private static JSONObject stage(String name, String state, String reason, JSONObject details) throws Exception {
        JSONObject row = new JSONObject();
        row.put("name", name);
        row.put("state", state);
        row.put("reason", reason);
        row.put("details", details == null ? new JSONObject() : details);
        row.put("unix_ms", System.currentTimeMillis());
        return row;
    }

    private static void emit(Listener listener, String stage, String state, String detail) {
        if (listener != null) listener.onEvent(stage, state, detail);
    }

    /**
     * Canonical receipt is always app-private. External app-specific export is a
     * secondary mirror: its failure is recorded but never destroys canonical evidence.
     */
    private static File persistReceipt(Context context, JSONObject receipt) throws Exception {
        File root = new File(context.getFilesDir(), DIRECTORY);
        File history = new File(root, "history");
        if ((!history.exists() && !history.mkdirs()) || !history.isDirectory()) {
            throw new IllegalStateException("Unable to create orchestrator history directory: " + history);
        }

        String runId = receipt.optString("run_id", "unknown-" + System.currentTimeMillis());
        File historyFile = new File(history, runId + ".json");
        File latest = new File(root, LATEST_FILE);
        receipt.put("canonical_receipt", historyFile.getAbsolutePath());
        receipt.put("canonical_latest", latest.getAbsolutePath());

        // First canonical publication is deliberately conservative. If the process
        // dies before external mirroring finishes, the surviving receipt cannot claim export success.
        receipt.put("external_export_state", "NOT_MEASURED");
        receipt.put("external_export_path", "UNAVAILABLE");
        atomicWrite(historyFile, receipt.toString() + "\n");
        atomicWrite(latest, receipt.toString() + "\n");

        File externalRoot = context.getExternalFilesDir(EXPORT_DIRECTORY);
        if (externalRoot == null) {
            receipt.put("external_export_state", "UNAVAILABLE");
            receipt.put("external_export_reason", "EXTERNAL_FILES_DIR_UNAVAILABLE");
            atomicWrite(historyFile, receipt.toString() + "\n");
            atomicWrite(latest, receipt.toString() + "\n");
            return historyFile;
        }

        File externalHistory = new File(externalRoot, "history");
        if ((!externalHistory.exists() && !externalHistory.mkdirs()) || !externalHistory.isDirectory()) {
            receipt.put("external_export_state", "UNAVAILABLE");
            receipt.put("external_export_reason", "EXTERNAL_HISTORY_DIRECTORY_UNAVAILABLE");
            receipt.put("external_export_path", externalRoot.getAbsolutePath());
            atomicWrite(historyFile, receipt.toString() + "\n");
            atomicWrite(latest, receipt.toString() + "\n");
            return historyFile;
        }

        File externalHistoryFile = new File(externalHistory, runId + ".json");
        File externalLatest = new File(externalRoot, LATEST_FILE);
        receipt.put("external_export_state", "PASS");
        receipt.put("external_export_reason", "APP_SPECIFIC_EXTERNAL_MIRROR_WRITTEN_ATOMICALLY");
        receipt.put("external_export_path", externalHistoryFile.getAbsolutePath());
        receipt.put("external_export_latest", externalLatest.getAbsolutePath());

        try {
            String finalText = receipt.toString() + "\n";
            atomicWrite(externalHistoryFile, finalText);
            atomicWrite(externalLatest, finalText);
            atomicWrite(historyFile, finalText);
            atomicWrite(latest, finalText);
        } catch (Throwable exportError) {
            receipt.put("external_export_state", "FAIL");
            receipt.put("external_export_reason", exportError.getClass().getSimpleName() + ": "
                + String.valueOf(exportError.getMessage()));
            receipt.put("external_export_path", "UNAVAILABLE");
            atomicWrite(historyFile, receipt.toString() + "\n");
            atomicWrite(latest, receipt.toString() + "\n");
        }
        return historyFile;
    }

    private static void atomicWrite(File file, String text) throws Exception {
        AtomicFile atomic = new AtomicFile(file);
        FileOutputStream stream = null;
        try {
            stream = atomic.startWrite();
            byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
            stream.write(bytes);
            stream.flush();
            stream.getFD().sync();
            atomic.finishWrite(stream);
        } catch (Exception error) {
            if (stream != null) atomic.failWrite(stream);
            throw error;
        }
    }

    public static final class Plan {
        public final boolean runSingleObservation;
        public final boolean runGovernedSeries;
        public final boolean analyzeHistory;
        public final boolean exportIndustrialMethods;

        public Plan(boolean runSingleObservation, boolean runGovernedSeries,
                    boolean analyzeHistory, boolean exportIndustrialMethods) {
            this.runSingleObservation = runSingleObservation;
            this.runGovernedSeries = runGovernedSeries;
            this.analyzeHistory = analyzeHistory;
            this.exportIndustrialMethods = exportIndustrialMethods;
        }

        public boolean hasSelectedAction() {
            return runSingleObservation || runGovernedSeries || analyzeHistory || exportIndustrialMethods;
        }

        JSONObject toJson() throws Exception {
            JSONObject out = new JSONObject();
            out.put("bootstrap_preflight_required", true);
            out.put("run_single_observation", runSingleObservation);
            out.put("run_governed_series_n30", runGovernedSeries);
            out.put("analyze_history", analyzeHistory);
            out.put("export_industrial_methods", exportIndustrialMethods);
            out.put("empty_plan_allowed", false);
            out.put("single_flight_scope", "ANDROID_APP_PROCESS");
            out.put("rollback_semantics", "non_destructive_measurement_outputs_atomic_receipt_publish");
            out.put("watchdog_semantics", "PaBenchmarkRunner_per_trial_timeout_60000ms");
            out.put("failover_semantics", "no_gate_bypass_open_wizard_when_bootstrap_blocked");
            return out;
        }
    }

    public interface Listener {
        void onEvent(String stage, String state, String detail);
        void onFinished(JSONObject receipt, File receiptFile);
    }
}
