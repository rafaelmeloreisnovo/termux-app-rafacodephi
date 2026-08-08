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
 * Fail-closed evidence boundary for the PA freestanding ELF device execution.
 *
 * The latest receipt is an atomic convenience pointer. Every attempted run is
 * also written to an immutable-by-convention history file before latest is
 * replaced, so a later failed run does not erase prior evidence.
 *
 * The receipt proves only what the launcher directly observes: artifact
 * identity, linker route, exit code, captured stdout markers, timeout state and
 * capture completeness. It does not promote those observations into claims
 * about isolated silicon behavior, timer accuracy, thermal stability or
 * statistical reproducibility.
 */
public final class PaBenchmarkReceipt {

    public static final String SCHEMA = "rafcodephi.pa-elf-runtime-receipt/v2";
    public static final String DIRECTORY = "rafaelia/receipts";
    public static final String HISTORY_DIRECTORY = DIRECTORY + "/pa-history";
    public static final String FILE_NAME = "pa_freestanding_elf_runtime_v2.json";

    public static final String STATE_PASS = "PASS";
    public static final String STATE_FAIL = "FAIL";
    public static final String STATE_BLOCKED = "BLOCKED";
    public static final String STATE_INVALIDATED = "INVALIDATED";

    private static final int MAX_RECEIPT_BYTES = 256 * 1024;
    private static final String HEADER = "RAFCODEPHI-PA-ELF 00000001";
    private static final String MODE = "MODE FREESTANDING NO_LIBC NO_MALLOC NO_JNI DIRECT_SYSCALL";
    private static final String END = "END 00000000";

    private PaBenchmarkReceipt() {}

    public static File getReceiptFile(Context context) {
        File directory = new File(context.getFilesDir(), DIRECTORY);
        return new File(directory, FILE_NAME);
    }

    public static File getHistoryDirectory(Context context) {
        return new File(context.getFilesDir(), HISTORY_DIRECTORY);
    }

    public static String getReadState(Context context) {
        File file = getReceiptFile(context);
        if (!file.isFile()) return "NOT_MEASURED";
        return read(context) == null ? "INVALIDATED" : "AVAILABLE";
    }

    public static String classifyEvidenceState(boolean timedOut,
                                               boolean executionError,
                                               int exitCode,
                                               boolean stdoutTruncated,
                                               boolean markerComplete) {
        if (timedOut || executionError) return STATE_BLOCKED;
        if (exitCode != 0) return STATE_FAIL;
        if (stdoutTruncated || !markerComplete) return STATE_INVALIDATED;
        return STATE_PASS;
    }

    public static String classifyEvidenceReason(boolean timedOut,
                                                boolean executionError,
                                                int exitCode,
                                                boolean stdoutTruncated,
                                                boolean markerComplete) {
        if (timedOut) return "PROCESS_TIMEOUT";
        if (executionError) return "EXECUTION_ERROR";
        if (exitCode != 0) return "NONZERO_EXIT_CODE";
        if (stdoutTruncated) return "STDOUT_CAPTURE_TRUNCATED";
        if (!markerComplete) return "REQUIRED_MARKERS_MISSING";
        return "ALL_RUNTIME_PREDICATES_SATISFIED";
    }

    public static File recordExecution(Context context,
                                       File elf,
                                       String linker,
                                       int exitCode,
                                       String stdout,
                                       Throwable executionError,
                                       boolean timedOut,
                                       boolean stdoutTruncated,
                                       long stdoutObservedBytes,
                                       long wallTimeMs) throws Exception {
        JSONObject receipt = new JSONObject();
        String generatedAt = utcNow();

        receipt.put("schema", SCHEMA);
        receipt.put("generated_at_utc", generatedAt);
        receipt.put("package_name", context.getPackageName());
        receipt.put("app_version", BuildConfig.VERSION_NAME);
        receipt.put("android_release", Build.VERSION.RELEASE);
        receipt.put("android_api", Build.VERSION.SDK_INT);
        receipt.put("manufacturer", Build.MANUFACTURER);
        receipt.put("model", Build.MODEL);
        receipt.put("process_is_64_bit", Build.VERSION.SDK_INT >= 23 && Process.is64Bit());
        receipt.put("vectra_scope", "INTERNAL_TERMUX_RAFCODEPHI_SCREEN");
        receipt.put("external_vectras_app_required", false);
        receipt.put("external_vectras_ci_required", false);

        JSONArray abis = new JSONArray();
        if (Build.SUPPORTED_ABIS != null) {
            for (String abi : Build.SUPPORTED_ABIS) abis.put(abi);
        }
        receipt.put("supported_abis", abis);

        receipt.put("linker", linker == null ? "" : linker);
        receipt.put("elf_path", elf == null ? "" : elf.getAbsolutePath());
        String elfSha = elf != null && elf.isFile() ? sha256File(elf) : "";
        receipt.put("elf_sha256", elfSha);
        receipt.put("exit_code", exitCode);
        receipt.put("timed_out", timedOut);
        receipt.put("wall_time_ms", Math.max(0L, wallTimeMs));

        String captured = stdout == null ? "" : stdout;
        byte[] capturedBytes = captured.getBytes(StandardCharsets.US_ASCII);
        String stdoutSha = sha256Bytes(capturedBytes);
        receipt.put("stdout_sha256", stdoutSha);
        receipt.put("stdout_captured_bytes", capturedBytes.length);
        receipt.put("stdout_observed_bytes", Math.max(stdoutObservedBytes, capturedBytes.length));
        receipt.put("stdout_truncated", stdoutTruncated);
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

        boolean hasExecutionError = executionError != null;
        String evidenceState = classifyEvidenceState(
            timedOut, hasExecutionError, exitCode, stdoutTruncated, markerComplete);
        String evidenceReason = classifyEvidenceReason(
            timedOut, hasExecutionError, exitCode, stdoutTruncated, markerComplete);
        boolean runtimePass = STATE_PASS.equals(evidenceState);

        receipt.put("evidence_state", evidenceState);
        receipt.put("evidence_reason", evidenceReason);
        receipt.put("runtime_exec_pass", runtimePass);
        receipt.put("claim_allowed_runtime_execution", runtimePass);
        receipt.put("claim_allowed_isolated_silicon", false);
        receipt.put("claim_allowed_reproducibility", false);
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

        File historyDirectory = getHistoryDirectory(context);
        if (!historyDirectory.exists() && !historyDirectory.mkdirs() && !historyDirectory.exists()) {
            throw new IOException("Unable to create PA receipt history directory: " + historyDirectory);
        }

        String identityHash = !stdoutSha.isEmpty() ? stdoutSha : (!elfSha.isEmpty() ? elfSha : "nohash");
        String shortHash = identityHash.length() >= 12 ? identityHash.substring(0, 12) : identityHash;
        File historyFile = new File(historyDirectory,
            "pa_" + utcFileStamp() + "_" + evidenceState.toLowerCase(Locale.US) + "_" + shortHash + ".json");
        receipt.put("history_file", historyFile.getAbsolutePath());
        receipt.put("latest_file", getReceiptFile(context).getAbsolutePath());

        writeAtomicFile(historyFile, receipt);
        writeAtomicFile(getReceiptFile(context), receipt);
        return getReceiptFile(context);
    }

    public static JSONObject read(Context context) {
        return readFile(getReceiptFile(context));
    }

    private static JSONObject readFile(File file) {
        if (file == null || !file.isFile()) return null;
        try (FileInputStream input = new FileInputStream(file)) {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream((int) Math.min(file.length(), MAX_RECEIPT_BYTES));
            byte[] buffer = new byte[4096];
            int read;
            int total = 0;
            while ((read = input.read(buffer)) >= 0) {
                if (read == 0) continue;
                total += read;
                if (total > MAX_RECEIPT_BYTES) return null;
                bytes.write(buffer, 0, read);
            }
            return new JSONObject(new String(bytes.toByteArray(), StandardCharsets.UTF_8));
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static void writeAtomicFile(File file, JSONObject receipt) throws IOException {
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

    private static String utcFileStamp() {
        SimpleDateFormat format = new SimpleDateFormat("yyyyMMdd'T'HHmmssSSS'Z'", Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(new Date());
    }
}
