package com.termux.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.system.Os;

import com.termux.shared.logger.Logger;

import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Signature-protected host custody for a verified loader URI. */
public final class BootstrapHandoffReceiver extends BroadcastReceiver {

    private static final String LOG_TAG = "BootstrapHandoff";
    private static final String ACTION_BOOTSTRAP_VERIFIED =
            "com.termux.rafacodephi.BOOTSTRAP_VERIFIED";
    private static final String PROVIDER_AUTHORITY =
            "com.termux.rafacodephi.loader.bootstrap";
    private static final String EXTRA_SUCCESS = "success";
    private static final String EXTRA_FAILURE_REASON = "failure_reason";
    private static final String EXTRA_VERIFIED_ABI = "verified_abi";
    private static final String EXTRA_EXPECTED_SHA256 = "expected_sha256";
    private static final String EXTRA_VERIFIED_BYTES = "verified_bytes";
    private static final long MAX_BOOTSTRAP_BYTES = 128L * 1024L * 1024L;
    private static final long MAX_UNCOMPRESSED_BYTES = 768L * 1024L * 1024L;
    private static final long MAX_ENTRY_BYTES = 256L * 1024L * 1024L;
    private static final int MAX_ZIP_ENTRIES = 65_536;
    private static final long MAX_COMPRESSION_RATIO = 500L;
    private static final Pattern SHA256 = Pattern.compile("^[0-9a-f]{64}$");

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null || !ACTION_BOOTSTRAP_VERIFIED.equals(intent.getAction())) {
            Logger.logError(LOG_TAG, "Rejected handoff with invalid action");
            return;
        }
        if (!intent.getBooleanExtra(EXTRA_SUCCESS, false)) {
            Logger.logError(LOG_TAG, "Loader acquisition failed: "
                    + intent.getStringExtra(EXTRA_FAILURE_REASON));
            return;
        }
        final PendingResult pending = goAsync();
        final Intent received = new Intent(intent);
        final Context appContext = context.getApplicationContext();
        new Thread(() -> {
            try {
                accept(appContext, received);
            } catch (Throwable t) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Bootstrap handoff rejected", t);
            } finally {
                Uri uri = received.getData();
                if (uri != null) {
                    appContext.revokeUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                }
                pending.finish();
            }
        }, "BootstrapHandoffReceiver").start();
    }

    private static void accept(Context context, Intent intent) throws Exception {
        Uri uri = intent.getData();
        if (uri == null
                || !"content".equals(uri.getScheme())
                || !PROVIDER_AUTHORITY.equals(uri.getAuthority())) {
            throw new SecurityException("INVALID_BOOTSTRAP_URI");
        }

        String expectedAbi = expectedAbiForDevice();
        String observedAbi = intent.getStringExtra(EXTRA_VERIFIED_ABI);
        if (!expectedAbi.equals(observedAbi)) {
            throw new SecurityException("ABI_MISMATCH expected=" + expectedAbi
                    + " observed=" + observedAbi);
        }
        String expectedSha256 = normalizeSha256(
                intent.getStringExtra(EXTRA_EXPECTED_SHA256));
        long declaredBytes = intent.getLongExtra(EXTRA_VERIFIED_BYTES, -1L);
        if (declaredBytes < 1 || declaredBytes > MAX_BOOTSTRAP_BYTES) {
            throw new SecurityException("INVALID_DECLARED_SIZE");
        }

        File inbox = new File(context.getFilesDir(), "bootstrap-inbox");
        if (!inbox.isDirectory() && !inbox.mkdirs()) {
            throw new IOException("INBOX_CREATE_FAILED");
        }
        Os.chmod(inbox.getAbsolutePath(), 0700);
        File part = new File(inbox, "bootstrap-external.zip.part");
        File accepted = new File(inbox, "bootstrap-external.zip");
        File receiptPart = new File(inbox, "bootstrap-external.receipt.json.part");
        File receipt = new File(inbox, "bootstrap-external.receipt.json");
        deleteQuietly(part);
        deleteQuietly(receiptPart);

        MessageDigest sha256 = MessageDigest.getInstance("SHA-256");
        long copied = 0;
        try (InputStream raw = context.getContentResolver().openInputStream(uri);
             BufferedInputStream input = raw == null ? null : new BufferedInputStream(raw, 65_536);
             FileOutputStream output = new FileOutputStream(part)) {
            if (input == null) throw new IOException("URI_OPEN_FAILED");
            byte[] buffer = new byte[65_536];
            int read;
            while ((read = input.read(buffer)) != -1) {
                copied += read;
                if (copied > MAX_BOOTSTRAP_BYTES) {
                    throw new IOException("BOOTSTRAP_SIZE_LIMIT_EXCEEDED");
                }
                sha256.update(buffer, 0, read);
                output.write(buffer, 0, read);
            }
            output.getFD().sync();
        } catch (Throwable t) {
            deleteQuietly(part);
            throw t;
        }
        if (copied != declaredBytes) {
            deleteQuietly(part);
            throw new SecurityException("DECLARED_SIZE_MISMATCH");
        }
        String observedSha256 = toHex(sha256.digest());
        if (!expectedSha256.equals(observedSha256)) {
            deleteQuietly(part);
            throw new SecurityException("HOST_SHA256_MISMATCH");
        }

        String expectedBlake3 = BootstrapIntegrityVerifier.expectedHashForCurrentAbi();
        if (!SHA256.matcher(expectedBlake3.toLowerCase(Locale.US)).matches()) {
            deleteQuietly(part);
            throw new SecurityException("CANONICAL_BLAKE3_NOT_CONFIGURED");
        }
        String observedBlake3 = BootstrapIntegrityVerifier.blake3Hex(
                part,
                MAX_BOOTSTRAP_BYTES);
        if (!expectedBlake3.equalsIgnoreCase(observedBlake3)) {
            deleteQuietly(part);
            throw new SecurityException("HOST_BLAKE3_MISMATCH");
        }
        validateZipArchive(part);

        Os.chmod(part.getAbsolutePath(), 0600);
        JSONObject receiptJson = new JSONObject();
        receiptJson.put("schema", "termux.rafacodephi.bootstrap_handoff_receipt.v1");
        receiptJson.put("state", "HOST_ACCEPTED_CANONICAL_BOOTSTRAP");
        receiptJson.put("abi", expectedAbi);
        receiptJson.put("sha256", observedSha256);
        receiptJson.put("blake3", observedBlake3);
        receiptJson.put("bytes", copied);
        receiptJson.put("provider_authority", PROVIDER_AUTHORITY);
        receiptJson.put("zip_policy", "bounded-v1");
        receiptJson.put("claim_allowed", false);
        receiptJson.put("observed_at_epoch_ms", System.currentTimeMillis());
        writeJsonPart(receiptPart, receiptJson.toString());

        try {
            Os.rename(part.getAbsolutePath(), accepted.getAbsolutePath());
            Os.chmod(accepted.getAbsolutePath(), 0600);
            Os.rename(receiptPart.getAbsolutePath(), receipt.getAbsolutePath());
            Os.chmod(receipt.getAbsolutePath(), 0600);
        } catch (Throwable t) {
            deleteQuietly(part);
            deleteQuietly(receiptPart);
            deleteQuietly(accepted);
            throw t;
        }

        Logger.logInfo(LOG_TAG, "Canonical bootstrap accepted in private inbox: abi="
                + expectedAbi + " bytes=" + copied);
        Intent launch = new Intent(context, TermuxActivity.class);
        launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(launch);
    }

    private static void validateZipArchive(File file) throws IOException {
        int entries = 0;
        long totalUncompressed = 0;
        boolean symlinksManifest = false;
        Set<String> names = new HashSet<>();
        try (ZipFile archive = new ZipFile(file)) {
            Enumeration<? extends ZipEntry> iterator = archive.entries();
            while (iterator.hasMoreElements()) {
                ZipEntry entry = iterator.nextElement();
                entries++;
                if (entries > MAX_ZIP_ENTRIES) throw new IOException("ZIP_ENTRY_LIMIT_EXCEEDED");
                String name = entry.getName();
                validateZipName(name);
                if (!names.add(name)) throw new IOException("ZIP_DUPLICATE_ENTRY");
                if ("SYMLINKS.txt".equals(name)) symlinksManifest = true;

                long size = entry.getSize();
                long compressed = entry.getCompressedSize();
                if (size < 0 || compressed < 0) throw new IOException("ZIP_SIZE_UNKNOWN");
                if (size > MAX_ENTRY_BYTES) throw new IOException("ZIP_ENTRY_TOO_LARGE");
                if (Long.MAX_VALUE - totalUncompressed < size) {
                    throw new IOException("ZIP_SIZE_OVERFLOW");
                }
                totalUncompressed += size;
                if (totalUncompressed > MAX_UNCOMPRESSED_BYTES) {
                    throw new IOException("ZIP_UNCOMPRESSED_LIMIT_EXCEEDED");
                }
                if (size > 0 && compressed == 0) {
                    throw new IOException("ZIP_ZERO_COMPRESSED_SIZE");
                }
                if (compressed > 0 && size / compressed > MAX_COMPRESSION_RATIO) {
                    throw new IOException("ZIP_COMPRESSION_RATIO_EXCEEDED");
                }
            }
        }
        if (entries == 0) throw new IOException("ZIP_EMPTY");
        if (!symlinksManifest) throw new IOException("ZIP_SYMLINKS_MANIFEST_MISSING");
    }

    private static void validateZipName(String name) throws IOException {
        if (name == null || name.isEmpty() || name.length() > 4096
                || name.startsWith("/") || name.startsWith("\\")
                || name.indexOf('\0') >= 0 || name.indexOf('\\') >= 0) {
            throw new IOException("ZIP_ENTRY_NAME_INVALID");
        }
        String[] parts = name.split("/");
        for (String part : parts) {
            if ("..".equals(part)) throw new IOException("ZIP_TRAVERSAL_ENTRY");
        }
    }

    private static void writeJsonPart(File part, String json) throws Exception {
        try (FileOutputStream output = new FileOutputStream(part)) {
            output.write((json + "\n").getBytes("UTF-8"));
            output.getFD().sync();
        }
        Os.chmod(part.getAbsolutePath(), 0600);
    }

    private static String expectedAbiForDevice() {
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

    private static String normalizeSha256(String value) {
        if (value == null) throw new IllegalArgumentException("SHA256_MISSING");
        String normalized = value.toLowerCase(Locale.US);
        if (!SHA256.matcher(normalized).matches()) {
            throw new IllegalArgumentException("SHA256_INVALID");
        }
        return normalized;
    }

    private static String toHex(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) result.append(String.format(Locale.US, "%02x", value));
        return result.toString();
    }

    private static void deleteQuietly(File file) {
        if (file.exists() && !file.delete()) {
            Logger.logWarn(LOG_TAG, "Could not delete temporary file: " + file);
        }
    }
}
