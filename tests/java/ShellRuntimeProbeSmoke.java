import com.termux.shared.termux.shell.command.runner.terminal.ShellRuntimeProbe;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Collections;

/** Host-only subprocess falsifiers; no Android SDK and no installer mutation. */
public final class ShellRuntimeProbeSmoke {
    private static final File CWD = new File(System.getProperty("java.io.tmpdir"));
    private static int assertions;

    private static File script(Path dir, String name, String body) throws Exception {
        Path file = dir.resolve(name);
        Files.write(file, ("#!/bin/sh\n" + body + "\n").getBytes(StandardCharsets.UTF_8));
        if (!file.toFile().setExecutable(true)) throw new Exception("script not executable");
        return file.toFile();
    }

    private static void check(boolean actual, boolean expected, String name) {
        assertions++;
        if (actual != expected) throw new AssertionError(name + " expected=" + expected + " actual=" + actual);
    }

    public static void main(String[] argv) throws Exception {
        Path dir = Files.createTempDirectory("raf-shell-runtime-smoke");
        try {
            File valid = script(dir, "valid.sh", "exec /bin/sh \"$@\"");
            check(ShellRuntimeProbe.canRun(valid, CWD, Collections.emptyMap(), 900), true,
                  "healthy installed shell");
            File linkerFailure = script(dir, "linker-fail.sh",
                "echo 'CANNOT LINK EXECUTABLE: cannot locate symbol memfd_create' >&2\nexit 127");
            check(ShellRuntimeProbe.canRun(linkerFailure, CWD, Collections.emptyMap(), 900), false,
                  "linker error fallback");
            File falseSuccess = script(dir, "false-success.sh", "exit 0");
            check(ShellRuntimeProbe.canRun(falseSuccess, CWD, Collections.emptyMap(), 900), false,
                  "exit zero is not sufficient");
            File broken = dir.resolve("bad-elf").toFile();
            Files.write(broken.toPath(), new byte[]{0x7f, 'E', 'L', 'F', 2, 1, 1});
            broken.setExecutable(true);
            check(ShellRuntimeProbe.canRun(broken, CWD, Collections.emptyMap(), 900), false,
                  "unloadable ELF ABI");
            File hung = script(dir, "hung.sh", "sleep 3");
            long before = System.nanoTime();
            check(ShellRuntimeProbe.canRun(hung, CWD, Collections.emptyMap(), 150), false,
                  "watchdog timeout");
            long elapsed = (System.nanoTime() - before) / 1_000_000L;
            if (elapsed > 2300) throw new AssertionError("timeout path stalled " + elapsed + "ms");
            // No probe cache: after installation repairs the same path, normal
            // preference automatically fails BACK to the desired healthy shell.
            File repaired = script(dir, "repaired.sh", "exit 127");
            check(ShellRuntimeProbe.canRun(repaired, CWD, Collections.emptyMap(), 900), false,
                  "broken before repair");
            repaired = script(dir, "repaired.sh", "exec /bin/sh \"$@\"");
            check(ShellRuntimeProbe.canRun(repaired, CWD, Collections.emptyMap(), 900), true,
                  "automatic failback after repair");
            // Never treat absent package binaries as a successful runtime.
            check(ShellRuntimeProbe.canRun(dir.resolve("absent").toFile(),
                  CWD, Collections.emptyMap(), 900), false, "absent executable");
            System.out.println("RAFCODEPHI_SHELL_RUNTIME=PASS_SCOPED_HOST assertions=" + assertions
                + " device_runtime=TOKEN_VAZIO claim_allowed=false");
        } finally {
            // Cleanup only generated host test fixtures (not Android HOME/PREFIX).
            try (java.util.stream.Stream<Path> entries = Files.list(dir)) {
                entries.forEach(p -> {
                    try { Files.deleteIfExists(p); } catch (Exception ignored) {}
                });
            }
            Files.deleteIfExists(dir);
        }
    }
}
