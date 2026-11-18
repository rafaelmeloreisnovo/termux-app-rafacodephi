package com.termux.rafacodephi.lowlevel;

/**
 * RAFCODEPHI 32-bit Java-only bit kernel.
 *
 * Contract: primitive operators only; no imports, JNI, allocation or library
 * calls in the operations. Runs in a JVM or Android ART, not as freestanding
 * machine code. The caller owns validation of any external buffers or I/O.
 *
 * Performance is unmeasured. O(1) here means bounded operations per word.
 */
public final class AuthorialBitOps {
    private AuthorialBitOps() {}

    /** Returns bit i, or -1 when index is outside 0..31. */
    public static int read(int value, int index) {
        if (index < 0 || index > 31) return -1;
        return (value >>> index) & 1;
    }

    /**
     * Returns the new word; invalid index or bit leaves value unchanged.
     * Callers that require an error signal must validate their arguments first.
     */
    public static int write(int value, int index, int bit) {
        if (index < 0 || index > 31 || (bit & ~1) != 0) return value;
        int mask = 1 << index;
        return bit == 0 ? value & ~mask : value | mask;
    }

    /** XOR-fold parity of all 32 bits, returning 0 or 1. */
    public static int parity(int value) {
        value ^= value >>> 16;
        value ^= value >>> 8;
        value ^= value >>> 4;
        value &= 0xF;
        return (0x6996 >>> value) & 1;
    }

    /** Counts set bits in a 32-bit word; bounded SWAR arithmetic. */
    public static int count(int value) {
        value -= (value >>> 1) & 0x55555555;
        value = (value & 0x33333333) + ((value >>> 2) & 0x33333333);
        return (((value + (value >>> 4)) & 0x0F0F0F0F) * 0x01010101) >>> 24;
    }
}
