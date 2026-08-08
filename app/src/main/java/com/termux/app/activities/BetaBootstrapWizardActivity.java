package com.termux.app.activities;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.provider.Settings;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.termux.app.BootstrapReadinessGate;
import com.termux.app.BootstrapWizardSource;
import com.termux.app.TermuxInstaller;
import com.termux.app.benchmark.BenchmarkMenuActivity;
import com.termux.rafacodephi.R;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxRuntimePaths;
import com.termux.shared.theme.NightMode;

import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Hardened bootstrap wizard that shares the exact runtime-readiness gate used by
 * the beta evidence orchestrator. The wizard may install/import bootstrap data;
 * the shared gate itself remains read-only and fail-closed.
 */
public class BetaBootstrapWizardActivity extends AppCompatActivity {

    private static final int REQUEST_BOOTSTRAP_DOCUMENT = 4201;
    private static final int REQUEST_PERMISSIONS = 4202;
    private static final int TOTAL_STEPS = 6;

    private int currentStep;
    private ProgressBar progressBar;
    private TextView stepTitle;
    private TextView stepDescription;
    private LinearLayout stepContent;
    private Button prevButton;
    private Button nextButton;
    private ScrollView scrollView;
    private final boolean[] stepCompleted = new boolean[TOTAL_STEPS];
    private final List<WizardCheck> wizardChecks = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TermuxRuntimePaths.init(this);
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);
        setContentView(R.layout.activity_android15_wizard);
        initializeViews();
        initializeWizardChecks();
        updateWizardStep();
        AppCompatActivityUtils.setToolbar(this, com.termux.shared.R.id.toolbar);
        AppCompatActivityUtils.setShowBackButtonInActionBar(this, true);
    }

    private void initializeViews() {
        progressBar = findViewById(R.id.wizard_progress);
        stepTitle = findViewById(R.id.step_title);
        stepDescription = findViewById(R.id.step_description);
        stepContent = findViewById(R.id.step_content);
        prevButton = findViewById(R.id.btn_prev);
        nextButton = findViewById(R.id.btn_next);
        scrollView = findViewById(R.id.wizard_scroll);
        prevButton.setOnClickListener(v -> previousStep());
        nextButton.setOnClickListener(v -> nextStep());
    }

    private void initializeWizardChecks() {
        wizardChecks.clear();
        wizardChecks.add(new WizardCheck(
            "Runtime Identity",
            "Validate Android/API and the Android-assigned private runtime path before any installation claim.",
            this::checkAndroidVersion));
        wizardChecks.add(new WizardCheck(
            "Required Permissions",
            "Shared/external storage permission is independent from private-prefix correctness and cannot fabricate bootstrap readiness.",
            this::checkPermissions));
        wizardChecks.add(new WizardCheck(
            "Battery Reliability",
            "Advisory background-reliability check. Failure here does not become a bootstrap PASS or filesystem failure.",
            this::checkBatteryOptimization));
        wizardChecks.add(new WizardCheck(
            "Bootstrap Filesystem",
            "Import or install bootstrap, then satisfy the shared runtime readiness gate used by the beta orchestrator.",
            this::checkBootstrapInstallation));
        wizardChecks.add(new WizardCheck(
            "System / Readiness Audit",
            "Verify supported architecture, private filesystem capability and the same bootstrap targets used by orchestration.",
            this::checkSystemCompatibilityAndBootstrap));
        wizardChecks.add(new WizardCheck(
            "Wizard Complete",
            "All mandatory gates are satisfied locally. Device/runtime/release claims still require their independent evidence.",
            this::checkFinalGate));
    }

    private void updateWizardStep() {
        TermuxRuntimePaths.init(this);
        progressBar.setProgress((currentStep * 100) / (TOTAL_STEPS - 1));
        WizardCheck check = wizardChecks.get(currentStep);
        stepTitle.setText(check.title);
        stepDescription.setText(check.description);
        prevButton.setEnabled(currentStep > 0);
        nextButton.setText(currentStep == TOTAL_STEPS - 1 ? "Finish" : "Next");

        stepContent.removeAllViews();
        boolean passed;
        try {
            passed = check.checkFunction.check();
        } catch (Throwable error) {
            passed = false;
        }
        stepCompleted[currentStep] = passed;
        addCheckResultView(passed);
        addStepSpecificContent(currentStep, passed);
        scrollView.smoothScrollTo(0, 0);
    }

    private void addCheckResultView(boolean passed) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setPadding(16, 16, 16, 16);
        ImageView icon = new ImageView(this);
        icon.setImageResource(passed ? android.R.drawable.presence_online : android.R.drawable.presence_busy);
        icon.setLayoutParams(new LinearLayout.LayoutParams(48, 48));
        TextView status = new TextView(this);
        status.setText(passed ? "✓ Gate passed" : "⚠ Action/evidence required");
        status.setPadding(16, 0, 0, 0);
        status.setTextSize(16);
        row.addView(icon);
        row.addView(status);
        stepContent.addView(row);
    }

    private void addStepSpecificContent(int step, boolean passed) {
        if (step == 0) {
            addRuntimeIdentityEvidence();
            return;
        }
        if (step == 1 && !passed) {
            addButton("Grant Permissions", v -> requestRequiredPermissions());
            return;
        }
        if (step == 2 && !passed) {
            addButton("Disable Battery Optimization", v -> openBatterySettings());
            return;
        }
        if (step == 3) {
            addFilesystemEvidenceView();
            addButton("Select bootstrap.zip", v -> chooseBootstrapZip());
            if (!"NOT_SELECTED".equals(BootstrapWizardSource.status(this))) {
                addButton("Clear selected bootstrap.zip", v -> {
                    BootstrapWizardSource.clear(this);
                    Toast.makeText(this, "Selected bootstrap cleared; embedded bootstrap remains fallback.", Toast.LENGTH_SHORT).show();
                    updateWizardStep();
                });
            }
            if (!passed) addButton("Install / Repair Bootstrap", v -> installBootstrapFilesystem());
            else addButton("Re-check Shared Readiness Gate", v -> updateWizardStep());
            return;
        }
        if (step == 4) {
            addFilesystemEvidenceView();
            addButton("View Full System Audit", v -> openAuditActivity());
            return;
        }
        if (step == 5 && passed) {
            addFilesystemEvidenceView();
            addButton("Open Unified Beta Evidence Pipeline", v ->
                startActivity(new Intent(this, BenchmarkMenuActivity.class)));
        }
    }

    private void addRuntimeIdentityEvidence() {
        TextView evidence = new TextView(this);
        evidence.setText(
            "Android=" + Build.VERSION.RELEASE + " API=" + Build.VERSION.SDK_INT + "\n" +
            "filesDir=" + TermuxRuntimePaths.filesDirPath() + "\n" +
            "runtime PREFIX=" + TermuxRuntimePaths.prefixDirPath() + "\n" +
            "compiled PREFIX=" + TermuxConstants.TERMUX_PREFIX_DIR_PATH + "\n" +
            "layout=" + TermuxRuntimePaths.layoutState() + "\n" +
            "real_pkg_relocation_claim_allowed=" + TermuxRuntimePaths.realPkgRelocationClaimAllowed());
        evidence.setTextIsSelectable(true);
        evidence.setPadding(16, 8, 16, 8);
        stepContent.addView(evidence);
    }

    private void addFilesystemEvidenceView() {
        BootstrapReadinessGate.Report report = BootstrapReadinessGate.evaluate(this);
        TextView evidence = new TextView(this);
        evidence.setText(
            "bootstrap.zip source=" + BootstrapWizardSource.status(this) + "\n" +
            report.render());
        evidence.setTextIsSelectable(true);
        evidence.setPadding(16, 8, 16, 8);
        stepContent.addView(evidence);
    }

    private void addButton(String text, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(text);
        button.setOnClickListener(listener);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.setMargins(16, 16, 16, 16);
        button.setLayoutParams(params);
        stepContent.addView(button);
    }

    private void previousStep() {
        if (currentStep > 0) {
            currentStep--;
            updateWizardStep();
        }
    }

    private void nextStep() {
        boolean passed;
        try {
            passed = wizardChecks.get(currentStep).checkFunction.check();
        } catch (Throwable error) {
            passed = false;
        }
        stepCompleted[currentStep] = passed;
        if (isBlockingStep(currentStep) && !passed) {
            new AlertDialog.Builder(this)
                .setTitle("Required gate is not satisfied")
                .setMessage("This step cannot be skipped. Missing evidence remains BLOCKED/TOKEN_VAZIO until the required action succeeds.")
                .setPositiveButton("OK", null)
                .show();
            updateWizardStep();
            return;
        }

        if (currentStep < TOTAL_STEPS - 1) {
            currentStep++;
            updateWizardStep();
        } else if (checkFinalGate()) {
            finish();
        } else {
            currentStep = 3;
            updateWizardStep();
        }
    }

    private boolean isBlockingStep(int step) {
        return step == 0 || step == 1 || step == 3 || step == 4 || step == 5;
    }

    private boolean checkAndroidVersion() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.N;
    }

    private boolean checkPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(this, android.Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) return false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) return Environment.isExternalStorageManager();
        return ContextCompat.checkSelfPermission(this, android.Manifest.permission.READ_EXTERNAL_STORAGE)
            == PackageManager.PERMISSION_GRANTED
            && ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
            == PackageManager.PERMISSION_GRANTED;
    }

    private boolean checkBatteryOptimization() {
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        return pm != null && pm.isIgnoringBatteryOptimizations(getPackageName());
    }

    private boolean checkBootstrapInstallation() {
        return BootstrapReadinessGate.evaluate(this).isPass();
    }

    private boolean checkSystemCompatibility() {
        String arch = System.getProperty("os.arch");
        if (arch == null || !(arch.contains("arm") || arch.contains("aarch64") || arch.contains("x86") || arch.contains("i686")))
            return false;
        File files = getFilesDir();
        return files != null && files.isDirectory() && files.canRead() && files.canWrite() && files.canExecute();
    }

    private boolean checkSystemCompatibilityAndBootstrap() {
        return checkSystemCompatibility() && checkBootstrapInstallation();
    }

    private boolean checkFinalGate() {
        return checkAndroidVersion() && checkPermissions() && checkSystemCompatibilityAndBootstrap();
    }

    private void requestRequiredPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && !Environment.isExternalStorageManager()) {
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivity(intent);
                return;
            } catch (Throwable ignored) {
                try {
                    startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
                    return;
                } catch (Throwable ignoredToo) {
                }
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            requestPermissions(new String[]{android.Manifest.permission.POST_NOTIFICATIONS}, REQUEST_PERMISSIONS);
        } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            requestPermissions(new String[]{
                android.Manifest.permission.READ_EXTERNAL_STORAGE,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, REQUEST_PERMISSIONS);
        }
    }

    private void openBatterySettings() {
        Intent intent = new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
        intent.setData(Uri.parse("package:" + getPackageName()));
        try {
            startActivity(intent);
        } catch (Throwable error) {
            try {
                startActivity(new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS));
            } catch (Throwable ignored) {
                showManualInstructions();
            }
        }
    }

    private void chooseBootstrapZip() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_BOOTSTRAP_DOCUMENT);
    }

    private void installBootstrapFilesystem() {
        TermuxInstaller.setupBootstrapIfNeeded(this, this::updateWizardStep);
    }

    private void openAuditActivity() {
        startActivity(new Intent(this, SystemAuditActivity.class));
    }

    private void showManualInstructions() {
        new AlertDialog.Builder(this)
            .setTitle("Manual Setup Required")
            .setMessage("Settings → Apps → Termux RAFCODEΦ → Battery → Unrestricted")
            .setPositiveButton("OK", null)
            .show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_BOOTSTRAP_DOCUMENT || resultCode != RESULT_OK || data == null || data.getData() == null)
            return;

        Uri uri = data.getData();
        try {
            getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (Throwable ignored) {
        }

        Toast.makeText(this, "Validating bootstrap.zip…", Toast.LENGTH_SHORT).show();
        new Thread(() -> {
            try {
                JSONObject receipt = BootstrapWizardSource.accept(this, uri);
                runOnUiThread(() -> {
                    Toast.makeText(this,
                        "bootstrap.zip accepted: " + receipt.optString("bootstrap_profile", "UNKNOWN")
                            + " / " + receipt.optString("runtime_layout", "UNKNOWN"),
                        Toast.LENGTH_LONG).show();
                    updateWizardStep();
                });
            } catch (Throwable error) {
                runOnUiThread(() -> {
                    new AlertDialog.Builder(this)
                        .setTitle("bootstrap.zip rejected")
                        .setMessage(error.getClass().getSimpleName() + ": " + String.valueOf(error.getMessage()))
                        .setPositiveButton("OK", null)
                        .show();
                    updateWizardStep();
                });
            }
        }, "wizard-bootstrap-import").start();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        updateWizardStep();
    }

    @Override
    protected void onResume() {
        super.onResume();
        TermuxRuntimePaths.init(this);
        if (progressBar != null && !wizardChecks.isEmpty()) updateWizardStep();
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    private static final class WizardCheck {
        final String title;
        final String description;
        final CheckFunction checkFunction;

        WizardCheck(String title, String description, CheckFunction checkFunction) {
            this.title = title;
            this.description = description;
            this.checkFunction = checkFunction;
        }
    }

    @FunctionalInterface
    private interface CheckFunction {
        boolean check();
    }
}
