package com.termux.app.benchmark;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

/**
 * DEX edge for the freestanding PA benchmark inside Termux RAFCODEΦ.
 *
 * This is not the external Vectras application and does not depend on any
 * Vectras package or CI. The activity only asks Android's linker to execute the
 * packaged ELF, captures bounded stdout while draining the pipe, and persists
 * fail-closed evidence for the internal Vectra runtime screen.
 */
public final class BenchmarkMenuActivity extends Activity {

    private static final int STDOUT_CAPTURE_LIMIT = 64 * 1024;
    private static final long PROCESS_TIMEOUT_MS = 60_000L;
    private static final long PROCESS_POLL_MS = 25L;
    private static final long READER_JOIN_MS = 2_000L;

    private TextView output;
    private Button run;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        setTitle("PA Silicon · Freestanding ELF");
        setContentView(layout());
    }

    private android.view.View layout() {
        ScrollView scroll = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(24, 24, 24, 24);

        TextView title = new TextView(this);
        title.setText("RAFCODEΦ · PA Silicon Core");
        title.setTextSize(18f);
        root.addView(title);

        TextView contract = new TextView(this);
        contract.setText(
            "Internal Termux RAFCODEΦ path — no external Vectras app required\n" +
            "DEX launcher → Android linker → ELF _start → C/ASM/syscalls\n" +
            "No JNI · No libc · No malloc · No ZIP\n" +
            "Evidence: artifact hash + exit code + timeout + stdout markers + append history");
        contract.setPadding(0, 12, 0, 16);
        root.addView(contract);

        run = new Button(this);
        run.setText("Execute ELF Benchmark");
        run.setOnClickListener(view -> execute());
        root.addView(run);

        output = new TextView(this);
        output.setTextIsSelectable(true);
        String readState = PaBenchmarkReceipt.getReadState(this);
        if ("AVAILABLE".equals(readState)) {
            JSONObject existing = PaBenchmarkReceipt.read(this);
            output.setText("Latest device receipt: "
                + (existing == null ? "INVALIDATED" : existing.optString("evidence_state", "UNKNOWN"))
                + "\nRun again to append a new observation.");
        } else if ("INVALIDATED".equals(readState)) {
            output.setText("Latest receipt exists but is unreadable: INVALIDATED. Run again to create new evidence; history is preserved.");
        } else {
            output.setText("ELF not executed by this build. Runtime evidence: NOT_MEASURED.");
        }
        output.setPadding(0, 16, 0, 0);
        root.addView(output, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));

        scroll.addView(root);
        return scroll;
    }

    private void execute() {
        run.setEnabled(false);
        output.setText("Executing direct ELF entry and binding evidence…");

        new Thread(() -> {
            File elf = null;
            String linker = null;
            String stdout = "";
            int exit = -1;
            String result;
            Throwable executionError = null;
            boolean timedOut = false;
            boolean stdoutTruncated = false;
            long stdoutObservedBytes = 0L;
            long startMs = SystemClock.elapsedRealtime();

            Process process = null;
            InputStream processOutput = null;
            Thread reader = null;
            ByteArrayOutputStream captured = new ByteArrayOutputStream(4096);
            AtomicLong observed = new AtomicLong(0L);
            AtomicBoolean truncated = new AtomicBoolean(false);
            AtomicReference<Throwable> readerError = new AtomicReference<>(null);

            try {
                elf = new File(getApplicationInfo().nativeLibraryDir, "libraf_pa_core.so");
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
                            if (accepted < read || previous + read > STDOUT_CAPTURE_LIMIT) {
                                truncated.set(true);
                            }
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

                result = "scope=INTERNAL_TERMUX_RAFCODEPHI\n"
                    + "external_vectras_required=false\n"
                    + "linker=" + linker + "\n"
                    + "elf=" + elf + "\n"
                    + "exit=" + exit + "\n"
                    + "timed_out=" + timedOut + "\n"
                    + "stdout_observed_bytes=" + stdoutObservedBytes + "\n"
                    + "stdout_truncated=" + stdoutTruncated + "\n\n"
                    + stdout;
            } catch (Throwable error) {
                executionError = error;
                try {
                    if (process != null) process.destroy();
                } catch (Throwable ignored) {
                }
                result = "FAIL_CLOSED\n" + error.getClass().getSimpleName() + ": " + error.getMessage();
            }

            long wallTimeMs = Math.max(0L, SystemClock.elapsedRealtime() - startMs);

            try {
                File receipt = PaBenchmarkReceipt.recordExecution(
                    this,
                    elf,
                    linker,
                    exit,
                    stdout,
                    executionError,
                    timedOut,
                    stdoutTruncated,
                    stdoutObservedBytes,
                    wallTimeMs);
                JSONObject persisted = PaBenchmarkReceipt.read(this);
                String evidenceState = persisted == null
                    ? "INVALIDATED"
                    : persisted.optString("evidence_state", "INVALIDATED");
                String evidenceReason = persisted == null
                    ? "RECEIPT_UNREADABLE_AFTER_WRITE"
                    : persisted.optString("evidence_reason", "UNKNOWN");
                result += "\n\nreceipt=" + receipt.getAbsolutePath();
                result += "\nhistory=" + PaBenchmarkReceipt.getHistoryDirectory(this).getAbsolutePath();
                result += "\nevidence_state=" + evidenceState;
                result += "\nevidence_reason=" + evidenceReason;
                result += "\nclaim_allowed_runtime_execution="
                    + (persisted != null && persisted.optBoolean("claim_allowed_runtime_execution", false));
            } catch (Throwable receiptError) {
                result += "\n\nRECEIPT_WRITE_FAIL_CLOSED="
                    + receiptError.getClass().getSimpleName() + ": " + receiptError.getMessage();
                result += "\nevidence_state=INVALIDATED";
                result += "\nclaim_allowed_runtime_execution=false";
            }

            final String rendered = result;
            runOnUiThread(() -> {
                output.setText(rendered);
                run.setEnabled(true);
            });
        }, "pa-elf-launch").start();
    }

    private String selectLinker() {
        boolean process64 = Build.VERSION.SDK_INT >= 23 && android.os.Process.is64Bit();
        String apex = process64
            ? "/apex/com.android.runtime/bin/linker64"
            : "/apex/com.android.runtime/bin/linker";
        if (new File(apex).isFile()) return apex;

        String system = process64 ? "/system/bin/linker64" : "/system/bin/linker";
        if (new File(system).isFile()) return system;
        return null;
    }
}
