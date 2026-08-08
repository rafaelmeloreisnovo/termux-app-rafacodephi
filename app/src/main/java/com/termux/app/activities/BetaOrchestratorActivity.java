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
public final class BetaOrchestratorActivity extends AppCompatActivity {

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
        refreshBootstrapStatus();
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
            "Rollback boundary: measurements are non-destructive and receipt publication is atomic.\n" +
            "Watchdog: each PA trial retains the runner's bounded 60 s process timeout.\n" +
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
        refresh = addButton(root, "REFRESH READINESS", v -> refreshBootstrapStatus());

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
        if (orchestrator.isRunning()) return;
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

        output.setText("PIPELINE_START\nbootstrap_preflight_required=true\n");
        setRunningUi(true);
        boolean started = orchestrator.executeAsync(this, plan, new BetaEvidenceOrchestrator.Listener() {
            @Override
            public void onEvent(String stage, String state, String detail) {
                runOnUiThread(() -> append(stage + " | " + state + " | " + detail + "\n"));
            }

            @Override
            public void onFinished(JSONObject receipt, File receiptFile) {
                runOnUiThread(() -> {
                    append("\nFINAL_STATE=" + receipt.optString("state", "INVALIDATED") + "\n");
                    append("FINAL_REASON=" + receipt.optString("reason", "UNKNOWN") + "\n");
                    append("ORCHESTRATOR_RECEIPT="
                        + (receiptFile == null ? "UNAVAILABLE" : receiptFile.getAbsolutePath()) + "\n");
                    append("claim_allowed_release=" + receipt.optBoolean("claim_allowed_release", false) + "\n");
                    append("claim_allowed_certification=" + receipt.optBoolean("claim_allowed_certification", false) + "\n");
                    setRunningUi(false);
                    refreshBootstrapStatus();
                });
            }
        });

        if (!started) {
            append("ORCHESTRATOR_START=BLOCKED_ALREADY_RUNNING_OR_INVALID_PLAN\n");
            setRunningUi(false);
        }
    }

    private void refreshBootstrapStatus() {
        BootstrapReadinessGate.Report report = BootstrapReadinessGate.evaluate(this);
        bootstrapStatus.setText("Bootstrap shared gate\n" + report.render());
        if (!report.isPass()) {
            openWizard.setEnabled(!orchestrator.isRunning());
        }
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
        refresh.setEnabled(!running);
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
        if (bootstrapStatus != null && !orchestrator.isRunning()) refreshBootstrapStatus();
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}
