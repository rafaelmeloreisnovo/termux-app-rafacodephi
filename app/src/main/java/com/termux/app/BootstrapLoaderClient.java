package com.termux.app;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;

import com.termux.rafacodephi.BuildConfig;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;

import java.io.File;
import java.net.URL;
import java.util.Locale;
import java.util.regex.Pattern;

/** Host-side explicit client for the separately signed loader APK. */
final class BootstrapLoaderClient {

    private static final String LOG_TAG = "BootstrapLoaderClient";
    private static final String LOADER_PACKAGE = "com.termux.rafacodephi.loader";
    private static final String LOADER_ACTIVITY =
            "com.termux.rafacodephi.loader.LoaderActivity";
    private static final String ACTION_INSTALL_BOOTSTRAP =
            "com.termux.rafacodephi.loader.INSTALL_BOOTSTRAP";
    private static final String EXTRA_ABI = "abi";
    private static final String EXTRA_SHA256 = "expected_sha256";
    private static final String EXTRA_SOURCE_URL = "source_url";
    private static final String META_CONTRACT_VERSION =
            "com.termux.rafacodephi.loader.CONTRACT_VERSION";
    private static final String META_CONTRACT_STATE =
            "com.termux.rafacodephi.loader.CONTRACT_STATE";
    private static final Pattern SHA256 = Pattern.compile("^[0-9a-f]{64}$");

    private BootstrapLoaderClient() {}

    /**
     * Starts the loader only when the host build pins both URL and SHA-256.
     * Returns true when control was transferred to the loader.
     */
    static boolean requestIfConfigured(Activity activity) {
        if (prefixReady() || acceptedExternalBootstrapExists(activity)) return false;

        String abi = currentBootstrapAbi();
        String url = configuredUrl(abi);
        String sha256 = configuredSha256(abi).toLowerCase(Locale.US);
        if (url.isEmpty() && sha256.isEmpty()) {
            Logger.logInfo(LOG_TAG, "No external bootstrap pin for ABI " + abi
                    + "; embedded bootstrap remains canonical");
            return false;
        }
        if (url.isEmpty() || !SHA256.matcher(sha256).matches()) {
            throw new IllegalStateException("EXTERNAL_BOOTSTRAP_PIN_INCOMPLETE");
        }
        validateHttpsUrl(url);
        verifyLoaderIdentity(activity);

        Intent request = new Intent(ACTION_INSTALL_BOOTSTRAP);
        request.setComponent(new ComponentName(LOADER_PACKAGE, LOADER_ACTIVITY));
        request.putExtra(EXTRA_ABI, abi);
        request.putExtra(EXTRA_SHA256, sha256);
        request.putExtra(EXTRA_SOURCE_URL, url);
        activity.startActivity(request);
        Logger.logInfo(LOG_TAG, "Transferred pinned bootstrap acquisition to signed loader: abi="
                + abi);
        return true;
    }

    private static boolean prefixReady() {
        return new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/bin/sh").isFile()
                && new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/bin/pkg").isFile();
    }

    private static boolean acceptedExternalBootstrapExists(Activity activity) {
        File inbox = new File(activity.getFilesDir(), "bootstrap-inbox");
        return new File(inbox, "bootstrap-external.zip").isFile()
                && new File(inbox, "bootstrap-external.receipt.json").isFile();
    }

    private static void verifyLoaderIdentity(Activity activity) {
        PackageManager manager = activity.getPackageManager();
        if (manager.checkSignatures(activity.getPackageName(), LOADER_PACKAGE)
                != PackageManager.SIGNATURE_MATCH) {
            throw new SecurityException("LOADER_SIGNATURE_MISMATCH");
        }
        try {
            ApplicationInfo info = manager.getApplicationInfo(
                    LOADER_PACKAGE,
                    PackageManager.GET_META_DATA);
            int version = info.metaData == null ? 0
                    : info.metaData.getInt(META_CONTRACT_VERSION, 0);
            String state = info.metaData == null ? null
                    : info.metaData.getString(META_CONTRACT_STATE);
            if (version < 2 || !"BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE".equals(state)) {
                throw new SecurityException("LOADER_CONTRACT_UNSUPPORTED");
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new SecurityException("LOADER_NOT_INSTALLED", e);
        }
    }

    private static void validateHttpsUrl(String raw) {
        try {
            URL url = new URL(raw);
            int port = url.getPort() == -1 ? url.getDefaultPort() : url.getPort();
            if (!"https".equalsIgnoreCase(url.getProtocol())
                    || url.getHost() == null
                    || url.getHost().trim().isEmpty()
                    || url.getUserInfo() != null
                    || url.getRef() != null
                    || port != 443) {
                throw new IllegalArgumentException("EXTERNAL_BOOTSTRAP_URL_BLOCKED");
            }
        } catch (Exception e) {
            throw new IllegalArgumentException("EXTERNAL_BOOTSTRAP_URL_INVALID", e);
        }
    }

    private static String currentBootstrapAbi() {
        if (Build.SUPPORTED_ABIS.length == 0) {
            throw new IllegalStateException("DEVICE_ABI_UNAVAILABLE");
        }
        switch (Build.SUPPORTED_ABIS[0]) {
            case "arm64-v8a": return "aarch64";
            case "armeabi-v7a": return "arm";
            case "x86": return "i686";
            case "x86_64": return "x86_64";
            default: throw new IllegalStateException("UNSUPPORTED_DEVICE_ABI");
        }
    }

    private static String configuredUrl(String abi) {
        switch (abi) {
            case "aarch64": return BuildConfig.EXTERNAL_BOOTSTRAP_URL_AARCH64;
            case "arm": return BuildConfig.EXTERNAL_BOOTSTRAP_URL_ARM;
            case "i686": return BuildConfig.EXTERNAL_BOOTSTRAP_URL_I686;
            case "x86_64": return BuildConfig.EXTERNAL_BOOTSTRAP_URL_X86_64;
            default: throw new IllegalArgumentException("UNSUPPORTED_ABI");
        }
    }

    private static String configuredSha256(String abi) {
        switch (abi) {
            case "aarch64": return BuildConfig.EXTERNAL_BOOTSTRAP_SHA256_AARCH64;
            case "arm": return BuildConfig.EXTERNAL_BOOTSTRAP_SHA256_ARM;
            case "i686": return BuildConfig.EXTERNAL_BOOTSTRAP_SHA256_I686;
            case "x86_64": return BuildConfig.EXTERNAL_BOOTSTRAP_SHA256_X86_64;
            default: throw new IllegalArgumentException("UNSUPPORTED_ABI");
        }
    }
}
