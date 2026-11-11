package com.termux.shared.termux.shell.command.runner.terminal;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

/**
 * Read-only Android ELF minimum-SDK probe for login shell selection.
 * This is not an ELF execution or dependency test. An unknown note is not PASS.
 */
public final class BootstrapElfApi {
    private static final int PROBE_BYTES = 4096;
    private BootstrapElfApi() {}

    /**
     * @return 0 for non-ELF, a positive minimum Android API, or -1 for
     *         an unreadable ELF or a missing/invalid Android note.
     */
    public static int minimumAndroidApi(File binary) {
        byte[] head = new byte[PROBE_BYTES];
        int count = 0;
        try (FileInputStream in = new FileInputStream(binary)) {
            while (count < head.length) {
                int n = in.read(head, count, head.length - count);
                if (n <= 0) break;
                count += n;
            }
        } catch (IOException e) {
            return -1;
        }
        if (count < 6) return -1;
        if (head[0] != 0x7f || head[1] != 'E' || head[2] != 'L' || head[3] != 'F') return 0;
        if (head[5] != 1) return -1; // Android ARM32/ARM64 little-endian ELF.
        for (int p = 12; p <= count - 12; p++) {
            if (head[p] != 'A' || head[p + 1] != 'n' || head[p + 2] != 'd' ||
                head[p + 3] != 'r' || head[p + 4] != 'o' || head[p + 5] != 'i' ||
                head[p + 6] != 'd' || head[p + 7] != 0) continue;
            if (little32(head, p - 12) != 8 || little32(head, p - 8) < 4 ||
                little32(head, p - 4) != 1) continue;
            int sdk = little32(head, p + 8);
            if (sdk >= 1 && sdk <= 1000) return sdk;
        }
        return -1;
    }

    private static int little32(byte[] data, int p) {
        return (data[p] & 0xff) | ((data[p+1] & 0xff) << 8) |
            ((data[p+2] & 0xff) << 16) | ((data[p+3] & 0xff) << 24);
    }
}
