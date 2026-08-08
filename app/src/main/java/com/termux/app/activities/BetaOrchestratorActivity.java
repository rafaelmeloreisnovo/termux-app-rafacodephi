package com.termux.app.activities;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import com.termux.app.BootstrapReadinessGate;
import com.termux.app.orchestration.BetaEvidenceOrchestrator;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.theme.NightMode;

import org.json.JSONObject;

import java.io.File;

/**
 * Unified operator surface for first-beta bootstrap readiness + PA evidence work.
 * Expert screens remain available, but normal operation starts here.
 */
public class BetaOrchestratorActivity extends AppCompatActivity {

    private final BetaEvidenceOrchestrator orchestrator = new BetaEvidenceOrchestrator();

    private CheckBox bootstrapGate;
    private CheckBox singleObservation;
    private CheckBox governedSeries;
    private CheckBox analyzeHistory;
    private CheckBox exportMethods;
    private TextView bootstrapStatus;
    private TextView output;
    private Button runSelected;
    private Button runFull;
    private Button cancel;
    private Button openWizard;
    private Button openVectra;
    private Button refresh;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);
        setTitle("RAFCODEΦ · Beta Orchestrator");
        setContentView(buildLayout());
        if (getSupportActionBar() != null) getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        refreshOperatorState(true);
    }

    private View buildLayout() {
        ScrollView scroll = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(20);
        root.setPadding(pad, pad, pad, pad);

        TextView title = new TextView(this);
        title.setText("First Beta · Evidence Pipeline");
        title.setTextSize(22f);
        root.addView(title);

        TextView contract = new TextView(this);
        contract.setText(
            "One operator surface; independent evidence gates.\n" +
            "Bootstrap readiness → PA observation → optional governed n=30 → analysis → methods export.\n" +
            "Fail-closed: TOKEN_VAZIO/UNAVAILABLE/BLOCKED are never promoted to PASS.\n" +
            "Single-flight: only one evidence pipeline may run in the Android app process.\n" +
            "Rollback boundary: measurements are non-destructive and receipt publication is atomic.\n" +
            "Watchdog: each PA trial retains the runner's bounded 60 s process timeout.\n" +
            "Results: canonical private receipt + best-effort app-specific external mirror.\n" +
            "Local execution PASS is reported separately from evidence state; publication stays BLOCKED.\n" +
            "Certification/release/cross-device claims remain blocked until their own evidence exists.");
        contract.setPadding(0, dp(10), 0, dp(14));
        root.addView(contract);

        bootstrapStatus = new TextView(this);
        bootstrapStatus.setTextIsSelectable(true);
        bootstrapStatus.setPadding(0, 0, 0, dp(12));
        root.addView(bootstrapStatus);

        bootstrapGate = addCheck(root,
            "1 · Require Bootstrap/Wizard readiness gate",
            "Mandatory. Checks runtime-resolved prefix, shell/package tools, safe utility shims and $HOME/storage.",
            true);
        bootstrapGate.setEnabled(false);

        singleObservation = addCheck(root,
            "2 · Execute one PA observation",
            "Physical packaged ELF observation; runtime + protocol-v2 timing must pass.",
            true);

        governedSeries = addCheck(root,
            "3 · Run governed 30-trial series",
            "Optional for fast smoke; required before a new n≥30 distribution summary can be produced.",
            false);

        analyzeHistory = addCheck(root,
            "4 · Analyze governed receipt history",
            "Keeps workloads/series separate and preserves thermal/interference evidence.",
            true);

        exportMethods = addCheck(root,
            "5 · Export industrial V3 methods/gap artifact",
            "Exports the local evidence boundary and TOKEN_VAZIO ledger for review.",
            true);

        runSelected = addButton(root, "RUN SELECTED PIPELINE", v -> startSelected(false));
        runFull = addButton(root, "RUN FULL BETA EVIDENCE PIPELINE", v -> startSelected(true));
        cancel = addButton(root, "STOP AFTER CURRENT ATOMIC STAGE", v -> {
            orchestrator.cancelAfterCurrentAtomicStage();
            append("CANCEL_REQUESTED: current PA trial/stage is retained; next stage will not start.\n");
        });
        cancel.setEnabled(false);

        openWizard = addButton(root, "OPEN BOOTSTRAP / PERMISSIONS WIZARD", v ->
            startActivity(new Intent(this, Android15WizardActivity.class)));
        openVectra = addButton(root, "OPEN VECTRA EXPERT DIAGNOSTICS", v ->
            startActivity(new Intent(this, VectraRuntimeActivity.class)));
        refresh = addButton(root, "REFRESH READINESS + PROCESS STATE + LATEST RECEIPT", v ->
            refreshOperatorState(false));

        TextView resultTitle = new TextView(this);
        resultTitle.setText("Execution / Receipt Log");
        resultTitle.setTextSize(18f);
        resultTitle.setPadding(0, dp(18), 0, dp(6));
        root.addView(resultTitle);

        output = new TextView(this);
        output.setTextIsSelectable(true);
        output.setText("IDLE\nclaim_allowed_release=false\npublication_gate=BLOCKED until independently evidenced");
        root.addView(output, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));

        scroll.addView(root);
        return scroll;
    }

    private CheckBox addCheck(LinearLayout root, String title, String detail, boolean checked) {
        CheckBox box = new CheckBox(this);
        box.setText(title + "\n" + detail);
        box.setChecked(checked);
        box.setPadding(0, dp(6), 0, dp(6));
        root.addView(box);
        return box;
    }

    private Button addButton(LinearLayout root, String label, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(listener);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);
        params.setMargins(0, dp(5), 0, dp(5));
        button.setLayoutParams(params);
        root.addView(button);
        return button;
    }

    private void startSelected(boolean full) {
        if (orchestrator.isRunning()) {
            output.setText("PROCESS_WIDE_PIPELINE=RUNNING\nnew_start=BLOCKED\nreason=SINGLE_FLIGHT_INVARIANT");
            setRunningUi(true);
            return;
        }
        if (full) {
            singleObservation.setChecked(true);
            governedSeries.setChecked(true);
            analyzeHistory.setChecked(true);
            exportMethods.setChecked(true);
        }

        BetaEvidenceOrchestrator.Plan plan = new BetaEvidenceOrchestrator.Plan(
            singleObservation.isChecked(),
            governedSeries.isChecked(),
            analyzeHistory.isChecked(),
            exportMethods.isChecked());

        output.setText("PIPELINE_START\nbootstrap_preflight_required=true\nsingle_flight_scope=ANDROID_APP_PROCESS\n");
        setRunningUi(true);
        boolean started = orchestrator.executeAsync(this, plan, new BetaEvidenceOrchestrator.Listener() {
            @Override
            public void onEvent(String stage, String state, String detail) {
                runOnUiThread(() -> {
                    if (isFinishing() || isDestroyed()) return;
                    append(stage + " | " + state + " | " + detail + "\n");
                });
            }

            @Override
            public void onFinished(JSONObject receipt, File receiptFile) {
                runOnUiThread(() -> {
                    if (isFinishing() || isDestroyed()) return;
                    append("\nFINAL_EVIDENCE_STATE=" + receipt.optString("state", "INVALIDATED") + "\n");
                    append("ORCHESTRATION_EXECUTION_STATE="
                        + receipt.optString("orchestration_execution_state", "INVALIDATED") + "\n");
                    append("FINAL_REASON=" + receipt.optString("reason", "UNKNOWN") + "\n");
                    append("CANONICAL_RECEIPT="
                        + (receiptFile == null ? "UNAVAILABLE" : receiptFile.getAbsolutePath()) + "\n");
                    append("EXTERNAL_EXPORT_STATE=" + receipt.optString("external_export_state", "NOT_MEASURED") + "\n");
                    append("EXTERNAL_EXPORT_PATH=" + receipt.optString("external_export_path", "UNAVAILABLE") + "\n");
                    append("claim_allowed_release=" + receipt.optBoolean("claim_allowed_release", false) + "\n");
                    append("claim_allowed_certification=" + receipt.optBoolean("claim_allowed_certification", false) + "\n");
                    setRunningUi(false);
                    refreshBootstrapStatus();
                });
            }
        });

        if (!started) {
            output.setText("ORCHESTRATOR_START=BLOCKED\nreason=EMPTY_PLAN_OR_PROCESS_WIDE_PIPELINE_ALREADY_RUNNING\nclaim_allowed_release=false");
            setRunningUi(orchestrator.isRunning());
        }
    }

    private void refreshOperatorState(boolean initial) {
        refreshBootstrapStatus();
        boolean running = orchestrator.isRunning();
        setRunningUi(running);
        if (running) {
            if (initial || output == null || !output.getText().toString().contains("PIPELINE_START")) {
                output.setText(
                    "PROCESS_WIDE_PIPELINE=RUNNING\n" +
                    "new_start=BLOCKED\n" +
                    "reason=SINGLE_FLIGHT_INVARIANT\n" +
                    "lifecycle_note=if_previous_activity_was_destroyed_cancellation_may_complete_after_current_atomic_trial\n" +
                    "claim_allowed_release=false");
            }
        } else {
            renderLatestReceiptIfIdle();
        }
    }

    private void refreshBootstrapStatus() {
        BootstrapReadinessGate.Report report = BootstrapReadinessGate.evaluate(this);
        bootstrapStatus.setText("Bootstrap shared gate\n" + report.render());
        if (!report.isPass()) openWizard.setEnabled(!orchestrator.isRunning());
    }

    private void renderLatestReceiptIfIdle() {
        if (orchestrator.isRunning() || output == null) return;
        JSONObject latest = BetaEvidenceOrchestrator.readLatest(this);
        if (latest == null) {
            output.setText("IDLE\nlatest_orchestrator_receipt=NOT_MEASURED\nclaim_allowed_release=false\npublication_gate=BLOCKED");
            return;
        }
        output.setText(
            "LATEST ORCHESTRATOR RECEIPT\n" +
            "run_id=" + latest.optString("run_id", "UNKNOWN") + "\n" +
            "evidence_state=" + latest.optString("state", "INVALIDATED") + "\n" +
            "orchestration_execution_state=" + latest.optString("orchestration_execution_state", "INVALIDATED") + "\n" +
            "reason=" + latest.optString("reason", "UNKNOWN") + "\n" +
            "analysis_observation_state=" + latest.optString("analysis_observation_state", "NOT_SELECTED") + "\n" +
            "canonical_receipt=" + latest.optString("canonical_receipt", "UNAVAILABLE") + "\n" +
            "external_export_state=" + latest.optString("external_export_state", "NOT_MEASURED") + "\n" +
            "external_export_path=" + latest.optString("external_export_path", "UNAVAILABLE") + "\n" +
            "publication_gate=" + latest.optString("publication_gate_state", "BLOCKED") + "\n" +
            "claim_allowed_release=" + latest.optBoolean("claim_allowed_release", false) + "\n" +
            "claim_allowed_certification=" + latest.optBoolean("claim_allowed_certification", false));
    }

    private void setRunningUi(boolean running) {
        singleObservation.setEnabled(!running);
        governedSeries.setEnabled(!running);
        analyzeHistory.setEnabled(!running);
        exportMethods.setEnabled(!running);
        runSelected.setEnabled(!running);
        runFull.setEnabled(!running);
        openWizard.setEnabled(!running);
        openVectra.setEnabled(!running);
        // Refresh remains enabled so a recreated Activity can recover a process-wide run.
        refresh.setEnabled(true);
        cancel.setEnabled(running);
    }

    private void append(String text) {
        output.append(text);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (bootstrapStatus != null) refreshOperatorState(false);
    }

    @Override
    protected void onDestroy() {
        // Fail-safe lifecycle rule: never leave a UI-owned series intentionally orphaned.
        // The current atomic PA trial is retained; cancellation is observed before the next trial/stage.
        if (orchestrator.isRunning()) orchestrator.cancelAfterCurrentAtomicStage();
        super.onDestroy();
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}
