package com.termux.app;

import android.os.Build;
import android.system.Os;
import android.system.StructStat;

import com.termux.rafacodephi.BuildConfig;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

final class BootstrapBaremetalGuard {
    private static final String LOG_TAG = "BootstrapBaremetalGuard";
    private static final int BUFFER_CAPACITY = 2048;
    private static final int PROFILE_READ_LIMIT = 64 * 1024;
    private static final String PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1";
    private static final String PROFILE_FILE = "BOOTSTRAP_PROFILE.json";
    private static final String LEGACY_PREFIX = "/data/data/com.termux/files/usr";
    private static final String[] BRIDGE_MARKERS = new String[] {
        "RAFCODEPHI pkg bridge",
        "RAFCODEPHI apt bridge",
        "real apt backend is not installed yet",
        "real apt/apt-get backend is not installed yet"
    };
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
        validateBootstrapProfileContract(prefix);

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

    private static void validateBootstrapProfileContract(String prefix) {
        File prefixDir = new File(prefix);
        File profileFile = new File(prefixDir, PROFILE_FILE);
        if (!profileFile.isFile()) {
            handleStrictFailure("bootstrapProfile", "missing " + PROFILE_FILE, null);
            return;
        }

        try {
            JSONObject profile = new JSONObject(readBoundedUtf8(profileFile, PROFILE_READ_LIMIT));
            requireEquals(PROFILE_SCHEMA, profile.optString("schema", ""), "profile schema");

            String profileName = profile.optString("profile", "");
            if (!"bridge".equals(profileName) && !"real-pkg".equals(profileName)) {
                throw new RuntimeException("unsupported bootstrap profile: " + profileName);
            }

            requireEquals(BuildConfig.APPLICATION_ID, profile.optString("package_name", ""), "profile package");
            requireEquals(prefix, profile.optString("prefix", ""), "profile prefix");
            requireEquals(expectedBootstrapArch(), profile.optString("arch", ""), "profile arch");

            if (profile.optBoolean("claim_allowed", true)) {
                throw new RuntimeException("bootstrap profile must keep claim_allowed=false");
            }
            if (profile.optBoolean("release_allowed", true)) {
                throw new RuntimeException("bootstrap profile must keep release_allowed=false");
            }
            requireEquals("TOKEN_VAZIO", profile.optString("device_validation", ""), "device validation");

            JSONArray required = profile.optJSONArray("required_entries");
            if (required == null || required.length() == 0) {
                throw new RuntimeException("bootstrap profile has no required_entries");
            }
            for (int i = 0; i < required.length(); i++) {
                verifyRequiredProfileEntry(prefixDir, required.optString(i, ""));
            }

            if ("bridge".equals(profileName)) {
                requireBridgeMarker(new File(prefixDir, "bin/pkg"), "pkg");
                requireBridgeMarker(new File(prefixDir, "bin/apt"), "apt");
                Logger.logWarn(LOG_TAG,
                    "bootstrap-guard phase=bootstrapProfile status=bridge-only device_validation=TOKEN_VAZIO");
            } else {
                verifyElf(new File(prefixDir, "bin/apt"), "apt");
                verifyElf(new File(prefixDir, "bin/apt-get"), "apt-get");
                verifyElf(new File(prefixDir, "bin/dpkg"), "dpkg");
                rejectBridgeMarker(new File(prefixDir, "bin/pkg"), "pkg");
                rejectBridgeMarker(new File(prefixDir, "bin/apt"), "apt");
                rejectBridgeMarker(new File(prefixDir, "bin/apt-get"), "apt-get");
                verifyLibApt(prefixDir);
                verifySourcesList(new File(prefixDir, "etc/apt/sources.list"));
                rejectLegacyPrefixInCriticalFiles(prefixDir);
                Logger.logInfo(LOG_TAG,
                    "bootstrap-guard phase=bootstrapProfile status=real-pkg-structural-candidate " +
                    "device_validation=TOKEN_VAZIO claim_allowed=false");
            }
        } catch (RuntimeException e) {
            handleStrictFailure("bootstrapProfile", e.getMessage(), e);
        } catch (Exception e) {
            handleStrictFailure("bootstrapProfile", "profile parse or I/O failure", e);
        }
    }

    private static void verifyRequiredProfileEntry(File prefixDir, String relative) throws IOException {
        if (relative == null || relative.isEmpty() || relative.startsWith("/") ||
            relative.contains("..") || relative.contains("\\")) {
            throw new RuntimeException("unsafe required profile entry: " + relative);
        }
        String canonicalPrefix = prefixDir.getCanonicalPath() + File.separator;
        File target = new File(prefixDir, relative);
        String canonicalTarget = target.getCanonicalPath();
        if (!canonicalTarget.startsWith(canonicalPrefix)) {
            throw new RuntimeException("required profile entry escapes prefix: " + relative);
        }
        if (!target.exists()) {
            throw new RuntimeException("missing required profile entry: " + relative);
        }
    }

    private static void requireBridgeMarker(File file, String label) throws IOException {
        String content = readBoundedUtf8(file, PROFILE_READ_LIMIT);
        if (!containsBridgeMarker(content)) {
            throw new RuntimeException("bridge profile does not identify " + label + " as bridge");
        }
    }

    private static void rejectBridgeMarker(File file, String label) throws IOException {
        String content = readBoundedUtf8(file, PROFILE_READ_LIMIT);
        if (containsBridgeMarker(content)) {
            throw new RuntimeException("real-pkg profile contains bridge marker in " + label);
        }
    }

    private static boolean containsBridgeMarker(String content) {
        for (String marker : BRIDGE_MARKERS) {
            if (content.contains(marker)) return true;
        }
        return false;
    }

    private static void verifyElf(File file, String label) throws IOException {
        byte[] magic = readBounded(file, 4);
        if (magic.length != 4 || magic[0] != 0x7f || magic[1] != 'E' ||
            magic[2] != 'L' || magic[3] != 'F') {
            throw new RuntimeException("real-pkg requires ELF binary for " + label);
        }
        verifyOwnerExecutable(file, label);
    }

    private static void verifyLibApt(File prefixDir) {
        File libDir = new File(prefixDir, "lib");
        File[] matches = libDir.listFiles((dir, name) ->
            name.equals("libapt-pkg.so") || name.startsWith("libapt-pkg.so."));
        if (matches == null || matches.length == 0) {
            throw new RuntimeException("real-pkg missing libapt-pkg");
        }
    }

    private static void verifySourcesList(File sourcesList) throws IOException {
        String text = readBoundedUtf8(sourcesList, PROFILE_READ_LIMIT);
        boolean found = false;
        for (String raw : text.split("\\r?\\n")) {
            String line = raw.trim();
            if (!line.isEmpty() && !line.startsWith("#") &&
                (line.contains("https://") || line.contains("http://"))) {
                found = true;
                break;
            }
        }
        if (!found) throw new RuntimeException("sources.list has no HTTP(S) repository");
    }

    private static void rejectLegacyPrefixInCriticalFiles(File prefixDir) throws IOException {
        String[] critical = new String[] {
            "bin/apt", "bin/apt-get", "bin/dpkg", "bin/pkg", "etc/apt/sources.list"
        };
        for (String relative : critical) {
            File file = new File(prefixDir, relative);
            if (file.isFile() && readBoundedUtf8(file, PROFILE_READ_LIMIT).contains(LEGACY_PREFIX)) {
                throw new RuntimeException("legacy Termux prefix found in " + relative);
            }
        }
    }

    private static byte[] readBounded(File file, int limit) throws IOException {
        if (!file.isFile()) throw new RuntimeException("missing file: " + file.getAbsolutePath());
        byte[] output = new byte[limit];
        int total = 0;
        try (FileInputStream input = new FileInputStream(file)) {
            while (total < limit) {
                int count = input.read(output, total, limit - total);
                if (count < 0) break;
                total += count;
            }
        }
        byte[] exact = new byte[total];
        System.arraycopy(output, 0, exact, 0, total);
        return exact;
    }

    private static String readBoundedUtf8(File file, int limit) throws IOException {
        return new String(readBounded(file, limit), StandardCharsets.UTF_8);
    }

    private static String expectedBootstrapArch() {
        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "";
        if ("armeabi-v7a".equals(abi)) return "arm";
        if ("arm64-v8a".equals(abi)) return "aarch64";
        if ("x86".equals(abi)) return "i686";
        if ("x86_64".equals(abi)) return "x86_64";
        return "unknown";
    }

    private static void requireEquals(String expected, String actual, String label) {
        if (!expected.equals(actual)) {
            throw new RuntimeException(label + " mismatch expected=" + expected + " actual=" + actual);
        }
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
