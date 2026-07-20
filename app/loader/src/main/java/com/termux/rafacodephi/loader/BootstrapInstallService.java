package com.termux.rafacodephi.loader;

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Downloads, verifies, and extracts a bootstrap ZIP to the target directory.
 *
 * Dispatch flow:
 *   1. Download bootstrap ZIP from sourceUrl to a temp file
 *   2. Verify SHA-256 against expectedSha256
 *   3. Extract ZIP entries to targetDir (rejects path traversal)
 *   4. Broadcast ACTION_INSTALL_RESULT
 *
 * ARM32/ARM64: uses only java.net.HttpURLConnection and java.util.zip — no NDK.
 */
@SuppressWarnings("deprecation")
public class BootstrapInstallService extends IntentService {

    private static final String TAG = "BootstrapInstallService";
    private static final int CONNECT_TIMEOUT_MS = 15_000;
    private static final int READ_TIMEOUT_MS    = 60_000;
    private static final int BUFFER_SIZE        = 65536;

    public BootstrapInstallService() {
        super("BootstrapInstallService");
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent == null) return;

        String abi       = intent.getStringExtra(BootstrapInstallContract.EXTRA_ABI);
        String sha256    = intent.getStringExtra(BootstrapInstallContract.EXTRA_SHA256);
        String sourceUrl = intent.getStringExtra(BootstrapInstallContract.EXTRA_SOURCE_URL);
        String targetDir = intent.getStringExtra(BootstrapInstallContract.EXTRA_TARGET_DIR);

        if (abi == null || sha256 == null || sourceUrl == null || targetDir == null) {
            broadcastResult(false, "MISSING_EXTRAS", abi);
            return;
        }

        File target = new File(targetDir);
        if (!target.isDirectory() && !target.mkdirs()) {
            broadcastResult(false, "TARGET_DIR_CREATE_FAILED", abi);
            return;
        }

        File tmpZip = new File(getCacheDir(), "bootstrap-" + abi + ".zip.tmp");
        try {
            Log.i(TAG, "Downloading " + sourceUrl);
            download(sourceUrl, tmpZip);

            Log.i(TAG, "Verifying SHA-256");
            if (!BootstrapChecksumValidator.validate(tmpZip, sha256)) {
                broadcastResult(false, "SHA256_MISMATCH", abi);
                return;
            }

            Log.i(TAG, "Extracting to " + targetDir);
            extractZip(tmpZip, target);

            broadcastResult(true, "", abi);
        } catch (Exception e) {
            Log.e(TAG, "Bootstrap install failed", e);
            broadcastResult(false, e.getClass().getSimpleName() + ": " + e.getMessage(), abi);
        } finally {
            //noinspection ResultOfMethodCallIgnored
            tmpZip.delete();
        }
    }

    private void download(String urlString, File dest) throws IOException {
        URL url = new URL(urlString);
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();
        conn.setConnectTimeout(CONNECT_TIMEOUT_MS);
        conn.setReadTimeout(READ_TIMEOUT_MS);
        conn.setRequestProperty("User-Agent", "RAFCODEΦ-Loader/1");
        conn.connect();

        int code = conn.getResponseCode();
        if (code < 200 || code >= 300) {
            throw new IOException("HTTP " + code + " from " + urlString);
        }

        try (InputStream in = new BufferedInputStream(conn.getInputStream(), BUFFER_SIZE);
             FileOutputStream out = new FileOutputStream(dest)) {
            byte[] buf = new byte[BUFFER_SIZE];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        } finally {
            conn.disconnect();
        }
    }

    private void extractZip(File zipFile, File targetDir) throws IOException {
        String targetCanonical = targetDir.getCanonicalPath();
        try (ZipInputStream zis = new ZipInputStream(
                new BufferedInputStream(new java.io.FileInputStream(zipFile), BUFFER_SIZE))) {
            ZipEntry entry;
            byte[] buf = new byte[BUFFER_SIZE];
            while ((entry = zis.getNextEntry()) != null) {
                String name = entry.getName();
                /* Reject path traversal */
                if (name.contains("..") || name.startsWith("/")) {
                    Log.w(TAG, "Skipping unsafe entry: " + name);
                    zis.closeEntry();
                    continue;
                }
                File out = new File(targetDir, name);
                /* Canonical-path check */
                if (!out.getCanonicalPath().startsWith(targetCanonical + File.separator)
                        && !out.getCanonicalPath().equals(targetCanonical)) {
                    Log.w(TAG, "Skipping traversal attempt: " + name);
                    zis.closeEntry();
                    continue;
                }
                if (entry.isDirectory()) {
                    if (!out.isDirectory() && !out.mkdirs()) {
                        throw new IOException("Failed to create directory: " + out);
                    }
                } else {
                    File parent = out.getParentFile();
                    if (parent != null && !parent.isDirectory()) parent.mkdirs();
                    try (FileOutputStream fos = new FileOutputStream(out)) {
                        int n;
                        while ((n = zis.read(buf)) > 0) fos.write(buf, 0, n);
                    }
                }
                zis.closeEntry();
            }
        }
    }

    private void broadcastResult(boolean success, String reason, String abi) {
        Intent result = new Intent(BootstrapInstallContract.ACTION_INSTALL_RESULT);
        result.putExtra(BootstrapInstallContract.EXTRA_SUCCESS, success);
        result.putExtra(BootstrapInstallContract.EXTRA_FAILURE_REASON, reason != null ? reason : "");
        result.putExtra(BootstrapInstallContract.EXTRA_INSTALLED_ABI, abi != null ? abi : "");
        sendBroadcast(result);
        Log.i(TAG, "Result: success=" + success + " abi=" + abi + " reason=" + reason);
    }
}
