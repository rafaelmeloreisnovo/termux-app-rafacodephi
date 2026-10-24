package com.termux.app.rafaelia;

/**
 * HOSTED_BOUNDARY only. This class owns the unavoidable Android/ART↔JNI edge.
 * A returned plan is never treated as a device execution or authorization.
 * There is no Java fallback engine, heap allocation of native work buffers,
 * extra Android permission, timer, logging, or background task here.
 */
public final class RafaeliaMiddlewareBridge {
    public static final int WORDS = 16;
    public static final int TRANSPORT_OK = 0;
    public static final int INVALID_BUFFER = -1;
    public static final int ALIASED_BUFFER = -2;
    public static final int WRONG_FRAME = -3;
    public static final int JNI_TRANSFER_FAILURE = -4;
    public static final int EXECUTION_CLAIM_BLOCKED = -5;
    public static final int NATIVE_LIBRARY_UNAVAILABLE = -6;

    private static final boolean LIBRARY_AVAILABLE;
    static {
        boolean loaded;
        try {
            System.loadLibrary("termux_rafaelia_middleware_s15");
            loaded = true;
        } catch (UnsatisfiedLinkError | SecurityException ex) {
            loaded = false;
        }
        LIBRARY_AVAILABLE = loaded;
    }

    private RafaeliaMiddlewareBridge() {}

    private static native int nativePlan(int[] request, int[] result);

    public static boolean isAvailable() {
        return LIBRARY_AVAILABLE;
    }

    /**
     * Caller owns both independent sixteen-word frames. Index 14 of result
     * MUST remain zero: S14 makes a deterministic routing plan, not a syscall,
     * a JNI workload dispatch, or an Android/NEON/GPU execution.
     */
    public static int plan(int[] request, int[] result) {
        if (request == null || result == null) return INVALID_BUFFER;
        if (request == result) return ALIASED_BUFFER;
        if (request.length != WORDS || result.length != WORDS) return WRONG_FRAME;
        if (!LIBRARY_AVAILABLE) return NATIVE_LIBRARY_UNAVAILABLE;
        return nativePlan(request, result);
    }
}
