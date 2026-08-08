package com.termux.shared.termux.shell.command.environment;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.environment.AndroidShellEnvironment;
import com.termux.shared.shell.command.environment.ShellEnvironmentUtils;
import com.termux.shared.termux.TermuxBootstrap;
import com.termux.shared.termux.TermuxRuntimePaths;
import com.termux.shared.termux.shell.TermuxShellUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.Charset;
import java.util.HashMap;

/** Environment for Termux with Android-assigned app-private runtime paths. */
public class TermuxShellEnvironment extends AndroidShellEnvironment {

    private static final String LOG_TAG = "TermuxShellEnvironment";
    public static final String ENV_PREFIX = "PREFIX";

    public TermuxShellEnvironment() {
        super();
        shellCommandShellEnvironment = new TermuxShellCommandShellEnvironment();
    }

    /** Init runtime path state and app shell constants/caches. */
    public synchronized static void init(@NonNull Context currentPackageContext) {
        TermuxRuntimePaths.init(currentPackageContext);
        TermuxAppShellEnvironment.setTermuxAppEnvironment(currentPackageContext);
    }

    /** Persist the exact environment used by the current Android-assigned layout. */
    public synchronized static void writeEnvironmentToFile(@NonNull Context currentPackageContext) {
        TermuxRuntimePaths.init(currentPackageContext);
        HashMap<String, String> environmentMap = new TermuxShellEnvironment().getEnvironment(currentPackageContext, false);
        String environmentString = ShellEnvironmentUtils.convertEnvironmentToDotEnvFile(environmentMap);
        File directory = new File(TermuxRuntimePaths.envDirPath());
        if (!directory.exists() && !directory.mkdirs() && !directory.isDirectory()) {
            Logger.logError(LOG_TAG, "Could not create runtime env directory: " + directory);
            return;
        }

        File temp = new File(TermuxRuntimePaths.envTempFilePath());
        File target = new File(TermuxRuntimePaths.envFilePath());
        try (FileOutputStream output = new FileOutputStream(temp)) {
            output.write(environmentString.getBytes(Charset.defaultCharset()));
            output.flush();
            output.getFD().sync();
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to write runtime shell environment temp file", e);
            return;
        }

        if (target.exists() && !target.delete()) {
            Logger.logError(LOG_TAG, "Could not replace runtime shell environment file: " + target);
            return;
        }
        if (!temp.renameTo(target)) {
            Logger.logError(LOG_TAG, "Could not atomically promote runtime shell environment file: " + temp + " -> " + target);
            return;
        }
        Logger.logInfo(LOG_TAG, "Runtime shell environment written path=" + target
            + " layout=" + TermuxRuntimePaths.layoutState());
    }

    @NonNull
    @Override
    public HashMap<String, String> getEnvironment(@NonNull Context currentPackageContext, boolean isFailSafe) {
        TermuxRuntimePaths.init(currentPackageContext);
        HashMap<String, String> environment = super.getEnvironment(currentPackageContext, isFailSafe);

        HashMap<String, String> termuxAppEnvironment = TermuxAppShellEnvironment.getEnvironment(currentPackageContext);
        if (termuxAppEnvironment != null) environment.putAll(termuxAppEnvironment);

        HashMap<String, String> termuxApiAppEnvironment = TermuxAPIShellEnvironment.getEnvironment(currentPackageContext);
        if (termuxApiAppEnvironment != null) environment.putAll(termuxApiAppEnvironment);

        environment.put(ENV_HOME, TermuxRuntimePaths.homeDirPath());
        environment.put(ENV_PREFIX, TermuxRuntimePaths.prefixDirPath());

        if (!isFailSafe) {
            environment.put(ENV_TMPDIR, TermuxRuntimePaths.tmpDirPath());
            if (TermuxBootstrap.isAppPackageVariantAPTAndroid5()) {
                environment.put(ENV_PATH, TermuxRuntimePaths.binDirPath() + ":" + TermuxRuntimePaths.binDirPath() + "/applets");
                environment.put(ENV_LD_LIBRARY_PATH, TermuxRuntimePaths.libDirPath());
            } else {
                environment.put(ENV_PATH, TermuxRuntimePaths.binDirPath());
                environment.remove(ENV_LD_LIBRARY_PATH);
            }
        }

        environment.put("TERMUX_RUNTIME_FILES_DIR", TermuxRuntimePaths.filesDirPath());
        environment.put("TERMUX_RUNTIME_LAYOUT", TermuxRuntimePaths.layoutState());
        environment.put("TERMUX_REAL_PKG_RELOCATION_CLAIM_ALLOWED", "false");
        return environment;
    }

    @NonNull
    @Override
    public String getDefaultWorkingDirectoryPath() {
        return TermuxRuntimePaths.homeDirPath();
    }

    @NonNull
    @Override
    public String getDefaultBinPath() {
        return TermuxRuntimePaths.binDirPath();
    }

    @NonNull
    @Override
    public String[] setupShellCommandArguments(@NonNull String executable, String[] arguments) {
        return TermuxShellUtils.setupShellCommandArguments(executable, arguments);
    }
}
