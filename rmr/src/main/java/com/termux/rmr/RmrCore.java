package com.termux.rmr;

/**
 * RMR low-level utilities exposed via JNI.
 */
public final class RmrCore {

    static {
        System.loadLibrary("rmr");
    }

    private RmrCore() {
        throw new IllegalStateException("Utility class");
    }

    public static native String nativeNormalizeTag(String tag);

    public static native int nativeClamp(int v, int lo, int hi);

    public static native int nativeStableHash(String s);

    public static native int nativeTransmuteU32(int v);

    public static native void nativeFlipInPlace(float[] a);
}
