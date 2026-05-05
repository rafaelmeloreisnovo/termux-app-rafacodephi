package com.termux.app;

import com.termux.rafacodephi.BuildConfig;
import com.termux.shared.logger.Logger;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

final class BootstrapBaremetalGuard {
    private static final String LOG_TAG = "BootstrapBaremetalGuard";
    private static final int BUFFER_CAPACITY = 2048;
    private static final ByteBuffer SHARED_BUFFER = ByteBuffer.allocateDirect(BUFFER_CAPACITY);
    private static final boolean LIB_LOADED;

    static {
        boolean loaded;
        try {
            System.loadLibrary("termux-baremetal");
            loaded = true;
        } catch (Throwable t) {
            loaded = false;
            Logger.logWarn(LOG_TAG, "Native guard unavailable: " + t.getMessage());
        }
        LIB_LOADED = loaded;
    }

    private BootstrapBaremetalGuard() {}

    private static native int selftestNative(ByteBuffer out, int cap);
    private static native int validatePrefixNative(String prefix, ByteBuffer out, int cap);

    static void selftest() {
        if (!LIB_LOADED) return;
        int rc = selftestNative(SHARED_BUFFER, BUFFER_CAPACITY);
        String json = readBufferString();
        if (rc < 0) {
            Logger.logWarn(LOG_TAG, "selftest failed rc=" + rc + " payload=" + json);
        } else {
            Logger.logInfo(LOG_TAG, "selftest ok payload=" + json);
        }
    }

    static void validateAfterBootstrap(String prefix) {
        if (!LIB_LOADED) {
            Logger.logWarn(LOG_TAG, "Skipped guard validation: native lib not loaded");
            return;
        }
        int rc = validatePrefixNative(prefix, SHARED_BUFFER, BUFFER_CAPACITY);
        String json = readBufferString();
        if (rc < 0) {
            String msg = "Guard validation failed rc=" + rc + " payload=" + json;
            if (BuildConfig.BOOTSTRAP_BAREMETAL_STRICT) {
                throw new RuntimeException(msg);
            }
            Logger.logWarn(LOG_TAG, msg + " (non-blocking beta)");
            return;
        }
        Logger.logInfo(LOG_TAG, "Guard validation OK payload=" + json);
    }

    private static String readBufferString() {
        byte[] data = new byte[BUFFER_CAPACITY];
        SHARED_BUFFER.position(0);
        SHARED_BUFFER.get(data);
        int len = 0;
        while (len < data.length && data[len] != 0) len++;
        return new String(data, 0, len, StandardCharsets.UTF_8);
    }
}
