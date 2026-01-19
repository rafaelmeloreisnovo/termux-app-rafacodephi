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

    public static native int nativeClamp(int v, int lo, int hi);

    public static native int nativeStableHash(String s);

    public static native void nativeFlipInPlace(float[] a);

    public static String normalizeTag(String tag) {
        if (tag == null) return "";
        String t = tag.trim();
        if (t.isEmpty()) return "";
        return t.toUpperCase();
    }
}
