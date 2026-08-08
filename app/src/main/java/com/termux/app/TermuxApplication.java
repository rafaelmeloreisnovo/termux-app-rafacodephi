package com.termux.app;

import android.app.Application;
import android.content.Context;

import com.termux.app.rafaelia.RafaeliaZeroRuntime;
import com.termux.rafacodephi.BuildConfig;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxBootstrap;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxRuntimePaths;
import com.termux.shared.termux.crash.TermuxCrashUtils;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;
import com.termux.shared.termux.shell.am.TermuxAmSocketServer;
import com.termux.shared.termux.shell.TermuxShellManager;
import com.termux.shared.termux.theme.TermuxThemeUtils;

import java.io.File;
import java.io.FileOutputStream;

public class TermuxApplication extends Application {

    private static final String LOG_TAG = "TermuxApplication";

    @Override
    public void onCreate() {
        super.onCreate();
        try {
            initializeApplication();
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Application initialization failed", e);
        }
    }

    private void initializeApplication() {
        Context context = getApplicationContext();
        TermuxRuntimePaths.init(context);
        Logger.logInfo(LOG_TAG, "runtime-filesystem filesDir=" + TermuxRuntimePaths.filesDirPath()
            + " prefix=" + TermuxRuntimePaths.prefixDirPath()
            + " canonicalPrefix=" + TermuxConstants.TERMUX_PREFIX_DIR_PATH
            + " layout=" + TermuxRuntimePaths.layoutState());

        try {
            TermuxCrashUtils.setDefaultCrashHandler(this);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to set crash handler", e);
        }

        try {
            setLogConfig(context);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to set log config", e);
        }

        try {
            com.termux.app.api.ApiLowLevelBridge.nativeInit();
            Logger.logDebug(LOG_TAG, "ApiLowLevelBridge.nativeInit() ok");
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "ApiLowLevelBridge nativeInit failed", e);
        }

        try {
            int zeroStatus = RafaeliaZeroRuntime.init();
            if (zeroStatus == RafaeliaZeroRuntime.OK)
                Logger.logDebug(LOG_TAG, "RafaeliaZeroRuntime init ok arch=" + RafaeliaZeroRuntime.architectureId());
            else
                Logger.logError(LOG_TAG, "RafaeliaZeroRuntime init status=" + zeroStatus);
        } catch (Throwable t) {
            Logger.logStackTraceWithMessage(LOG_TAG, "RafaeliaZeroRuntime initialization failed", t);
        }

        try {
            BootstrapBaremetalGuard.selftest();
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "BootstrapBaremetalGuard selftest unavailable", e);
        }

        try {
            TermuxBootstrap.setTermuxPackageManagerAndVariant(BuildConfig.TERMUX_PACKAGE_VARIANT);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to set package manager variant", e);
        }

        TermuxAppSharedProperties properties = null;
        try {
            properties = TermuxAppSharedProperties.init(context);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to initialize shared properties", e);
        }

        try {
            TermuxShellManager.init(context);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to initialize shell manager", e);
        }

        try {
            if (properties != null) TermuxThemeUtils.setAppNightMode(properties.getNightMode());
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to set night mode", e);
        }

        boolean runtimeFilesAccessible = runtimeFilesDirectoryAccessible();
        if (runtimeFilesAccessible) {
            Logger.logInfo(LOG_TAG, "Android-assigned Termux files directory is accessible");
            if (TermuxRuntimePaths.isCanonicalLayout()) {
                try {
                    // The existing AM socket helper still owns canonical-prefix assumptions.
                    TermuxAmSocketServer.setupTermuxAmSocketServer(context);
                } catch (Throwable e) {
                    Logger.logStackTraceWithMessage(LOG_TAG, "Failed to setup termux-am socket server", e);
                }
            } else {
                Logger.logWarn(LOG_TAG,
                    "termux-am socket setup skipped for relocated layout until its path contract is runtime-resolved");
            }
        } else {
            Logger.logError(LOG_TAG, "Android-assigned Termux files directory is not writable: "
                + TermuxRuntimePaths.filesDirPath());
        }

        try {
            TermuxShellEnvironment.init(this);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to initialize shell environment", e);
        }

        if (runtimeFilesAccessible) {
            initializeInstalledBootstrapEnvironment();
            writeShellEnvironmentFile("application-startup");
        }
    }

    /** Direct capability probe of Context.getFilesDir(), not a hard-coded /data/data alias. */
    private boolean runtimeFilesDirectoryAccessible() {
        File files = TermuxRuntimePaths.filesDir();
        try {
            if (!files.exists() && !files.mkdirs() && !files.isDirectory()) return false;
            if (!files.isDirectory() || !files.canRead() || !files.canWrite() || !files.canExecute()) return false;
            File probe = new File(files, ".rafcodephi-app-init-write-probe.tmp");
            try (FileOutputStream output = new FileOutputStream(probe)) {
                output.write(0x52);
                output.flush();
                output.getFD().sync();
            }
            boolean ok = probe.isFile() && probe.length() == 1L;
            if (!probe.delete()) Logger.logWarn(LOG_TAG, "Could not delete app init write probe");
            return ok;
        } catch (Throwable t) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Runtime files directory capability probe failed", t);
            return false;
        }
    }

    private void initializeInstalledBootstrapEnvironment() {
        File shell = new File(TermuxRuntimePaths.binDirPath(), "sh");
        File packageManager = new File(TermuxRuntimePaths.binDirPath(), "pkg");
        if (!shell.isFile() || !packageManager.isFile()) {
            Logger.logInfo(LOG_TAG, "bootstrap-env-init skipped: runtime prefix not ready shell="
                + shell.isFile() + " pkg=" + packageManager.isFile());
            return;
        }

        try {
            Logger.logInfo(LOG_TAG, "bootstrap-env-init phase=guard-existing-prefix prefix=" + TermuxRuntimePaths.prefixDirPath());
            BootstrapBaremetalGuard.validateAfterBootstrap(TermuxRuntimePaths.prefixDirPath());
            Logger.logInfo(LOG_TAG, "bootstrap-env-init phase=guard-existing-prefix status=ok");
        } catch (Throwable t) {
            Logger.logStackTraceWithMessage(LOG_TAG, "bootstrap-env-init failed for existing runtime prefix", t);
            if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT)
                throw new RuntimeException("Existing bootstrap environment failed initialization", t);
        }
    }

    private void writeShellEnvironmentFile(String phase) {
        try {
            Logger.logInfo(LOG_TAG, "bootstrap-env-init phase=" + phase + " action=write-shell-environment");
            TermuxShellEnvironment.writeEnvironmentToFile(this);
        } catch (Throwable e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to write runtime environment file", e);
        }
    }

    public static void setLogConfig(Context context) {
        Logger.setDefaultLogTag(TermuxConstants.TERMUX_APP_NAME);
        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return;
        preferences.setLogLevel(null, preferences.getLogLevel());
    }
}
