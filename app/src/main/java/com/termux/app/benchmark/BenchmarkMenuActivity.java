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
 * This class does not load a library and declares no Java native method.
 * It only asks Android's linker to execute the packaged ELF, captures stdout,
 * and persists an atomic evidence receipt for the runtime/industrial views.
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
        contract.setText("DEX launcher → Android linker → ELF _start → C/ASM/syscalls\nNo JNI · No libc · No malloc · No ZIP\nEvidence: artifact hash + exit code + stdout markers + atomic receipt");
        contract.setPadding(0, 12, 0, 16);
        root.addView(contract);

        run = new Button(this);
        run.setText("Execute ELF Benchmark");
        run.setOnClickListener(view -> execute());
        root.addView(run);

        output = new TextView(this);
        output.setTextIsSelectable(true);
        output.setText("ELF not executed. No device receipt exists until an execution is attempted.");
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

            try {
                elf = new File(getApplicationInfo().nativeLibraryDir, "libraf_pa_core.so");
                if (!elf.isFile()) throw new IllegalStateException("ELF missing: " + elf);

                linker = selectLinker();
                Process process = new ProcessBuilder(linker, elf.getAbsolutePath())
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

                exit = process.waitFor();
                stdout = new String(bytes.toByteArray(), StandardCharsets.US_ASCII);
                result = "linker=" + linker + "\nelf=" + elf + "\nexit=" + exit + "\n\n" + stdout;
            } catch (Throwable error) {
                executionError = error;
                result = "FAIL_CLOSED\n" + error.getClass().getSimpleName() + ": " + error.getMessage();
            }

            try {
                File receipt = PaBenchmarkReceipt.recordExecution(
                    this, elf, linker, exit, stdout, executionError);
                result += "\n\nreceipt=" + receipt.getAbsolutePath();
                result += "\nevidence_state=" + (executionError == null && exit == 0
                    ? "OBSERVED_CHECK_RECEIPT_MARKERS" : "FAIL_OR_BLOCKED");
            } catch (Throwable receiptError) {
                result += "\n\nRECEIPT_WRITE_FAIL_CLOSED="
                    + receiptError.getClass().getSimpleName() + ": " + receiptError.getMessage();
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
