package com.termux.app;

import android.content.Context;
import android.net.Uri;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxRuntimePaths;

import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** Fail-closed bootstrap.zip source selected by the installation wizard. */
public final class BootstrapWizardSource {

    public static final String FILE_NAME = "bootstrap-external.zip";
    public static final String RECEIPT_NAME = "bootstrap-external.receipt.json";
    public static final String RECEIPT_SCHEMA = "termux.rafacodephi.bootstrap_handoff_receipt.v1";
    public static final String RECEIPT_STATE = "HOST_ACCEPTED_CANONICAL_BOOTSTRAP";

    private static final String LOG_TAG = "BootstrapWizardSource";
    private static final long MAX_BOOTSTRAP_BYTES = 256L * 1024L * 1024L;
    private static final int MAX_PROFILE_BYTES = 64 * 1024;
    private static final int MAX_SYMLINKS_BYTES = 1024 * 1024;
    private static final Pattern HASH_256 = Pattern.compile("^[0-9a-f]{64}$");

    private BootstrapWizardSource() {}

    @NonNull
    public static File inbox(@NonNull Context context) {
        return new File(context.getFilesDir(), "bootstrap-inbox");
    }

    @NonNull
    public static File zipFile(@NonNull Context context) {
        return new File(inbox(context), FILE_NAME);
    }

    @NonNull
    public static File receiptFile(@NonNull Context context) {
        return new File(inbox(context), RECEIPT_NAME);
    }

    public static void clear(@NonNull Context context) {
        File zip = zipFile(context);
        File receipt = receiptFile(context);
        if (zip.exists() && !zip.delete()) Logger.logWarn(LOG_TAG, "Could not delete " + zip);
        if (receipt.exists() && !receipt.delete()) Logger.logWarn(LOG_TAG, "Could not delete " + receipt);
    }

    /** Copy, validate and atomically accept a user-selected bootstrap.zip. */
    @NonNull
    public static JSONObject accept(@NonNull Context context, @NonNull Uri uri) throws Exception {
        TermuxRuntimePaths.init(context);
        File dir = inbox(context);
        if (!dir.exists() && !dir.mkdirs() && !dir.isDirectory()) {
            throw new IllegalStateException("BOOTSTRAP_INBOX_CREATE_FAILED: " + dir);
        }
        File tmp = new File(dir, FILE_NAME + ".tmp");
        if (tmp.exists() && !tmp.delete()) throw new IllegalStateException("BOOTSTRAP_TMP_DELETE_FAILED");

        long total = 0L;
        try (InputStream raw = context.getContentResolver().openInputStream(uri)) {
            if (raw == null) throw new IllegalStateException("BOOTSTRAP_DOCUMENT_OPEN_FAILED");
            try (BufferedInputStream input = new BufferedInputStream(raw, 65_536);
                 FileOutputStream output = new FileOutputStream(tmp)) {
                byte[] buffer = new byte[65_536];
                int read;
                while ((read = input.read(buffer)) != -1) {
                    total += read;
                    if (total > MAX_BOOTSTRAP_BYTES) throw new IllegalArgumentException("BOOTSTRAP_TOO_LARGE");
                    output.write(buffer, 0, read);
                }
                output.flush();
                output.getFD().sync();
            }
        } catch (Throwable t) {
            if (tmp.exists()) tmp.delete();
            throw t;
        }
        if (total < 1) {
            tmp.delete();
            throw new IllegalArgumentException("BOOTSTRAP_EMPTY");
        }

        Validation validation = validateZipContract(tmp, TermuxRuntimePaths.isRelocatedLayout());
        String expected = BootstrapIntegrityVerifier.expectedHashForCurrentAbi().toLowerCase(Locale.US);
        if (!HASH_256.matcher(expected).matches()) {
            tmp.delete();
            throw new IllegalStateException("EXTERNAL_BOOTSTRAP_CANONICAL_BLAKE3_UNAVAILABLE");
        }
        String actual = BootstrapIntegrityVerifier.blake3Hex(tmp, MAX_BOOTSTRAP_BYTES).toLowerCase(Locale.US);
        if (!expected.equals(actual)) {
            tmp.delete();
            throw new SecurityException("BOOTSTRAP_BLAKE3_MISMATCH expected=" + expected + " actual=" + actual);
        }

        File target = zipFile(context);
        if (target.exists() && !target.delete()) {
            tmp.delete();
            throw new IllegalStateException("BOOTSTRAP_TARGET_REPLACE_FAILED");
        }
        if (!tmp.renameTo(target)) {
            tmp.delete();
            throw new IllegalStateException("BOOTSTRAP_ATOMIC_RENAME_FAILED");
        }

        JSONObject receipt = new JSONObject();
        receipt.put("schema", RECEIPT_SCHEMA);
        receipt.put("state", RECEIPT_STATE);
        receipt.put("source", "WIZARD_DOCUMENT");
        receipt.put("abi", currentBootstrapAbi());
        receipt.put("blake3", actual);
        receipt.put("bytes", target.length());
        receipt.put("runtime_layout", TermuxRuntimePaths.layoutState());
        receipt.put("runtime_files_dir", TermuxRuntimePaths.filesDirPath());
        receipt.put("runtime_prefix", TermuxRuntimePaths.prefixDirPath());
        receipt.put("bootstrap_profile", validation.profile);
        receipt.put("bootstrap_package_layer", validation.packageLayer);
        receipt.put("relocatable_bridge_allowed", validation.relocatableBridgeAllowed);
        receipt.put("claim_allowed", false);
        writeReceiptAtomic(receiptFile(context), receipt);
        return receipt;
    }

    @Nullable
    public static JSONObject readAcceptedReceipt(@NonNull Context context) {
        try {
            File receipt = receiptFile(context);
            if (!receipt.isFile() || receipt.length() < 1 || receipt.length() > 64 * 1024) return null;
            return new JSONObject(readBounded(receipt, 64 * 1024));
        } catch (Throwable t) {
            return null;
        }
    }

    /** Revalidates receipt + hash before installer consumes the selected file. */
    @Nullable
    public static byte[] loadAcceptedBytes(@NonNull Context context) throws Exception {
        TermuxRuntimePaths.init(context);
        File zip = zipFile(context);
        JSONObject receipt = readAcceptedReceipt(context);
        if (!zip.isFile() || receipt == null) return null;
        String expected = BootstrapIntegrityVerifier.expectedHashForCurrentAbi().toLowerCase(Locale.US);
        String recorded = receipt.optString("blake3", "").toLowerCase(Locale.US);
        boolean receiptValid = RECEIPT_SCHEMA.equals(receipt.optString("schema"))
            && RECEIPT_STATE.equals(receipt.optString("state"))
            && currentBootstrapAbi().equals(receipt.optString("abi"))
            && HASH_256.matcher(expected).matches()
            && expected.equals(recorded)
            && receipt.optLong("bytes", -1L) == zip.length()
            && !receipt.optBoolean("claim_allowed", true);
        if (!receiptValid) {
            clear(context);
            throw new SecurityException("BOOTSTRAP_WIZARD_RECEIPT_INVALIDATED");
        }
        String actual = BootstrapIntegrityVerifier.blake3Hex(zip, MAX_BOOTSTRAP_BYTES).toLowerCase(Locale.US);
        if (!expected.equals(actual)) {
            clear(context);
            throw new SecurityException("BOOTSTRAP_WIZARD_HASH_INVALIDATED");
        }
        validateZipContract(zip, TermuxRuntimePaths.isRelocatedLayout());
        try (FileInputStream input = new FileInputStream(zip);
             ByteArrayOutputStream output = new ByteArrayOutputStream((int) Math.min(zip.length(), 8 * 1024 * 1024))) {
            byte[] buffer = new byte[65_536];
            int read;
            long total = 0L;
            while ((read = input.read(buffer)) != -1) {
                total += read;
                if (total > MAX_BOOTSTRAP_BYTES) throw new IllegalArgumentException("BOOTSTRAP_TOO_LARGE");
                output.write(buffer, 0, read);
            }
            return output.toByteArray();
        }
    }

    @NonNull
    public static String status(@NonNull Context context) {
        JSONObject receipt = readAcceptedReceipt(context);
        if (receipt == null || !zipFile(context).isFile()) return "NOT_SELECTED";
        return receipt.optString("state", "INVALIDATED") + " / "
            + receipt.optString("bootstrap_profile", "UNKNOWN") + " / "
            + receipt.optString("runtime_layout", "UNKNOWN");
    }

    private static Validation validateZipContract(File zip, boolean relocated) throws Exception {
        boolean symlinks = false;
        boolean shDirect = false;
        boolean pkgDirect = false;
        boolean busyboxDirect = false;
        boolean prootDirect = false;
        String symlinkText = null;
        String profileJson = null;
        try (ZipInputStream input = new ZipInputStream(new BufferedInputStream(new FileInputStream(zip), 65_536))) {
            ZipEntry entry;
            while ((entry = input.getNextEntry()) != null) {
                String name = entry.getName();
                if (name.startsWith("/") || name.contains("../") || name.equals("..") || name.contains("\\")) {
                    throw new SecurityException("BOOTSTRAP_ZIP_TRAVERSAL: " + name);
                }
                if ("SYMLINKS.txt".equals(name)) {
                    symlinks = true;
                    symlinkText = readZipEntryBounded(input, MAX_SYMLINKS_BYTES, "BOOTSTRAP_SYMLINKS_TOO_LARGE");
                }
                if ("bin/sh".equals(name)) shDirect = true;
                if ("bin/pkg".equals(name)) pkgDirect = true;
                if ("bin/busybox".equals(name)) busyboxDirect = true;
                if ("bin/proot".equals(name)) prootDirect = true;
                if ("BOOTSTRAP_PROFILE.json".equals(name)) {
                    profileJson = readZipEntryBounded(input, MAX_PROFILE_BYTES, "BOOTSTRAP_PROFILE_TOO_LARGE");
                }
            }
        }

        Set<String> symlinkDestinations = parseSymlinkDestinations(symlinkText);
        boolean sh = shDirect || symlinkDestinations.contains("bin/sh");
        boolean pkg = pkgDirect || symlinkDestinations.contains("bin/pkg");
        boolean busybox = busyboxDirect || symlinkDestinations.contains("bin/busybox");
        boolean proot = prootDirect || symlinkDestinations.contains("bin/proot");
        // The accepted archive must be able to start a real shell and expose
        // pkg recovery. Busybox/proot belong to the later full package-runtime
        // gate and cannot block first-boot recovery.
        if (!symlinks || !sh || !pkg) {
            throw new IllegalArgumentException("BOOTSTRAP_REQUIRED_INSTALLED_ENTRIES_MISSING symlinks=" + symlinks
                + " sh=" + sh + " pkg=" + pkg + " busybox_observed=" + busybox + " proot_observed=" + proot);
        }

        String profile = "MISSING";
        String packageLayer = "UNKNOWN";
        boolean relocationAllowed = false;
        if (profileJson != null) {
            JSONObject data = new JSONObject(profileJson);
            profile = data.optString("profile", "UNKNOWN");
            packageLayer = data.optString("package_layer", "UNKNOWN");
            boolean claimAllowed = data.optBoolean("claim_allowed", true);
            relocationAllowed = "bridge".equalsIgnoreCase(profile)
                && "bridge".equalsIgnoreCase(packageLayer)
                && !claimAllowed;
        }
        if (relocated && !relocationAllowed) {
            throw new IllegalStateException("RELOCATED_RUNTIME_BLOCKED_FOR_NON_RELOCATABLE_BOOTSTRAP profile="
                + profile + " package_layer=" + packageLayer);
        }
        return new Validation(profile, packageLayer, relocationAllowed);
    }

    private static String readZipEntryBounded(ZipInputStream input, int limit, String overflowReason) throws Exception {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        int read;
        int total = 0;
        while ((read = input.read(buffer)) != -1) {
            total += read;
            if (total > limit) throw new IllegalArgumentException(overflowReason);
            output.write(buffer, 0, read);
        }
        return new String(output.toByteArray(), StandardCharsets.UTF_8);
    }

    private static Set<String> parseSymlinkDestinations(@Nullable String text) {
        Set<String> destinations = new HashSet<>();
        if (text == null || text.isEmpty()) return destinations;
        int number = 0;
        for (String raw : text.split("\\r?\\n")) {
            number++;
            if (raw.isEmpty()) continue;
            String[] parts = raw.split("←", -1);
            if (parts.length != 2 || parts[0].isEmpty() || parts[1].isEmpty()) {
                throw new IllegalArgumentException("MALFORMED_BOOTSTRAP_SYMLINK_LINE_" + number);
            }
            String target = parts[0];
            String link = parts[1];
            if (link.startsWith("/") || link.contains("..") || link.contains("\\")) {
                throw new SecurityException("UNSAFE_BOOTSTRAP_SYMLINK_DESTINATION_" + number + ":" + link);
            }
            if (!destinations.add(link)) {
                throw new IllegalArgumentException("DUPLICATE_BOOTSTRAP_SYMLINK_DESTINATION_" + number + ":" + link);
            }
            if (target.contains("/data/data/com.termux/files/usr")) {
                throw new SecurityException("LEGACY_BOOTSTRAP_SYMLINK_TARGET_" + number);
            }
        }
        return destinations;
    }

    private static String currentBootstrapAbi() {
        if (Build.SUPPORTED_ABIS.length == 0) return "unknown";
        String abi = Build.SUPPORTED_ABIS[0];
        if ("arm64-v8a".equals(abi)) return "aarch64";
        if ("armeabi-v7a".equals(abi)) return "arm";
        if ("x86".equals(abi)) return "i686";
        if ("x86_64".equals(abi)) return "x86_64";
        return abi;
    }

    private static String readBounded(File file, int limit) throws Exception {
        try (FileInputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int total = 0;
            int read;
            while ((read = input.read(buffer)) != -1) {
                total += read;
                if (total > limit) throw new IllegalArgumentException("FILE_TOO_LARGE");
                output.write(buffer, 0, read);
            }
            return new String(output.toByteArray(), StandardCharsets.UTF_8);
        }
    }

    private static void writeReceiptAtomic(File file, JSONObject receipt) throws Exception {
        File tmp = new File(file.getParentFile(), file.getName() + ".tmp");
        try (FileOutputStream output = new FileOutputStream(tmp)) {
            output.write(receipt.toString(2).getBytes(StandardCharsets.UTF_8));
            output.write('\n');
            output.flush();
            output.getFD().sync();
        }
        if (file.exists() && !file.delete()) throw new IllegalStateException("RECEIPT_REPLACE_FAILED");
        if (!tmp.renameTo(file)) throw new IllegalStateException("RECEIPT_ATOMIC_RENAME_FAILED");
    }

    private static final class Validation {
        final String profile;
        final String packageLayer;
        final boolean relocatableBridgeAllowed;

        Validation(String profile, String packageLayer, boolean relocatableBridgeAllowed) {
            this.profile = profile;
            this.packageLayer = packageLayer;
            this.relocatableBridgeAllowed = relocatableBridgeAllowed;
        }
    }
}
