package com.termux.rafacodephi.loader;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Locale;

/** Bounded acquisition only. Installation remains owned by the host app. */
@SuppressWarnings("deprecation")
public final class BootstrapInstallService extends IntentService {

    private static final String TAG = "BootstrapInstallService";
    private static final int CONNECT_TIMEOUT_MS = 15_000;
    private static final int READ_TIMEOUT_MS = 60_000;
    private static final int BUFFER_SIZE = 65_536;

    public BootstrapInstallService() {
        super("BootstrapInstallService");
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent == null) return;
        String abi = intent.getStringExtra(BootstrapInstallContract.EXTRA_ABI);
        String expectedSha256 = null;
        try {
            if (!BootstrapInstallContract.ACTION_INSTALL_BOOTSTRAP.equals(intent.getAction())) {
                throw new IllegalArgumentException("INVALID_ACTION");
            }
            abi = BootstrapSourcePolicy.requireAbi(abi);
            expectedSha256 = BootstrapSourcePolicy.requireSha256(
                    intent.getStringExtra(BootstrapInstallContract.EXTRA_SHA256));
            URL initialUrl = BootstrapSourcePolicy.requireInitialUrl(
                    intent.getStringExtra(BootstrapInstallContract.EXTRA_SOURCE_URL));

            File verifiedDir = new File(getFilesDir(), "verified");
            if (!verifiedDir.isDirectory() && !verifiedDir.mkdirs()) {
                throw new IOException("VERIFIED_DIR_CREATE_FAILED");
            }
            File target = new File(
                    verifiedDir,
                    "bootstrap-" + abi + "-" + expectedSha256 + ".zip");
            if (!target.isFile() || !BootstrapChecksumValidator.validate(target, expectedSha256)) {
                File part = new File(verifiedDir, target.getName() + ".part");
                if (part.exists() && !part.delete()) {
                    throw new IOException("STALE_PART_DELETE_FAILED");
                }
                long bytes;
                try {
                    bytes = downloadVerified(initialUrl, part, expectedSha256);
                    if (target.exists() && !target.delete()) {
                        throw new IOException("OLD_VERIFIED_DELETE_FAILED");
                    }
                    if (!part.renameTo(target)) {
                        throw new IOException("VERIFIED_ATOMIC_RENAME_FAILED");
                    }
                } catch (Throwable t) {
                    //noinspection ResultOfMethodCallIgnored
                    part.delete();
                    throw t;
                }
                publishSuccess(this, abi, expectedSha256, target, bytes);
            } else {
                publishSuccess(this, abi, expectedSha256, target, target.length());
            }
        } catch (Throwable t) {
            Log.e(TAG, "Bootstrap acquisition failed", t);
            publishFailure(
                    this,
                    abi,
                    expectedSha256,
                    t.getMessage() == null ? t.getClass().getSimpleName() : t.getMessage());
        }
    }

    private long downloadVerified(URL initialUrl, File destination, String expectedSha256)
            throws IOException, NoSuchAlgorithmException {
        URL origin = initialUrl;
        URL current = initialUrl;
        for (int redirects = 0; redirects <= BootstrapSourcePolicy.MAX_REDIRECTS; redirects++) {
            HttpURLConnection connection = (HttpURLConnection) current.openConnection();
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setUseCaches(false);
            connection.setRequestProperty("User-Agent", "RAFCODEPhi-Loader/2");
            try {
                int code = connection.getResponseCode();
                if (isRedirect(code)) {
                    if (redirects == BootstrapSourcePolicy.MAX_REDIRECTS) {
                        throw new IOException("TOO_MANY_REDIRECTS");
                    }
                    current = BootstrapSourcePolicy.requireSameOriginRedirect(
                            origin,
                            current,
                            connection.getHeaderField("Location"));
                    continue;
                }
                if (code < 200 || code >= 300) throw new IOException("HTTP_" + code);
                long contentLength = connection.getContentLength();
                if (contentLength > BootstrapSourcePolicy.MAX_DOWNLOAD_BYTES) {
                    throw new IOException("DOWNLOAD_TOO_LARGE");
                }
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                long total = 0;
                try (InputStream input = new BufferedInputStream(
                        connection.getInputStream(), BUFFER_SIZE);
                     FileOutputStream output = new FileOutputStream(destination)) {
                    byte[] buffer = new byte[BUFFER_SIZE];
                    int read;
                    while ((read = input.read(buffer)) != -1) {
                        total += read;
                        if (total > BootstrapSourcePolicy.MAX_DOWNLOAD_BYTES) {
                            throw new IOException("DOWNLOAD_LIMIT_EXCEEDED");
                        }
                        digest.update(buffer, 0, read);
                        output.write(buffer, 0, read);
                    }
                    output.getFD().sync();
                }
                if (contentLength >= 0 && contentLength != total) {
                    throw new IOException("CONTENT_LENGTH_MISMATCH");
                }
                String observed = toHex(digest.digest());
                if (!expectedSha256.equals(observed)) throw new IOException("SHA256_MISMATCH");
                return total;
            } finally {
                connection.disconnect();
            }
        }
        throw new IOException("REDIRECT_STATE_INVALID");
    }

    private static boolean isRedirect(int code) {
        return code == HttpURLConnection.HTTP_MOVED_PERM
                || code == HttpURLConnection.HTTP_MOVED_TEMP
                || code == HttpURLConnection.HTTP_SEE_OTHER
                || code == 307
                || code == 308;
    }

    private static String toHex(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) result.append(String.format(Locale.US, "%02x", value));
        return result.toString();
    }

    private static void publishSuccess(
            Context context, String abi, String expectedSha256, File file, long bytes) {
        Uri uri = new Uri.Builder()
                .scheme("content")
                .authority(BootstrapInstallContract.PROVIDER_AUTHORITY)
                .appendPath(file.getName())
                .build();
        context.grantUriPermission(
                BootstrapInstallContract.HOST_PACKAGE,
                uri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION);
        Intent result = baseResult(true, "", abi, expectedSha256);
        result.setData(uri);
        result.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        result.putExtra(BootstrapInstallContract.EXTRA_VERIFIED_BYTES, bytes);
        context.sendBroadcast(result, BootstrapInstallContract.HANDOFF_PERMISSION);
        Log.i(TAG, "Verified bootstrap handed to host: abi=" + abi + " bytes=" + bytes);
    }

    static void publishFailure(Context context, String abi, String reason) {
        publishFailure(context, abi, null, reason);
    }

    private static void publishFailure(
            Context context, String abi, String expectedSha256, String reason) {
        context.sendBroadcast(
                baseResult(false,
                        reason == null ? "UNKNOWN_FAILURE" : reason,
                        abi,
                        expectedSha256),
                BootstrapInstallContract.HANDOFF_PERMISSION);
    }

    private static Intent baseResult(
            boolean success, String reason, String abi, String expectedSha256) {
        Intent result = new Intent(BootstrapInstallContract.ACTION_BOOTSTRAP_VERIFIED);
        result.setPackage(BootstrapInstallContract.HOST_PACKAGE);
        result.putExtra(BootstrapInstallContract.EXTRA_SUCCESS, success);
        result.putExtra(BootstrapInstallContract.EXTRA_FAILURE_REASON, reason);
        result.putExtra(BootstrapInstallContract.EXTRA_VERIFIED_ABI, abi == null ? "" : abi);
        if (expectedSha256 != null) {
            result.putExtra(BootstrapInstallContract.EXTRA_SHA256, expectedSha256);
        }
        return result;
    }
}
