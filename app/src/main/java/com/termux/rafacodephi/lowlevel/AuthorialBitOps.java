package com.termux.rafacodephi.lowlevel;

/**
 * RAFCODEPHI authorial bit primitives.
 * No imports, no allocation, no String/Math/Arrays/BitSet calls.
 * Critical production paths must pair this adapter with PURE_CORE C/Rust/ASM.
 */
public final class AuthorialBitOps {
    private AuthorialBitOps() {}
    /** Returns bit i; invalid indexes return -1. O(1). */
    public static int read(int value, int index) {
        if (index < 0 || index > 31) return -1;
        return (value >>> index) & 1;
    }
    /** Sets or clears bit i without library calls. O(1). */
    public static int write(int value, int index, int bit) {
        if (index < 0 || index > 31 || (bit & ~1) != 0) return value;
        int mask = 1 << index;
        return bit == 0 ? value & ~mask : value | mask;
    }
    /** Branch-bounded parity fold. O(log word). */
    public static int parity(int value) {
        value ^= value >>> 16; value ^= value >>> 8; value ^= value >>> 4;
        value &= 0xF;
        return (0x6996 >>> value) & 1;
    }
    /** Fixed-width population count. O(log word). */
    public static int count(int value) {
        value -= (value >>> 1) & 0x55555555;
        value = (value & 0x33333333) + ((value >>> 2) & 0x33333333);
        return (((value + (value >>> 4)) & 0x0F0F0F0F) * 0x01010101) >>> 24;
    }
}
