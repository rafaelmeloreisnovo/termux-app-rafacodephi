package com.termux.app.benchmark;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONObject;

import java.io.File;
import java.util.Locale;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * DEX edge for the freestanding PA benchmark inside Termux RAFCODEΦ.
 *
 * This is not the external Vectras application. Single observations and
 * governed n=30 series both execute the packaged ELF through Android's linker.
 * Every governed trial receives an explicit series id/index/target and pre/post
 * environment observations. No warm-up or outlier is silently discarded.
 */
public final class BenchmarkMenuActivity extends Activity {

    private TextView output;
    private Button run;
    private Button runSeries;
    private Button cancelSeries;
    private Button analyze;
    private final AtomicBoolean seriesCancelled = new AtomicBoolean(false);
    private volatile boolean workRunning;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle("PA Silicon · Evidence Benchmark");
        setContentView(layout());
    }

    private View layout() {
        ScrollView scroll = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(24, 24, 24, 24);

        TextView title = new TextView(this);
        title.setText("RAFCODEΦ · PA Evidence Core V3");
        title.setTextSize(18f);
        root.addView(title);

        TextView contract = new TextView(this);
        contract.setText(
            "Internal Termux RAFCODEΦ path — no external Vectras app required\n" +
            "DEX launcher → Android linker → ELF _start → C/ASM/syscalls\n" +
            "PA payload: no JNI · no libc · no malloc\n" +
            "Protocol V2: CLOCK_MONOTONIC ns + timer overhead + timing-independent workload identity\n" +
            "Series invariant: explicit n=30; R0…R5 never pooled; every trial preserved\n" +
            "Environment: pre/post thermal, DVFS visibility, battery and memory observations\n" +
            "Cross-device / isolated-silicon / energy claims remain blocked by design.");
        contract.setPadding(0, 12, 0, 16);
        root.addView(contract);

        run = new Button(this);
        run.setText("Execute One PA Observation");
        run.setOnClickListener(view -> executeSingle());
        root.addView(run);

        runSeries = new Button(this);
        runSeries.setText("Run Governed 30-Trial Series");
        runSeries.setOnClickListener(view -> executeSeries());
        root.addView(runSeries);

        cancelSeries = new Button(this);
        cancelSeries.setText("Cancel Series After Current Trial");
        cancelSeries.setEnabled(false);
        cancelSeries.setOnClickListener(view -> {
            seriesCancelled.set(true);
            cancelSeries.setEnabled(false);
            appendUi("\nCancellation requested. Current trial evidence will be retained; no fake n=30 completion will be generated.\n");
        });
        root.addView(cancelSeries);

        analyze = new Button(this);
        analyze.setText("Analyze Governed Series History");
        analyze.setOnClickListener(view -> analyzeHistory());
        root.addView(analyze);

        output = new TextView(this);
        output.setTextIsSelectable(true);
        output.setPadding(0, 16, 0, 0);
        renderInitialState();
        root.addView(output, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));

        scroll.addView(root);
        return scroll;
    }

    private void renderInitialState() {
        String readState = PaBenchmarkReceipt.getReadState(this);
        if ("AVAILABLE".equals(readState)) {
            JSONObject existing = PaBenchmarkReceipt.read(this);
            if (existing == null) {
                output.setText("Latest device receipt: INVALIDATED");
                return;
            }
            output.setText("Latest device receipt: "
                + existing.optString("evidence_state", "UNKNOWN")
                + "\nPA protocol: " + existing.optInt("pa_protocol_version", 0)
                + "\nRuntime claim: " + existing.optBoolean("claim_allowed_runtime_execution", false)
                + "\nTiming claim: " + existing.optBoolean("claim_allowed_timing_measurement", false)
                + "\nSeries: " + existing.optString("series_id", "AD_HOC")
                + "\nRun again to append a new immutable history observation.");
        } else if ("INVALIDATED".equals(readState)) {
            output.setText("Latest receipt exists but is unreadable: INVALIDATED. History is not promoted.");
        } else {
            output.setText("No current V3 PA receipt. Runtime/measurement evidence for this build: NOT_MEASURED.");
        }
    }

    private void executeSingle() {
        if (!beginWork(false)) return;
        output.setText("Executing one direct PA ELF observation with pre/post environment evidence…");
        new Thread(() -> {
            PaBenchmarkRunner.Result result = PaBenchmarkRunner.runOnce(this);
            runOnUiThread(() -> {
                output.setText(result.render());
                endWork();
            });
        }, "pa-single-observation").start();
    }

    private void executeSeries() {
        if (!beginWork(true)) return;
        seriesCancelled.set(false);
        final int target = PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N;
        final String seriesId = "pa30-" + System.currentTimeMillis() + "-"
            + UUID.randomUUID().toString().substring(0, 8).toLowerCase(Locale.US);
        output.setText("Starting governed series\nseries_id=" + seriesId
            + "\ntarget_n=" + target
            + "\npolicy=NO_SILENT_WARMUP_NO_OUTLIER_DELETION\n");

        new Thread(() -> {
            int attempted = 0;
            int runtimePass = 0;
            int timingPass = 0;
            String stopReason = "TARGET_REACHED";

            for (int index = 0; index < target; index++) {
                if (seriesCancelled.get()) {
                    stopReason = "USER_CANCELLED";
                    break;
                }
                final int displayIndex = index + 1;
                runOnUiThread(() -> appendUi("\n[" + displayIndex + "/" + target + "] executing…"));
                PaBenchmarkRunner.Result result = PaBenchmarkRunner.runOnce(this, seriesId, index, target);
                attempted++;
                if (result.runtimePass()) runtimePass++;
                if (result.timingPass()) timingPass++;

                final String state = result.evidenceState();
                final boolean tpass = result.timingPass();
                final boolean thermal = result.receipt != null
                    && result.receipt.optBoolean("thermal_interference_observed", false);
                runOnUiThread(() -> appendUi(" state=" + state
                    + " timing=" + tpass
                    + " thermal_interference=" + thermal));

                if (!result.runtimePass() || !result.timingPass()) {
                    stopReason = "FAIL_CLOSED_TRIAL_" + displayIndex;
                    break;
                }
            }

            File analysisFile = null;
            JSONObject analysis = null;
            Throwable analysisError = null;
            try {
                analysisFile = PaBenchmarkSeriesAnalyzer.analyzeAndWrite(this);
                analysis = PaBenchmarkSeriesAnalyzer.analyze(this);
            } catch (Throwable error) {
                analysisError = error;
            }

            final int fAttempted = attempted;
            final int fRuntimePass = runtimePass;
            final int fTimingPass = timingPass;
            final String fStopReason = stopReason;
            final File fAnalysisFile = analysisFile;
            final JSONObject fAnalysis = analysis;
            final Throwable fAnalysisError = analysisError;
            runOnUiThread(() -> {
                StringBuilder text = new StringBuilder();
                text.append(output.getText()).append("\n\n=== SERIES END ===\n");
                text.append("series_id=").append(seriesId).append("\n");
                text.append("attempted=").append(fAttempted).append("/").append(target).append("\n");
                text.append("runtime_pass=").append(fRuntimePass).append("\n");
                text.append("timing_pass=").append(fTimingPass).append("\n");
                text.append("stop_reason=").append(fStopReason).append("\n");
                if (fAnalysis != null) {
                    text.append("analysis_state=").append(fAnalysis.optString("state", "INVALIDATED")).append("\n");
                    text.append("analysis_reason=").append(fAnalysis.optString("reason", "UNKNOWN")).append("\n");
                    text.append("analysis_file=").append(fAnalysisFile == null ? "UNAVAILABLE" : fAnalysisFile.getAbsolutePath()).append("\n");
                    text.append("claim_allowed_reproducibility=false\n");
                    text.append("claim_allowed_cross_device_comparison=false\n");
                } else if (fAnalysisError != null) {
                    text.append("analysis_state=INVALIDATED\nanalysis_error=")
                        .append(fAnalysisError.getClass().getSimpleName()).append(": ")
                        .append(String.valueOf(fAnalysisError.getMessage())).append("\n");
                }
                output.setText(text.toString());
                endWork();
            });
        }, "pa-governed-series-30").start();
    }

    private void analyzeHistory() {
        if (!beginWork(false)) return;
        output.setText("Analyzing governed PA series history…");
        new Thread(() -> {
            try {
                File file = PaBenchmarkSeriesAnalyzer.analyzeAndWrite(this);
                JSONObject report = PaBenchmarkSeriesAnalyzer.analyze(this);
                runOnUiThread(() -> {
                    output.setText("analysis_state=" + report.optString("state", "INVALIDATED")
                        + "\nanalysis_reason=" + report.optString("reason", "UNKNOWN")
                        + "\neligible_governed_receipts=" + report.optInt("eligible_governed_receipts", 0)
                        + "\nad_hoc_timing_receipts_not_promoted=" + report.optInt("ad_hoc_timing_receipts_not_promoted", 0)
                        + "\nseries_count=" + report.optInt("series_count", 0)
                        + "\nclaim_allowed_reproducibility=false"
                        + "\nclaim_allowed_cross_device_comparison=false"
                        + "\nanalysis_file=" + file.getAbsolutePath());
                    endWork();
                });
            } catch (Throwable error) {
                runOnUiThread(() -> {
                    output.setText("analysis_state=INVALIDATED\n"
                        + error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()));
                    endWork();
                });
            }
        }, "pa-series-analysis").start();
    }

    private boolean beginWork(boolean series) {
        if (workRunning) return false;
        workRunning = true;
        run.setEnabled(false);
        runSeries.setEnabled(false);
        analyze.setEnabled(false);
        cancelSeries.setEnabled(series);
        return true;
    }

    private void endWork() {
        workRunning = false;
        run.setEnabled(true);
        runSeries.setEnabled(true);
        analyze.setEnabled(true);
        cancelSeries.setEnabled(false);
        seriesCancelled.set(false);
    }

    private void appendUi(String text) {
        output.append(text);
    }
}
