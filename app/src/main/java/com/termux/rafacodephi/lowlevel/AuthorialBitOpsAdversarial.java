package com.termux.rafacodephi.lowlevel;

/**
 * Independent reference vectors for the 32-bit Java-only bit kernel.
 * This test code may use the JVM; the product source uses primitive operators.
 * Failure returns a nonzero case number and the runner throws with that number.
 */
public final class AuthorialBitOpsAdversarial {
    private AuthorialBitOpsAdversarial() {}

    public static int run() {
        int[] cases = {0, -1, Integer.MIN_VALUE, Integer.MAX_VALUE,
                0x55555555, 0xAAAAAAAA, 0x80000001, 0x01010101};
        for (int value : cases) {
            int expectedCount = 0;
            for (int i = 0; i < 32; i++) {
                int expectedBit = (value >>> i) & 1;
                expectedCount += expectedBit;
                if (AuthorialBitOps.read(value, i) != expectedBit) return 100 + i;
                for (int bit = 0; bit <= 1; bit++) {
                    int next = AuthorialBitOps.write(value, i, bit);
                    if (AuthorialBitOps.read(next, i) != bit) return 200 + i;
                    if (((next ^ value) & ~(1 << i)) != 0) return 300 + i;
                }
            }
            if (AuthorialBitOps.count(value) != expectedCount) return 400;
            if (AuthorialBitOps.parity(value) != (expectedCount & 1)) return 401;
        }
        for (int value = 0; value < 4096; value++) {
            int expected = 0;
            int rest = value;
            while (rest != 0) {
                expected += rest & 1;
                rest >>>= 1;
            }
            if (AuthorialBitOps.count(value) != expected) return 500 + value;
            if (AuthorialBitOps.parity(value) != (expected & 1)) return 5000 + value;
        }
        if (AuthorialBitOps.read(-1, -1) != -1) return 10000;
        if (AuthorialBitOps.read(-1, 32) != -1) return 10001;
        if (AuthorialBitOps.write(7, -1, 1) != 7) return 10002;
        if (AuthorialBitOps.write(7, 32, 1) != 7) return 10003;
        if (AuthorialBitOps.write(7, 1, 2) != 7) return 10004;
        return 0;
    }

    public static void main(String[] args) {
        int failure = run();
        if (failure != 0) throw new AssertionError("bit vector " + failure);
    }
}
