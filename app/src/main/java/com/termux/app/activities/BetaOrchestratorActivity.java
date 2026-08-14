package com.termux.app.activities;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.app.BootstrapReadinessGate;
import com.termux.app.orchestration.BetaEvidenceOrchestrator;
import com.termux.app.orchestration.ControlCenterEvidenceBundle;
import com.termux.app.orchestration.ControlCenterSnapshot;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.theme.NightMode;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;

/**
 * Single operator surface for RAFCODEΦ bootstrap, package runtime, Vectra and evidence work.
 * Expert screens remain reachable from here, but normal operation no longer requires screen hopping.
 */
public class BetaOrchestratorActivity extends AppCompatActivity {

    private static final int REQUEST_EXPORT_ALL_EVIDENCE = 4301;

    private final BetaEvidenceOrchestrator orchestrator = new BetaEvidenceOrchestrator();

    private CheckBox bootstrapGate;
    private CheckBox runtimeSnapshotStage;
    private CheckBox singleObservation;
    private CheckBox governedSeries;
    private CheckBox analyzeHistory;
    private CheckBox exportMethods;
    private TextView bootstrapStatus;
    private TextView runtimeStatus;
    private TextView output;
    private Button runSelected;
    private Button runFull;
    private Button cancel;
    private Button openWizard;
    private Button openVectra;
    private Button refresh;
    private Button exportAll;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);
        setTitle("RAFCODEΦ · Control Center");
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
        title.setText("RAFCODEΦ · Control Center");
        title.setTextSize(24f);
        root.addView(title);

        TextView contract = new TextView(this);
        contract.setText(
            "One screen: bootstrap + real package runtime + internal Vectra + PA evidence + analysis + export.\n" +
            "Choose stages below and run once. Export All produces one ZIP with the complete control-center evidence set.\n" +
            "Fail-closed: TOKEN_VAZIO/UNAVAILABLE/BLOCKED never become PASS. Device/release claims remain independent.");
        contract.setPadding(0, dp(10), 0, dp(14));
        root.addView(contract);

        TextView bootstrapTitle = sectionTitle("Bootstrap / Package Readiness");
        root.addView(bootstrapTitle);
        bootstrapStatus = selectableText();
        root.addView(bootstrapStatus);

        TextView runtimeTitle = sectionTitle("Package Runtime + Vectra Snapshot");
        root.addView(runtimeTitle);
        runtimeStatus = selectableText();
        root.addView(runtimeStatus);

        TextView pipelineTitle = sectionTitle("Run Selected Pipeline");
        root.addView(pipelineTitle);

        bootstrapGate = addCheck(root,
            "1 · Require bootstrap readiness",
            "Mandatory gate for canonical prefix, real-pkg profile and package runtime surface.", true);
        bootstrapGate.setEnabled(false);

        runtimeSnapshotStage = addCheck(root,
            "2 · Capture package + Vectra runtime snapshot",
            "Reads real ELF/package state, ABI and supported Android sensors before the evidence pipeline.", true);

        singleObservation = addCheck(root,
            "3 · Execute one PA observation",
            "Physical packaged ELF observation; runtime + protocol-v2 timing must pass.", true);

        governedSeries = addCheck(root,
            "4 · Run governed 30-trial series",
            "Optional for fast smoke; required before a new n≥30 distribution summary can be produced.", false);

        analyzeHistory = addCheck(root,
            "5 · Analyze governed receipt history",
            "Keeps workloads/series separate and preserves thermal/interference evidence.", true);

        exportMethods = addCheck(root,
            "6 · Export industrial V3 methods/gap artifact",
            "Preserves the local evidence boundary and TOKEN_VAZIO ledger.", true);

        runSelected = addButton(root, "RUN SELECTED", v -> startSelected(false));
        runFull = addButton(root, "RUN EVERYTHING", v -> startSelected(true));
        cancel = addButton(root, "STOP AFTER CURRENT ATOMIC STAGE", v -> {
            orchestrator.cancelAfterCurrentAtomicStage();
            append("CANCEL_REQUESTED: current atomic stage retained; next stage will not start.\n");
        });
        cancel.setEnabled(false);

        exportAll = addButton(root, "EXPORT ALL EVIDENCE (.ZIP)…", v -> requestAllEvidenceExport());
        refresh = addButton(root, "REFRESH ALL STATUS", v -> refreshOperatorState(false));

        TextView expertTitle = sectionTitle("Expert / Recovery");
        root.addView(expertTitle);
        openWizard = addButton(root, "OPEN BOOTSTRAP / PERMISSIONS WIZARD", v ->
            startActivity(new Intent(this, Android15WizardActivity.class)));
        openVectra = addButton(root, "OPEN FULL VECTRA DETAILS", v ->
            startActivity(new Intent(this, VectraRuntimeActivity.class)));

        TextView resultTitle = sectionTitle("Execution / Receipt Log");
        root.addView(resultTitle);
        output = selectableText();
        output.setText("IDLE\nclaim_allowed_release=false\npublication_gate=BLOCKED until independently evidenced");
        root.addView(output, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        scroll.addView(root);
        return scroll;
    }

    private TextView sectionTitle(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(18f);
        view.setPadding(0, dp(16), 0, dp(6));
        return view;
    }

    private TextView selectableText() {
        TextView view = new TextView(this);
        view.setTextIsSelectable(true);
        view.setPadding(0, 0, 0, dp(10));
        return view;
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
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
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
            runtimeSnapshotStage.setChecked(true);
            singleObservation.setChecked(true);
            governedSeries.setChecked(true);
            analyzeHistory.setChecked(true);
            exportMethods.setChecked(true);
        }

        if (runtimeSnapshotStage.isChecked()) {
            runtimeStatus.setText(ControlCenterSnapshot.render(this));
        }

        BetaEvidenceOrchestrator.Plan plan = new BetaEvidenceOrchestrator.Plan(
            singleObservation.isChecked(), governedSeries.isChecked(),
            analyzeHistory.isChecked(), exportMethods.isChecked());

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
                    setRunningUi(false);
                    refreshBootstrapStatus();
                    refreshRuntimeStatus();
                });
            }
        });

        if (!started) {
            output.setText("ORCHESTRATOR_START=BLOCKED\nreason=EMPTY_PLAN_OR_PROCESS_WIDE_PIPELINE_ALREADY_RUNNING\nclaim_allowed_release=false");
            setRunningUi(orchestrator.isRunning());
        }
    }

    private void requestAllEvidenceExport() {
        if (orchestrator.isRunning()) {
            append("EXPORT_ALL=BLOCKED reason=PIPELINE_RUNNING\n");
            return;
        }
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");
        intent.putExtra(Intent.EXTRA_TITLE, "rafcodephi-control-center-evidence-" + System.currentTimeMillis() + ".zip");
        startActivityForResult(intent, REQUEST_EXPORT_ALL_EVIDENCE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_EXPORT_ALL_EVIDENCE) return;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            append("EXPORT_ALL=BLOCKED reason=USER_CANCELLED_OR_NO_DESTINATION\n");
            return;
        }

        Uri target = data.getData();
        BootstrapReadinessGate.Report report = BootstrapReadinessGate.evaluate(this);
        String bootstrap = "Bootstrap shared gate\n" + report.render();
        String runtime = ControlCenterSnapshot.render(this);
        JSONObject latest = BetaEvidenceOrchestrator.readLatest(this);
        try (ParcelFileDescriptor descriptor = getContentResolver().openFileDescriptor(target, "rwt")) {
            if (descriptor == null) throw new IllegalStateException("destination_file_descriptor_unavailable");
            int entries;
            try (FileOutputStream stream = new FileOutputStream(descriptor.getFileDescriptor())) {
                entries = ControlCenterEvidenceBundle.write(this, stream, bootstrap, runtime, latest);
                stream.flush();
                stream.getFD().sync();
            }
            append("EXPORT_ALL=PASS entries=" + entries + " destination=" + target + "\n");
            append("EXPORT_ALL_AUTHORITY=copy_only canonical_receipts_remain_authoritative\n");
        } catch (Throwable error) {
            append("EXPORT_ALL=FAIL reason=" + error.getClass().getSimpleName()
                + ":" + String.valueOf(error.getMessage()) + "\n");
        }
    }

    private void refreshOperatorState(boolean initial) {
        refreshBootstrapStatus();
        refreshRuntimeStatus();
        boolean running = orchestrator.isRunning();
        setRunningUi(running);
        if (running) {
            if (initial || output == null || !output.getText().toString().contains("PIPELINE_START")) {
                output.setText("PROCESS_WIDE_PIPELINE=RUNNING\nnew_start=BLOCKED\nreason=SINGLE_FLIGHT_INVARIANT\nclaim_allowed_release=false");
            }
        } else {
            renderLatestReceiptIfIdle();
        }
    }

    private void refreshBootstrapStatus() {
        BootstrapReadinessGate.Report report = BootstrapReadinessGate.evaluate(this);
        bootstrapStatus.setText("Bootstrap shared gate\n" + report.render());
        if (openWizard != null) openWizard.setEnabled(!orchestrator.isRunning());
    }

    private void refreshRuntimeStatus() {
        if (runtimeStatus != null) runtimeStatus.setText(ControlCenterSnapshot.render(this));
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
            "canonical_receipt=" + latest.optString("canonical_receipt", "UNAVAILABLE") + "\n" +
            "external_export_state=" + latest.optString("external_export_state", "NOT_MEASURED") + "\n" +
            "external_export_path=" + latest.optString("external_export_path", "UNAVAILABLE") + "\n" +
            "publication_gate=" + latest.optString("publication_gate_state", "BLOCKED") + "\n" +
            "claim_allowed_release=" + latest.optBoolean("claim_allowed_release", false) + "\n" +
            "claim_allowed_certification=" + latest.optBoolean("claim_allowed_certification", false));
    }

    private void setRunningUi(boolean running) {
        runtimeSnapshotStage.setEnabled(!running);
        singleObservation.setEnabled(!running);
        governedSeries.setEnabled(!running);
        analyzeHistory.setEnabled(!running);
        exportMethods.setEnabled(!running);
        runSelected.setEnabled(!running);
        runFull.setEnabled(!running);
        openWizard.setEnabled(!running);
        openVectra.setEnabled(!running);
        exportAll.setEnabled(!running);
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
        if (orchestrator.isRunning()) orchestrator.cancelAfterCurrentAtomicStage();
        super.onDestroy();
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}
