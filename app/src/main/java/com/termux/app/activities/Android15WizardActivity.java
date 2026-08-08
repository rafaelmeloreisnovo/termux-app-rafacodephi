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

import com.termux.app.BootstrapWizardSource;
import com.termux.app.TermuxInstaller;
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
 * Installation wizard with explicit filesystem source, runtime-path evidence and
 * fail-closed bootstrap gate.
 */
public class Android15WizardActivity extends AppCompatActivity {

    private static final int REQUEST_BOOTSTRAP_DOCUMENT = 4201;
    private static final int REQUEST_PERMISSIONS = 4202;
    private static final int TOTAL_STEPS = 6;

    private int currentStep = 0;
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
            "Welcome to Termux RAFCODEΦ",
            "This wizard validates the actual Android-assigned runtime before terminal startup.\n\n"
                + "Android: " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")\n"
                + "Runtime layout: " + TermuxRuntimePaths.layoutState(),
            this::checkAndroidVersion));

        wizardChecks.add(new WizardCheck(
            "Required Permissions",
            "Storage access is validated separately from the app-private filesystem. "
                + "A storage permission cannot repair an incorrect /data/data path assumption.",
            this::checkPermissions));

        wizardChecks.add(new WizardCheck(
            "Battery Optimization",
            "Background reliability check. This is advisory for filesystem installation and does not fabricate a bootstrap PASS.",
            this::checkBatteryOptimization));

        wizardChecks.add(new WizardCheck(
            "Bootstrap Filesystem",
            "Install the filesystem into the private directory assigned by Android.\n\n"
                + "You can use the embedded canonical bootstrap or explicitly select bootstrap.zip. "
                + "A selected file is copied into the private inbox, BLAKE3 verified, ZIP-safety checked and bound to ABI before use.\n\n"
                + "Relocated /mnt/expand layouts accept only a bridge bootstrap that explicitly keeps claim_allowed=false. "
                + "Real apt/dpkg relocation remains blocked until binaries are rebuilt/validated for that prefix.",
            this::checkBootstrapInstallation));

        wizardChecks.add(new WizardCheck(
            "System Compatibility Audit",
            "Checking CPU architecture and runtime filesystem capability. The canonical compile-time prefix and Android-assigned prefix are reported separately.",
            this::checkSystemCompatibility));

        wizardChecks.add(new WizardCheck(
            "Setup Complete",
            "Required gates passed. The terminal may start using the runtime-resolved app-private filesystem. "
                + "This does not promote a bridge bootstrap to a real-pkg apt/dpkg claim.",
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
        } catch (Throwable t) {
            passed = false;
        }
        stepCompleted[currentStep] = passed;
        addCheckResultView(passed);
        addStepSpecificContent(currentStep, passed);
        scrollView.smoothScrollTo(0, 0);
    }

    private void addCheckResultView(boolean passed) {
        LinearLayout resultLayout = new LinearLayout(this);
        resultLayout.setOrientation(LinearLayout.HORIZONTAL);
        resultLayout.setPadding(16, 16, 16, 16);

        ImageView statusIcon = new ImageView(this);
        statusIcon.setImageResource(passed ? android.R.drawable.presence_online : android.R.drawable.presence_busy);
        statusIcon.setLayoutParams(new LinearLayout.LayoutParams(48, 48));

        TextView statusText = new TextView(this);
        statusText.setText(passed ? "✓ Gate passed" : "⚠ Action required");
        statusText.setPadding(16, 0, 0, 0);
        statusText.setTextSize(16);

        resultLayout.addView(statusIcon);
        resultLayout.addView(statusText);
        stepContent.addView(resultLayout);
    }

    private void addStepSpecificContent(int step, boolean passed) {
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
                    Toast.makeText(this, "Selected bootstrap cleared; embedded bootstrap is fallback.", Toast.LENGTH_SHORT).show();
                    updateWizardStep();
                });
            }
            if (!passed) addButton("Install Filesystem", v -> installBootstrapFilesystem());
            else addButton("Re-check Filesystem", v -> updateWizardStep());
            return;
        }
        if (step == 4) addButton("View Full Audit Report", v -> openAuditActivity());
    }

    private void addFilesystemEvidenceView() {
        TextView evidence = new TextView(this);
        StringBuilder text = new StringBuilder();
        text.append("Android-assigned filesDir: ").append(TermuxRuntimePaths.filesDirPath()).append("\n");
        text.append("Runtime PREFIX: ").append(TermuxRuntimePaths.prefixDirPath()).append("\n");
        text.append("Canonical compiled PREFIX: ").append(TermuxConstants.TERMUX_PREFIX_DIR_PATH).append("\n");
        text.append("Path state: ").append(TermuxRuntimePaths.layoutState()).append("\n");
        text.append("bootstrap.zip source: ").append(BootstrapWizardSource.status(this)).append("\n");
        text.append("Real-pkg relocation claim: BLOCKED / false");
        evidence.setText(text.toString());
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
        // Re-evaluate before transition. Required steps fail closed.
        boolean passed = wizardChecks.get(currentStep).checkFunction.check();
        stepCompleted[currentStep] = passed;
        if (isBlockingStep(currentStep) && !passed) {
            new AlertDialog.Builder(this)
                .setTitle("Required gate is not satisfied")
                .setMessage("This step cannot be skipped. Complete the action and re-check it before continuing.")
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
        TermuxRuntimePaths.init(this);
        File prefix = TermuxRuntimePaths.prefixDir();
        File bin = new File(TermuxRuntimePaths.binDirPath());
        File shell = new File(bin, "sh");
        File pkg = new File(bin, "pkg");
        File busybox = new File(bin, "busybox");
        File proot = new File(bin, "proot");
        return prefix.isDirectory() && bin.isDirectory()
            && shell.isFile() && shell.canExecute()
            && pkg.isFile() && pkg.canExecute()
            && busybox.isFile() && busybox.canExecute()
            && proot.isFile() && proot.canExecute();
    }

    private boolean checkSystemCompatibility() {
        String arch = System.getProperty("os.arch");
        if (arch == null || !(arch.contains("arm") || arch.contains("aarch64") || arch.contains("x86") || arch.contains("i686")))
            return false;
        File files = getFilesDir();
        return files != null && files.isDirectory() && files.canRead() && files.canWrite() && files.canExecute();
    }

    private boolean checkFinalGate() {
        return checkAndroidVersion() && checkPermissions() && checkBootstrapInstallation() && checkSystemCompatibility();
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
        } catch (Throwable e) {
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
            .setMessage("Please go to Settings → Apps → Termux RAFCODEΦ → Battery → Unrestricted")
            .setPositiveButton("OK", null)
            .show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_BOOTSTRAP_DOCUMENT || resultCode != RESULT_OK || data == null || data.getData() == null)
            return;

        final Uri uri = data.getData();
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
            } catch (Throwable t) {
                runOnUiThread(() -> {
                    new AlertDialog.Builder(this)
                        .setTitle("bootstrap.zip rejected")
                        .setMessage(t.getClass().getSimpleName() + ": " + String.valueOf(t.getMessage()))
                        .setPositiveButton("OK", null)
                        .show();
                    updateWizardStep();
                });
            }
        }, "wizard-bootstrap-import").start();
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
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

    private static class WizardCheck {
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
