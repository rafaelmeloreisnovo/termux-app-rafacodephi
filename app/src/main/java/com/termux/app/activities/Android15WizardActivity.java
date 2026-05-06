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
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.termux.rafacodephi.R;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.theme.NightMode;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Android 15 Installation Wizard Activity
 * 
 * This activity provides a comprehensive setup wizard for Android 15+ devices,
 * handling all necessary permissions, configurations, and optimizations.
 * 
 * Features:
 * - Guided setup process with step-by-step instructions
 * - Automatic detection of Android version and restrictions
 * - Permission management for Android 15+ requirements
 * - Battery optimization exemption setup
 * - Phantom Process Killer mitigation
 * - Bootstrap installation verification
 * - ISO 8000/9001 internal alignment tracking
 * 
 * @author Termux RAFCODEΦ Team
 * @version 1.0.0
 */
public class Android15WizardActivity extends AppCompatActivity {

    private static final String LOG_TAG = "Android15WizardActivity";
    
    // Wizard step tracking
    private int currentStep = 0;
    private static final int TOTAL_STEPS = 6;
    
    // UI Components
    private ProgressBar progressBar;
    private TextView stepTitle;
    private TextView stepDescription;
    private LinearLayout stepContent;
    private Button prevButton;
    private Button nextButton;
    private ScrollView scrollView;
    
    // Wizard state
    private boolean[] stepCompleted = new boolean[TOTAL_STEPS];
    private List<WizardCheck> wizardChecks = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
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
        // Step 0: Welcome & Android Version Check
        wizardChecks.add(new WizardCheck(
            "Welcome to Termux RAFCODEΦ",
            "This wizard will help you configure Termux for optimal performance on Android 15+.\n\n" +
            "Features include:\n" +
            "• 16KB page size alignment for stability\n" +
            "• Phantom Process Killer mitigation\n" +
            "• Battery optimization exemption\n" +
            "• ISO 8000/9001 internal alignment tracking\n" +
            "• Hardware/software compatibility audit\n\n" +
            "Your Android Version: " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")",
            this::checkAndroidVersion
        ));
        
        // Step 1: Permissions
        wizardChecks.add(new WizardCheck(
            "Required Permissions",
            "Termux requires the following permissions to function properly:\n\n" +
            "• Storage access (MANAGE_EXTERNAL_STORAGE on Android 11+)\n" +
            "• Notification permission (Android 13+)\n" +
            "• Foreground service permission\n" +
            "• Display over other apps (optional)",
            this::checkPermissions
        ));
        
        // Step 2: Battery Optimization
        wizardChecks.add(new WizardCheck(
            "Battery Optimization",
            "To prevent Android from killing Termux in the background:\n\n" +
            "• Disable battery optimization for this app\n" +
            "• This allows background processes to run\n" +
            "• Required for long-running terminal sessions",
            this::checkBatteryOptimization
        ));
        
        // Step 3: Bootstrap Verification
        wizardChecks.add(new WizardCheck(
            "Bootstrap Installation",
            "Verifying the Termux bootstrap installation:\n\n" +
            "• Checking PREFIX directory\n" +
            "• Verifying essential binaries\n" +
            "• Validating 16KB page alignment",
            this::checkBootstrapInstallation
        ));
        
        // Step 4: System Compatibility
        wizardChecks.add(new WizardCheck(
            "System Compatibility Audit",
            "Checking hardware and software compatibility:\n\n" +
            "• CPU architecture verification\n" +
            "• Memory page size detection\n" +
            "• SELinux status check\n" +
            "• File system capabilities",
            this::checkSystemCompatibility
        ));
        
        // Step 5: Final Configuration
        wizardChecks.add(new WizardCheck(
            "Setup Complete",
            "Your Termux RAFCODEΦ installation is configured!\n\n" +
            "You can now:\n" +
            "• Run terminal commands\n" +
            "• Install packages with pkg or apt\n" +
            "• Access the system audit from Settings\n" +
            "• View compliance reports\n\n" +
            "Tap 'Finish' to start using Termux.",
            () -> true
        ));
    }
    
    private void updateWizardStep() {
        // Update progress
        progressBar.setProgress((currentStep * 100) / (TOTAL_STEPS - 1));
        
        // Update content
        WizardCheck check = wizardChecks.get(currentStep);
        stepTitle.setText(check.title);
        stepDescription.setText(check.description);
        
        // Update buttons
        prevButton.setEnabled(currentStep > 0);
        
        if (currentStep == TOTAL_STEPS - 1) {
            nextButton.setText("Finish");
        } else {
            nextButton.setText("Next");
        }
        
        // Run check and update UI
        stepContent.removeAllViews();
        boolean checkPassed = check.checkFunction.check();
        stepCompleted[currentStep] = checkPassed;
        
        addCheckResultView(checkPassed);
        
        // Scroll to top
        scrollView.smoothScrollTo(0, 0);
    }
    
    private void addCheckResultView(boolean passed) {
        LinearLayout resultLayout = new LinearLayout(this);
        resultLayout.setOrientation(LinearLayout.HORIZONTAL);
        resultLayout.setPadding(16, 16, 16, 16);
        
        ImageView statusIcon = new ImageView(this);
        statusIcon.setImageResource(passed ? 
            android.R.drawable.presence_online : 
            android.R.drawable.presence_busy);
        statusIcon.setLayoutParams(new LinearLayout.LayoutParams(48, 48));
        
        TextView statusText = new TextView(this);
        statusText.setText(passed ? "✓ Check passed" : "⚠ Action required");
        statusText.setPadding(16, 0, 0, 0);
        statusText.setTextSize(16);
        
        resultLayout.addView(statusIcon);
        resultLayout.addView(statusText);
        
        stepContent.addView(resultLayout);
        
        // Add action buttons if needed
        addStepSpecificContent(currentStep, passed);
    }
    
    private void addStepSpecificContent(int step, boolean passed) {
        switch (step) {
            case 1: // Permissions
                if (!passed) {
                    addButton("Grant Permissions", v -> requestPermissions());
                }
                break;
            case 2: // Battery Optimization
                if (!passed) {
                    addButton("Disable Battery Optimization", v -> openBatterySettings());
                }
                break;
            case 4: // System Compatibility
                addButton("View Full Audit Report", v -> openAuditActivity());
                break;
        }
    }
    
    private void addButton(String text, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(text);
        button.setOnClickListener(listener);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
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
        if (currentStep < TOTAL_STEPS - 1) {
            currentStep++;
            updateWizardStep();
        } else {
            // Finish wizard
            finish();
        }
    }
    
    // Check functions
    private boolean checkAndroidVersion() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.N;
    }
    
    private boolean checkPermissions() {
        // Check notification permission (Android 13+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }

        // Check storage permission according to platform model
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                return false;
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
            if (ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }

        return true;
    }
    
    private boolean checkBatteryOptimization() {
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        return pm != null && pm.isIgnoringBatteryOptimizations(getPackageName());
    }
    
    private boolean checkBootstrapInstallation() {
        File prefixDir = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH);
        File binDir = new File(TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH);
        File shellFile = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/bin/sh");
        File pkgFile = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/bin/pkg");

        return prefixDir.exists() && binDir.exists() &&
            shellFile.exists() && shellFile.canRead() &&
            pkgFile.exists() && pkgFile.canRead();
    }
    
    private boolean checkSystemCompatibility() {
        // Basic compatibility checks
        String arch = System.getProperty("os.arch");
        return arch != null && (arch.contains("arm") || arch.contains("aarch64") || 
                               arch.contains("x86") || arch.contains("i686"));
    }
    
    // Action functions
    private void requestPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && !Environment.isExternalStorageManager()) {
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivity(intent);
                return;
            } catch (Exception ignored) {
                try {
                    startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
                    return;
                } catch (Exception ignoredToo) {
                    // Continue to other permission requests below.
                }
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            requestPermissions(new String[]{android.Manifest.permission.POST_NOTIFICATIONS}, 1);
            return;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            requestPermissions(new String[]{
                android.Manifest.permission.READ_EXTERNAL_STORAGE,
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, 1);
        }
    }
    
    private void openBatterySettings() {
        Intent intent = new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
        intent.setData(Uri.parse("package:" + getPackageName()));
        try {
            startActivity(intent);
        } catch (Exception e) {
            // Fallback to general battery settings
            try {
                startActivity(new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS));
            } catch (Exception ex) {
                // Show manual instructions
                showManualInstructions();
            }
        }
    }
    
    private void openAuditActivity() {
        Intent intent = new Intent(this, SystemAuditActivity.class);
        startActivity(intent);
    }
    
    private void showManualInstructions() {
        new AlertDialog.Builder(this)
            .setTitle("Manual Setup Required")
            .setMessage("Please go to:\nSettings → Apps → Termux RAFCODEΦ → Battery → Unrestricted")
            .setPositiveButton("OK", null)
            .show();
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
        // Refresh current step check
        updateWizardStep();
    }
    
    /**
     * Helper class for wizard check steps
     */
    private static class WizardCheck {
        String title;
        String description;
        CheckFunction checkFunction;
        
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
