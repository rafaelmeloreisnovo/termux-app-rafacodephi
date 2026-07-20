package com.termux.rafaelia;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Android bridge for the RAFAELIA ZERO RFZ1 binary kernel.
 * The canonical hot path accepts DirectByteBuffer and performs no per-call allocation.
 */
public final class RafaeliaZero {
    public static final int OK = 0;
    public static final int E_NULL = -1;
    public static final int E_SIZE = -2;
    public static final int E_MAGIC = -3;
    public static final int E_VERSION = -4;
    public static final int E_CRC = -5;
    public static final int E_RANGE = -6;
    public static final int E_STATE = -7;

    public static final int MAX_PAYLOAD = 1024;
    private static final Object LOCK = new Object();
    private static final ByteBuffer STAGING = ByteBuffer.allocateDirect(MAX_PAYLOAD).order(ByteOrder.nativeOrder());
    private static final boolean AVAILABLE;
    private static final int INIT_STATUS;

    static {
        boolean available = false;
        int status = E_STATE;
        try {
            System.loadLibrary("termux_rafaelia_zero");
            status = nativeInit();
            available = status == OK;
        } catch (Throwable ignored) {
            status = E_STATE;
        }
        INIT_STATUS = status;
        AVAILABLE = available;
    }

    private RafaeliaZero() {}

    private static native int nativeInit();
    private static native int nativeEncodeIngestDirect(ByteBuffer payload, int offset, int bytes,
                                                       int flags, long source, long sequence);
    private static native int nativeIngestFrameDirect(ByteBuffer frame, int offset, int bytes);
    private static native int nativeStateDigest();
    private static native int nativeArchitecture();
    private static native int nativeMaxPayload();
    private static native int nativeAccepted();
    private static native int nativeRejected();

    public static int init() {
        return INIT_STATUS;
    }

    public static boolean isAvailable() {
        return AVAILABLE;
    }

    public static int ingestDirect(ByteBuffer payload, int bytes, long source, long sequence, int flags) {
        if (!AVAILABLE) return E_STATE;
        if (payload == null || !payload.isDirect()) return E_NULL;
        int offset = payload.position();
        if (bytes < 0 || bytes > MAX_PAYLOAD || offset < 0 || offset + bytes > payload.capacity()) return E_RANGE;
        synchronized (LOCK) {
            return nativeEncodeIngestDirect(payload, offset, bytes, flags, source, sequence);
        }
    }

    public static int ingest(byte[] payload, int bytes, long source, long sequence, int flags) {
        if (!AVAILABLE) return E_STATE;
        if (payload == null) return E_NULL;
        if (bytes < 0 || bytes > payload.length || bytes > MAX_PAYLOAD) return E_RANGE;
        synchronized (LOCK) {
            STAGING.clear();
            STAGING.put(payload, 0, bytes);
            return nativeEncodeIngestDirect(STAGING, 0, bytes, flags, source, sequence);
        }
    }

    public static int ingestFrameDirect(ByteBuffer frame, int bytes) {
        if (!AVAILABLE) return E_STATE;
        if (frame == null || !frame.isDirect()) return E_NULL;
        int offset = frame.position();
        if (bytes < 0 || offset < 0 || offset + bytes > frame.capacity()) return E_RANGE;
        synchronized (LOCK) {
            return nativeIngestFrameDirect(frame, offset, bytes);
        }
    }

    public static int stateDigest() {
        return AVAILABLE ? nativeStateDigest() : 0;
    }

    public static int architectureId() {
        return AVAILABLE ? nativeArchitecture() : 0;
    }

    public static int maxPayload() {
        return AVAILABLE ? nativeMaxPayload() : MAX_PAYLOAD;
    }

    public static int acceptedCount() {
        return AVAILABLE ? nativeAccepted() : 0;
    }

    public static int rejectedCount() {
        return AVAILABLE ? nativeRejected() : 0;
    }
}
