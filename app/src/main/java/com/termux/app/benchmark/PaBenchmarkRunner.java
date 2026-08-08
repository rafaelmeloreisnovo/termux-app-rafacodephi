package com.termux.app.benchmark;

import android.content.Context;
import android.os.Build;
import android.os.SystemClock;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

/** One physical PA ELF execution with bounded stdout and pre/post environment observations. */
public final class PaBenchmarkRunner {

    private static final int STDOUT_CAPTURE_LIMIT = 64 * 1024;
    private static final long PROCESS_TIMEOUT_MS = 60_000L;
    private static final long PROCESS_POLL_MS = 25L;
    private static final long READER_JOIN_MS = 2_000L;

    private PaBenchmarkRunner() {}

    public static Result runOnce(Context context) {
        return runOnce(context, "", -1, 0);
    }

    public static Result runOnce(Context context, String seriesId, int seriesIndex, int seriesTargetN) {
        File elf = null;
        String linker = null;
        String stdout = "";
        int exit = -1;
        Throwable executionError = null;
        boolean timedOut = false;
        boolean stdoutTruncated = false;
        long stdoutObservedBytes = 0L;
        long startMs = SystemClock.elapsedRealtime();
        JSONObject environmentBefore = BenchmarkEnvironmentSnapshot.capture(context);
        JSONObject environmentAfter = null;
        File receiptFile = null;
        JSONObject persisted = null;

        Process process = null;
        InputStream processOutput = null;
        Thread reader = null;
        ByteArrayOutputStream captured = new ByteArrayOutputStream(4096);
        AtomicLong observed = new AtomicLong(0L);
        AtomicBoolean truncated = new AtomicBoolean(false);
        AtomicReference<Throwable> readerError = new AtomicReference<>(null);

        try {
            elf = new File(context.getApplicationInfo().nativeLibraryDir, "libraf_pa_core.so");
            if (!elf.isFile()) throw new IllegalStateException("ELF missing: " + elf);

            linker = selectLinker();
            if (linker == null) throw new IllegalStateException("Android linker unavailable");

            process = new ProcessBuilder(linker, elf.getAbsolutePath())
                .redirectErrorStream(true)
                .start();

            processOutput = process.getInputStream();
            final InputStream readerStream = processOutput;
            reader = new Thread(() -> {
                try (InputStream stream = readerStream) {
                    byte[] buffer = new byte[1024];
                    int read;
                    while ((read = stream.read(buffer)) >= 0) {
                        if (read == 0) continue;
                        long previous = observed.getAndAdd(read);
                        int remaining = (int) Math.max(0L, STDOUT_CAPTURE_LIMIT - previous);
                        int accepted = Math.min(read, remaining);
                        if (accepted > 0) {
                            synchronized (captured) {
                                captured.write(buffer, 0, accepted);
                            }
                        }
                        if (accepted < read || previous + read > STDOUT_CAPTURE_LIMIT) truncated.set(true);
                    }
                } catch (Throwable error) {
                    readerError.compareAndSet(null, error);
                }
            }, "pa-elf-stdout");
            reader.start();

            long deadline = startMs + PROCESS_TIMEOUT_MS;
            boolean finished = false;
            while (SystemClock.elapsedRealtime() < deadline) {
                try {
                    exit = process.exitValue();
                    finished = true;
                    break;
                } catch (IllegalThreadStateException stillRunning) {
                    Thread.sleep(PROCESS_POLL_MS);
                }
            }

            if (!finished) {
                timedOut = true;
                process.destroy();
                try {
                    if (processOutput != null) processOutput.close();
                } catch (Throwable ignored) {
                }
            }

            if (reader != null) {
                reader.join(READER_JOIN_MS);
                if (reader.isAlive()) {
                    truncated.set(true);
                    reader.interrupt();
                }
            }

            Throwable streamFailure = readerError.get();
            if (streamFailure != null && !timedOut) {
                executionError = new IllegalStateException("stdout capture failed", streamFailure);
            }

            synchronized (captured) {
                stdout = new String(captured.toByteArray(), StandardCharsets.US_ASCII);
            }
            stdoutObservedBytes = observed.get();
            stdoutTruncated = truncated.get();
        } catch (Throwable error) {
            executionError = error;
            try {
                if (process != null) process.destroy();
            } catch (Throwable ignored) {
            }
            synchronized (captured) {
                stdout = new String(captured.toByteArray(), StandardCharsets.US_ASCII);
            }
            stdoutObservedBytes = observed.get();
            stdoutTruncated = truncated.get();
        } finally {
            environmentAfter = BenchmarkEnvironmentSnapshot.capture(context);
        }

        long wallTimeMs = Math.max(0L, SystemClock.elapsedRealtime() - startMs);
        Throwable receiptError = null;
        try {
            receiptFile = PaBenchmarkReceipt.recordExecution(
                context, elf, linker, exit, stdout, executionError, timedOut,
                stdoutTruncated, stdoutObservedBytes, wallTimeMs,
                environmentBefore, environmentAfter, seriesId, seriesIndex, seriesTargetN);
            persisted = PaBenchmarkReceipt.read(context);
        } catch (Throwable error) {
            receiptError = error;
        }

        return new Result(elf, linker, exit, stdout, executionError, receiptError,
            timedOut, stdoutTruncated, stdoutObservedBytes, wallTimeMs,
            environmentBefore, environmentAfter, receiptFile, persisted,
            seriesId, seriesIndex, seriesTargetN);
    }

    private static String selectLinker() {
        boolean process64 = Build.VERSION.SDK_INT >= 23 && android.os.Process.is64Bit();
        String apex = process64
            ? "/apex/com.android.runtime/bin/linker64"
            : "/apex/com.android.runtime/bin/linker";
        if (new File(apex).isFile()) return apex;
        String system = process64 ? "/system/bin/linker64" : "/system/bin/linker";
        if (new File(system).isFile()) return system;
        return null;
    }

    public static final class Result {
        public final File elf;
        public final String linker;
        public final int exitCode;
        public final String stdout;
        public final Throwable executionError;
        public final Throwable receiptError;
        public final boolean timedOut;
        public final boolean stdoutTruncated;
        public final long stdoutObservedBytes;
        public final long wallTimeMs;
        public final JSONObject environmentBefore;
        public final JSONObject environmentAfter;
        public final File receiptFile;
        public final JSONObject receipt;
        public final String seriesId;
        public final int seriesIndex;
        public final int seriesTargetN;

        Result(File elf, String linker, int exitCode, String stdout,
               Throwable executionError, Throwable receiptError,
               boolean timedOut, boolean stdoutTruncated, long stdoutObservedBytes,
               long wallTimeMs, JSONObject environmentBefore, JSONObject environmentAfter,
               File receiptFile, JSONObject receipt, String seriesId,
               int seriesIndex, int seriesTargetN) {
            this.elf = elf;
            this.linker = linker;
            this.exitCode = exitCode;
            this.stdout = stdout;
            this.executionError = executionError;
            this.receiptError = receiptError;
            this.timedOut = timedOut;
            this.stdoutTruncated = stdoutTruncated;
            this.stdoutObservedBytes = stdoutObservedBytes;
            this.wallTimeMs = wallTimeMs;
            this.environmentBefore = environmentBefore;
            this.environmentAfter = environmentAfter;
            this.receiptFile = receiptFile;
            this.receipt = receipt;
            this.seriesId = seriesId == null ? "" : seriesId;
            this.seriesIndex = seriesIndex;
            this.seriesTargetN = seriesTargetN;
        }

        public boolean runtimePass() {
            return receipt != null && receipt.optBoolean("claim_allowed_runtime_execution", false);
        }

        public boolean timingPass() {
            return receipt != null && receipt.optBoolean("claim_allowed_timing_measurement", false);
        }

        public String evidenceState() {
            return receipt == null ? "INVALIDATED" : receipt.optString("evidence_state", "INVALIDATED");
        }

        public String render() {
            StringBuilder out = new StringBuilder();
            out.append("scope=INTERNAL_TERMUX_RAFCODEPHI\n");
            out.append("external_vectras_required=false\n");
            out.append("linker=").append(linker == null ? "UNAVAILABLE" : linker).append("\n");
            out.append("elf=").append(elf == null ? "UNAVAILABLE" : elf.getAbsolutePath()).append("\n");
            out.append("exit=").append(exitCode).append("\n");
            out.append("timed_out=").append(timedOut).append("\n");
            out.append("stdout_observed_bytes=").append(stdoutObservedBytes).append("\n");
            out.append("stdout_truncated=").append(stdoutTruncated).append("\n");
            out.append("wall_time_ms=").append(wallTimeMs).append("\n");
            if (!seriesId.isEmpty()) {
                out.append("series_id=").append(seriesId).append("\n");
                out.append("series_index=").append(seriesIndex).append("/").append(seriesTargetN).append("\n");
            }
            if (executionError != null) {
                out.append("execution_error=").append(executionError.getClass().getSimpleName())
                    .append(": ").append(String.valueOf(executionError.getMessage())).append("\n");
            }
            if (receiptError != null) {
                out.append("RECEIPT_WRITE_FAIL_CLOSED=").append(receiptError.getClass().getSimpleName())
                    .append(": ").append(String.valueOf(receiptError.getMessage())).append("\n");
            }
            if (receipt != null) {
                out.append("evidence_state=").append(evidenceState()).append("\n");
                out.append("evidence_reason=").append(receipt.optString("evidence_reason", "UNKNOWN")).append("\n");
                out.append("pa_protocol=").append(receipt.optInt("pa_protocol_version", 0)).append("\n");
                out.append("claim_allowed_runtime_execution=").append(runtimePass()).append("\n");
                out.append("claim_allowed_timing_measurement=").append(timingPass()).append("\n");
                out.append("environment_state=").append(receipt.optString("environment_state", "NOT_MEASURED")).append("\n");
                out.append("thermal_interference_observed=")
                    .append(receipt.optBoolean("thermal_interference_observed", false)).append("\n");
                out.append("receipt=").append(receiptFile == null ? "UNAVAILABLE" : receiptFile.getAbsolutePath()).append("\n");
            }
            out.append("\n").append(stdout == null ? "" : stdout);
            return out.toString();
        }
    }
}
