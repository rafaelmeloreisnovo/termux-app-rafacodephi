package com.termux.rafacodephi.lowlevel;

/** Deterministic vectors; nonzero return means failure. */
public final class AuthorialBitOpsVectors {
    private AuthorialBitOpsVectors() {}
    public static int run() {
        if (AuthorialBitOps.read(0x80000001, 0) != 1) return 1;
        if (AuthorialBitOps.read(0x80000001, 31) != 1) return 2;
        if (AuthorialBitOps.read(0, 32) != -1) return 3;
        if (AuthorialBitOps.write(0, 7, 1) != 128) return 4;
        if (AuthorialBitOps.write(128, 7, 0) != 0) return 5;
        if (AuthorialBitOps.parity(0xF) != 0) return 6;
        if (AuthorialBitOps.parity(0x7) != 1) return 7;
        if (AuthorialBitOps.count(0xF0F0) != 8) return 8;
        return 0;
    }
}
