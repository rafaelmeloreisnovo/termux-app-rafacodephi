package com.termux.shared.termux.shell.command.runner.terminal;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Runtime-backed login-shell readiness probe.
 *
 * The Android loader decides ABI, API and dynamic symbols: a file's executable
 * bit or ELF Android note alone is never sufficient. This is a bounded
 * noninteractive probe with NO changes to HOME, PREFIX or the chosen shell.
 *
 * No positive cache: every new interactive session retries the preferred
 * shell, so repair/replacement naturally fails back from system sh to Bash.
 */
public final class ShellRuntimeProbe {
    private static final String MARKER = "RAFCODEPHI_SHELL_READY_v1";
    private static final int MAX_OUTPUT_BYTES = 4096;
    public static final long DEFAULT_TIMEOUT_MS = 900L;

    private ShellRuntimeProbe() {}

    public static boolean canRun(File shell, File workDir,
                                 Map<String, String> proposedEnv, long timeoutMillis) {
        if (shell == null || !shell.isFile() || !shell.canExecute()
                || workDir == null || !workDir.isDirectory()
                || timeoutMillis <= 0 || timeoutMillis > 5000) return false;
        Process process = null;
        try {
            ProcessBuilder builder = new ProcessBuilder(shell.getAbsolutePath(), "-c",
                    "printf '%s' " + MARKER);
            builder.directory(workDir);
            builder.redirectError(new File("/dev/null"));
            Map<String, String> env = builder.environment();
            env.clear();
            // The probe uses the same essential shell runtime paths while
            // avoiding unrelated app/host injections and user startup files.
            for (String key : new String[] {
                "HOME", "PREFIX", "PATH", "TMPDIR", "LD_LIBRARY_PATH", "LANG", "TERM"
            }) {
                String value = proposedEnv == null ? null : proposedEnv.get(key);
                if (value != null && !value.isEmpty()) env.put(key, value);
            }
            if (!env.containsKey("PATH")) env.put("PATH", "/system/bin:/system/xbin");
            if (!env.containsKey("HOME")) env.put("HOME", workDir.getAbsolutePath());
            process = builder.start();
            process.getOutputStream().close();
            if (!process.waitFor(timeoutMillis, TimeUnit.MILLISECONDS)) return false;
            if (process.exitValue() != 0) return false;
            InputStream output = process.getInputStream();
            ByteArrayOutputStream observed = new ByteArrayOutputStream(64);
            byte[] buffer = new byte[256];
            // available() avoids blocking on descendants holding a pipe open.
            while (observed.size() < MAX_OUTPUT_BYTES) {
                int ready = output.available();
                if (ready <= 0) break;
                int n = output.read(buffer, 0, Math.min(buffer.length,
                        Math.min(ready, MAX_OUTPUT_BYTES - observed.size())));
                if (n <= 0) break;
                observed.write(buffer, 0, n);
            }
            return observed.toString("UTF-8").contains(MARKER);
        } catch (IOException | SecurityException e) {
            return false;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return false;
        } catch (RuntimeException e) {
            // A malformed shell or Android process-spawn error must never take
            // down the normal terminal's recovery path.
            return false;
        } finally {
            if (process != null) {
                if (process.isAlive()) process.destroyForcibly();
                try { process.getInputStream().close(); } catch (IOException ignored) {}
                try { process.getErrorStream().close(); } catch (IOException ignored) {}
                try { process.getOutputStream().close(); } catch (IOException ignored) {}
            }
        }
    }
}
