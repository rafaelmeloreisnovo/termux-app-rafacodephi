package com.termux.app;

import android.os.Build;
import android.system.Os;
import android.system.StructStat;

import com.termux.rafacodephi.BuildConfig;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;

import java.io.File;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

final class BootstrapBaremetalGuard {
    private static final String LOG_TAG = "BootstrapBaremetalGuard";
    private static final int BUFFER_CAPACITY = 2048;
    private static final ByteBuffer SHARED_BUFFER = ByteBuffer.allocateDirect(BUFFER_CAPACITY);
    private static final boolean LIB_LOADED;

    static {
        boolean loaded;
        try {
            System.loadLibrary("termux-baremetal");
            loaded = true;
        } catch (Throwable t) {
            loaded = false;
            Logger.logWarn(LOG_TAG, "Native guard unavailable: " + t.getMessage());
        }
        LIB_LOADED = loaded;
    }

    private BootstrapBaremetalGuard() {}

    private static native int selftestNative(ByteBuffer out, int cap);
    private static native int validatePrefixNative(String prefix, ByteBuffer out, int cap);

    static void selftest() {
        if (!LIB_LOADED) {
            String msg = "selftest skipped: native lib not loaded";
            if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) throw new RuntimeException(msg);
            Logger.logWarn(LOG_TAG, msg);
            return;
        }
        int rc;
        String json;
        synchronized (SHARED_BUFFER) {
            clearBuffer();
            try {
                rc = selftestNative(SHARED_BUFFER, BUFFER_CAPACITY);
            } catch (UnsatisfiedLinkError e) {
                String msg = "selftestNative missing JNI symbol: " + e.getMessage();
                if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) throw new RuntimeException(msg, e);
                Logger.logWarn(LOG_TAG, msg);
                return;
            }
            json = readBufferString();
        }
        if (rc < 0) {
            String msg = "selftest failed rc=" + rc + " payload=" + json;
            if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) throw new RuntimeException(msg);
            Logger.logWarn(LOG_TAG, msg);
        } else {
            Logger.logInfo(LOG_TAG, "selftest ok payload=" + json);
        }
        Logger.logInfo(LOG_TAG, "bootstrap-guard phase=selftest status=ok payload=" + json);
    }

    static void validateAfterBootstrap(String prefix) {
        validateInstallFilesystemAndShell(prefix);

        PaPayloadInstaller.ensureInstalled(prefix);

        if (!LIB_LOADED) {
            String msg = "Skipped guard validation: native lib not loaded";
            if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) throw new RuntimeException(msg);
            Logger.logWarn(LOG_TAG, msg);
            return;
        }
        int rc;
        String json;
        synchronized (SHARED_BUFFER) {
            clearBuffer();
            try {
                rc = validatePrefixNative(prefix, SHARED_BUFFER, BUFFER_CAPACITY);
            } catch (UnsatisfiedLinkError e) {
                String msg = "validatePrefixNative missing JNI symbol: " + e.getMessage();
                if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) throw new RuntimeException(msg, e);
                Logger.logWarn(LOG_TAG, msg);
                return;
            }
            json = readBufferString();
        }
        if (rc < 0) {
            handleStrictFailure("validatePrefix", "critical native return rc=" + rc + " payload=" + json, null);
            return;
        }
        Logger.logInfo(LOG_TAG, "bootstrap-guard phase=validatePrefix status=ok payload=" + json);
    }

    private static void validateInstallFilesystemAndShell(String prefix) {
        if (prefix == null || prefix.trim().isEmpty()) {
            throw new RuntimeException("Install filesystem guard failed: empty prefix");
        }

        File prefixDir = new File(prefix);
        ensureDirectory(prefixDir, 0700, "$PREFIX");
        ensureDirectory(new File(prefixDir, "bin"), 0700, "$PREFIX/bin");
        ensureDirectory(new File(prefixDir, "etc"), 0700, "$PREFIX/etc");
        ensureDirectory(new File(prefixDir, "etc/termux"), 0700, "$PREFIX/etc/termux");
        ensureDirectory(new File(prefixDir, "tmp"), 0700, "$PREFIX/tmp");
        ensureDirectory(new File(prefixDir, "var"), 0700, "$PREFIX/var");
        ensureDirectory(new File(prefixDir, "var/tmp"), 0700, "$PREFIX/var/tmp");

        ensureDirectory(TermuxConstants.TERMUX_HOME_DIR, 0700, "$HOME");
        ensureDirectory(TermuxConstants.TERMUX_DATA_HOME_DIR, 0700, "$HOME/.termux");
        ensureDirectory(TermuxConstants.TERMUX_CONFIG_HOME_DIR, 0700, "$HOME/.config/termux");
        ensureStoragePlaceholder(TermuxConstants.TERMUX_STORAGE_HOME_DIR);

        verifyOwnerExecutable(new File(prefixDir, "bin/sh"), "bootstrap shell");
        verifyOwnerExecutable(new File(prefixDir, "bin/pkg"), "bootstrap package manager");

        String primaryAbi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "unknown";
        Logger.logInfo(LOG_TAG, "bootstrap-guard phase=installFilesystemShell status=ok abi=" + primaryAbi +
            " arm32=" + "armeabi-v7a".equals(primaryAbi) + " prefix=" + prefix);
    }

    private static void ensureDirectory(File directory, int mode, String label) {
        if (directory == null) {
            throw new RuntimeException("Install filesystem guard failed: null directory for " + label);
        }
        if (directory.exists() && !directory.isDirectory()) {
            throw new RuntimeException("Install filesystem guard failed: " + label + " is not a directory: " + directory.getAbsolutePath());
        }
        if (!directory.exists() && !directory.mkdirs() && !directory.isDirectory()) {
            throw new RuntimeException("Install filesystem guard failed: could not create " + label + ": " + directory.getAbsolutePath());
        }
        try {
            Os.chmod(directory.getAbsolutePath(), mode);
        } catch (Exception e) {
            throw new RuntimeException("Install filesystem guard failed: chmod " + label + " to 0" + Integer.toOctalString(mode) +
                " at " + directory.getAbsolutePath(), e);
        }
    }

    private static void ensureStoragePlaceholder(File storageHome) {
        if (storageHome == null) {
            throw new RuntimeException("Install filesystem guard failed: null storage home directory");
        }
        if (storageHome.exists()) {
            if (!storageHome.isDirectory()) {
                throw new RuntimeException("Install filesystem guard failed: $HOME/storage is not a directory: " + storageHome.getAbsolutePath());
            }
            Logger.logInfo(LOG_TAG, "bootstrap-guard phase=installStoragePlaceholder status=existing path=" + storageHome.getAbsolutePath());
            return;
        }
        if (!storageHome.mkdirs() && !storageHome.isDirectory()) {
            throw new RuntimeException("Install filesystem guard failed: could not create $HOME/storage placeholder: " + storageHome.getAbsolutePath());
        }
        try {
            Os.chmod(storageHome.getAbsolutePath(), 0700);
        } catch (Exception e) {
            throw new RuntimeException("Install filesystem guard failed: chmod $HOME/storage placeholder at " + storageHome.getAbsolutePath(), e);
        }
        Logger.logInfo(LOG_TAG, "bootstrap-guard phase=installStoragePlaceholder status=created path=" + storageHome.getAbsolutePath());
    }

    private static void verifyOwnerExecutable(File file, String label) {
        try {
            if (!file.exists()) {
                throw new RuntimeException("missing " + label + ": " + file.getAbsolutePath());
            }
            StructStat stat = Os.stat(file.getAbsolutePath());
            if ((stat.st_mode & 0100) == 0) {
                throw new RuntimeException(label + " is not executable by owner. mode=0" + Integer.toOctalString(stat.st_mode));
            }
        } catch (RuntimeException e) {
            throw new RuntimeException("Install filesystem guard failed: " + e.getMessage(), e);
        } catch (Exception e) {
            throw new RuntimeException("Install filesystem guard failed for " + label + " at " + file.getAbsolutePath(), e);
        }
    }

    private static void handleStrictFailure(String phase, String cause, Throwable error) {
        String message = "bootstrap-guard phase=" + phase + " status=failed cause=" + cause;
        if (error != null && error.getMessage() != null && !error.getMessage().isEmpty()) {
            message += " detail=" + error.getMessage();
        }
        if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) {
            throw new RuntimeException(message, error);
        }
        Logger.logWarn(LOG_TAG, message + " strict=false");
    }

    private static void clearBuffer() {
        SHARED_BUFFER.position(0);
        for (int i = 0; i < BUFFER_CAPACITY; i++) SHARED_BUFFER.put((byte) 0);
        SHARED_BUFFER.position(0);
    }

    private static String readBufferString() {
        byte[] data = new byte[BUFFER_CAPACITY];
        SHARED_BUFFER.position(0);
        SHARED_BUFFER.get(data);
        int len = 0;
        while (len < data.length && data[len] != 0) len++;
        return new String(data, 0, len, StandardCharsets.UTF_8);
    }
}
