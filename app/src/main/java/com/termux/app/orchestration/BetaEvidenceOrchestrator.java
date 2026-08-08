package com.termux.app.orchestration;

import android.content.Context;
import android.util.AtomicFile;

import com.termux.app.BootstrapReadinessGate;
import com.termux.app.benchmark.IndustrialBenchmarkMethodology;
import com.termux.app.benchmark.PaBenchmarkRunner;
import com.termux.app.benchmark.PaBenchmarkSeriesAnalyzer;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
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
 */
public final class BetaEvidenceOrchestrator {

    public static final String SCHEMA = "rafcodephi.beta-evidence-orchestrator/v1";
    public static final String DIRECTORY = "rafcodephi-beta-orchestrator";
    public static final String LATEST_FILE = "latest.json";

    private final AtomicBoolean running = new AtomicBoolean(false);
    private final AtomicBoolean cancelRequested = new AtomicBoolean(false);

    public boolean executeAsync(Context context, Plan plan, Listener listener) {
        if (context == null || plan == null) return false;
        if (!running.compareAndSet(false, true)) return false;
        cancelRequested.set(false);
        Context appContext = context.getApplicationContext();
        new Thread(() -> execute(appContext, plan, listener), "rafcodephi-beta-orchestrator").start();
        return true;
    }

    public void cancelAfterCurrentAtomicStage() {
        cancelRequested.set(true);
    }

    public boolean isRunning() {
        return running.get();
    }

    private void execute(Context context, Plan plan, Listener listener) {
        final long startedMs = System.currentTimeMillis();
        final String runId = "beta-" + startedMs + "-" + UUID.randomUUID().toString().substring(0, 8);
        JSONObject receipt = new JSONObject();
        JSONArray stages = new JSONArray();
        File persisted = null;
        String finalState = "FAIL";
        String finalReason = "ORCHESTRATOR_EXCEPTION";

        try {
            receipt.put("schema", SCHEMA);
            receipt.put("run_id", runId);
            receipt.put("started_unix_ms", startedMs);
            receipt.put("plan", plan.toJson());
            receipt.put("stages", stages);
            receipt.put("claim_allowed_release", false);
            receipt.put("claim_allowed_certification", false);
            receipt.put("claim_allowed_cross_device_comparison", false);
            receipt.put("claim_allowed_isolated_silicon", false);

            emit(listener, "BOOTSTRAP_PREFLIGHT", "RUNNING", "Evaluating the shared bootstrap readiness contract");
            BootstrapReadinessGate.Report bootstrap = BootstrapReadinessGate.evaluate(context);
            stages.put(stage("BOOTSTRAP_PREFLIGHT", bootstrap.state, bootstrap.reason, bootstrap.toJson()));
            emit(listener, "BOOTSTRAP_PREFLIGHT", bootstrap.state, bootstrap.reason);
            if (!bootstrap.isPass()) {
                finalState = "BLOCKED";
                finalReason = "BOOTSTRAP_READINESS_BLOCKED_OPEN_WIZARD";
                persisted = finish(context, receipt, finalState, finalReason, listener);
                return;
            }

            if (cancelRequested.get()) {
                finalState = "BLOCKED";
                finalReason = "USER_CANCELLED_AFTER_BOOTSTRAP_PREFLIGHT";
                persisted = finish(context, receipt, finalState, finalReason, listener);
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
                    finalState = "FAIL";
                    finalReason = reason;
                    persisted = finish(context, receipt, finalState, finalReason, listener);
                    return;
                }
            }

            if (cancelRequested.get()) {
                finalState = "BLOCKED";
                finalReason = "USER_CANCELLED_AFTER_SINGLE_OBSERVATION";
                persisted = finish(context, receipt, finalState, finalReason, listener);
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
                    if (cancelRequested.get()) {
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
                    finalState = seriesState;
                    finalReason = seriesReason;
                    persisted = finish(context, receipt, finalState, finalReason, listener);
                    return;
                }
            }

            if (cancelRequested.get()) {
                finalState = "BLOCKED";
                finalReason = "USER_CANCELLED_BEFORE_ANALYSIS";
                persisted = finish(context, receipt, finalState, finalReason, listener);
                return;
            }

            if (plan.analyzeHistory) {
                emit(listener, "SERIES_ANALYSIS", "RUNNING", "Analyzing governed receipt history without cross-series pooling");
                File analysisFile = PaBenchmarkSeriesAnalyzer.analyzeAndWrite(context);
                JSONObject analysis = PaBenchmarkSeriesAnalyzer.analyze(context);
                String state = analysis.optString("state", "INVALIDATED");
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
                    finalState = "FAIL";
                    finalReason = "SERIES_ANALYSIS_INVALIDATED";
                    persisted = finish(context, receipt, finalState, finalReason, listener);
                    return;
                }
            }

            if (cancelRequested.get()) {
                finalState = "BLOCKED";
                finalReason = "USER_CANCELLED_BEFORE_EXPORT";
                persisted = finish(context, receipt, finalState, finalReason, listener);
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
                    finalState = "FAIL";
                    finalReason = "METHOD_EXPORT_FAILED";
                    persisted = finish(context, receipt, finalState, finalReason, listener);
                    return;
                }
            }

            finalState = "PASS";
            finalReason = plan.runGovernedSeries
                ? "SELECTED_PIPELINE_COMPLETED_GOVERNED_SERIES_PRESENT"
                : "SELECTED_PIPELINE_COMPLETED_GOVERNED_SERIES_OPTIONAL_NOT_SELECTED";
            receipt.put("publication_gate_state", "BLOCKED");
            receipt.put("publication_gate_reason", "REVIEW_RELEASE_AND_CROSS_DEVICE_EVIDENCE_NOT_PROVEN_BY_LOCAL_ORCHESTRATION");
            persisted = finish(context, receipt, finalState, finalReason, listener);
        } catch (Throwable error) {
            try {
                receipt.put("exception", error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()));
            } catch (Throwable ignored) {
            }
            emit(listener, "ORCHESTRATOR", "FAIL", error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()));
            try {
                persisted = finish(context, receipt, "FAIL", "ORCHESTRATOR_EXCEPTION", listener);
            } catch (Throwable ignored) {
                if (listener != null) listener.onFinished(receipt, null);
            }
        } finally {
            running.set(false);
            cancelRequested.set(false);
        }
    }

    private File finish(Context context, JSONObject receipt, String state, String reason, Listener listener) throws Exception {
        receipt.put("state", state);
        receipt.put("reason", reason);
        receipt.put("finished_unix_ms", System.currentTimeMillis());
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

    private static File persistReceipt(Context context, JSONObject receipt) throws Exception {
        File root = new File(context.getFilesDir(), DIRECTORY);
        File history = new File(root, "history");
        if ((!history.exists() && !history.mkdirs()) || !history.isDirectory()) {
            throw new IllegalStateException("Unable to create orchestrator history directory: " + history);
        }
        String runId = receipt.optString("run_id", "unknown-" + System.currentTimeMillis());
        File historyFile = new File(history, runId + ".json");
        atomicWrite(historyFile, receipt.toString() + "\n");
        File latest = new File(root, LATEST_FILE);
        atomicWrite(latest, receipt.toString() + "\n");
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

        JSONObject toJson() throws Exception {
            JSONObject out = new JSONObject();
            out.put("bootstrap_preflight_required", true);
            out.put("run_single_observation", runSingleObservation);
            out.put("run_governed_series_n30", runGovernedSeries);
            out.put("analyze_history", analyzeHistory);
            out.put("export_industrial_methods", exportIndustrialMethods);
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
