package com.termux.app.benchmark;

import android.content.Context;
import android.os.Build;
import android.os.Process;
import android.util.AtomicFile;

import com.termux.rafacodephi.BuildConfig;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * Append-safe evidence boundary for the PA freestanding ELF device execution.
 *
 * The receipt deliberately proves only what the launcher directly observes:
 * artifact identity, linker route, exit code and exact stdout markers. It does
 * not promote those observations into claims about isolated silicon behavior,
 * timer accuracy, thermal stability or statistical reproducibility.
 */
public final class PaBenchmarkReceipt {

    public static final String SCHEMA = "rafcodephi.pa-elf-runtime-receipt/v1";
    public static final String DIRECTORY = "rafaelia/receipts";
    public static final String FILE_NAME = "pa_freestanding_elf_runtime_v1.json";

    private static final String HEADER = "RAFCODEPHI-PA-ELF 00000001";
    private static final String MODE = "MODE FREESTANDING NO_LIBC NO_MALLOC NO_JNI DIRECT_SYSCALL";
    private static final String END = "END 00000000";

    private PaBenchmarkReceipt() {}

    public static File getReceiptFile(Context context) {
        File directory = new File(context.getFilesDir(), DIRECTORY);
        return new File(directory, FILE_NAME);
    }

    public static File recordExecution(Context context,
                                       File elf,
                                       String linker,
                                       int exitCode,
                                       String stdout,
                                       Throwable executionError) throws Exception {
        JSONObject receipt = new JSONObject();
        receipt.put("schema", SCHEMA);
        receipt.put("generated_at_utc", utcNow());
        receipt.put("package_name", context.getPackageName());
        receipt.put("app_version", BuildConfig.VERSION_NAME);
        receipt.put("android_release", Build.VERSION.RELEASE);
        receipt.put("android_api", Build.VERSION.SDK_INT);
        receipt.put("manufacturer", Build.MANUFACTURER);
        receipt.put("model", Build.MODEL);
        receipt.put("process_is_64_bit", Build.VERSION.SDK_INT >= 23 && Process.is64Bit());

        JSONArray abis = new JSONArray();
        if (Build.SUPPORTED_ABIS != null) {
            for (String abi : Build.SUPPORTED_ABIS) abis.put(abi);
        }
        receipt.put("supported_abis", abis);

        receipt.put("linker", linker == null ? "" : linker);
        receipt.put("elf_path", elf == null ? "" : elf.getAbsolutePath());
        receipt.put("elf_sha256", elf != null && elf.isFile() ? sha256File(elf) : "");
        receipt.put("exit_code", exitCode);

        String captured = stdout == null ? "" : stdout;
        receipt.put("stdout_sha256", sha256Bytes(captured.getBytes(StandardCharsets.US_ASCII)));
        receipt.put("stdout_bytes", captured.getBytes(StandardCharsets.US_ASCII).length);
        receipt.put("stdout", captured);

        JSONObject markers = new JSONObject();
        markers.put("header", captured.contains(HEADER));
        markers.put("mode_contract_marker", captured.contains(MODE));
        boolean allRuns = true;
        for (int i = 0; i < 6; i++) {
            boolean present = captured.contains("R" + i + " ");
            markers.put("r" + i, present);
            allRuns &= present;
        }
        markers.put("end", captured.contains(END));
        receipt.put("markers", markers);

        boolean markerComplete = markers.getBoolean("header")
            && markers.getBoolean("mode_contract_marker")
            && allRuns
            && markers.getBoolean("end");
        boolean runtimePass = executionError == null && exitCode == 0 && markerComplete;
        receipt.put("runtime_exec_pass", runtimePass);
        receipt.put("evidence_scope", "physical_process_execution_observation");
        receipt.put("claim_boundary",
            "Does not by itself prove isolated silicon performance, timer accuracy, thermal stability, " +
            "cross-device comparability, statistical reproducibility, or standards certification.");

        if (executionError != null) {
            JSONObject error = new JSONObject();
            error.put("class", executionError.getClass().getName());
            error.put("message", String.valueOf(executionError.getMessage()));
            receipt.put("execution_error", error);
        }

        return writeAtomic(context, receipt);
    }

    public static JSONObject read(Context context) {
        File file = getReceiptFile(context);
        if (!file.isFile()) return null;
        try (FileInputStream input = new FileInputStream(file)) {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream((int) Math.min(file.length(), 128 * 1024));
            byte[] buffer = new byte[4096];
            int read;
            int total = 0;
            while ((read = input.read(buffer)) >= 0) {
                if (read == 0) continue;
                total += read;
                if (total > 128 * 1024) return null;
                bytes.write(buffer, 0, read);
            }
            return new JSONObject(new String(bytes.toByteArray(), StandardCharsets.UTF_8));
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static File writeAtomic(Context context, JSONObject receipt) throws IOException {
        File file = getReceiptFile(context);
        File parent = file.getParentFile();
        if (parent == null || (!parent.exists() && !parent.mkdirs() && !parent.exists())) {
            throw new IOException("Unable to create receipt directory: " + parent);
        }

        AtomicFile atomicFile = new AtomicFile(file);
        FileOutputStream output = null;
        try {
            output = atomicFile.startWrite();
            byte[] bytes = (receipt.toString() + "\n").getBytes(StandardCharsets.UTF_8);
            output.write(bytes);
            output.flush();
            output.getFD().sync();
            atomicFile.finishWrite(output);
            return file;
        } catch (IOException error) {
            if (output != null) atomicFile.failWrite(output);
            throw error;
        }
    }

    private static String sha256File(File file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        try (FileInputStream input = new FileInputStream(file)) {
            byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read > 0) digest.update(buffer, 0, read);
            }
        }
        return hex(digest.digest());
    }

    private static String sha256Bytes(byte[] bytes) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        return hex(digest.digest(bytes));
    }

    private static String hex(byte[] bytes) {
        StringBuilder sb = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) sb.append(String.format(Locale.US, "%02x", value & 0xff));
        return sb.toString();
    }

    private static String utcNow() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(new Date());
    }
}
