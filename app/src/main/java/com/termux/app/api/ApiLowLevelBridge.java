package com.termux.app.api;

import java.nio.ByteBuffer;

/**
 * Zero-overhead JNI bridge to the api_lowlevel native layer.
 * All calls use DirectByteBuffer — no heap allocation in the hot path.
 * Native implementation: app/src/main/cpp/lowlevel/api_jni_bridge.c
 */
public final class ApiLowLevelBridge {

    static {
        System.loadLibrary("api_lowlevel");
    }

    /* ── API method IDs — must mirror api_lowlevel.h defines ─────────────── */
    public static final int API_SENSOR       = 0x01;
    public static final int API_LOCATION     = 0x02;
    public static final int API_CAMERA_INFO  = 0x03;
    public static final int API_CAMERA_PHOTO = 0x04;
    public static final int API_VIBRATE      = 0x05;
    public static final int API_TORCH        = 0x06;
    public static final int API_WIFI_SCAN    = 0x07;
    public static final int API_WIFI_CONN    = 0x08;
    public static final int API_WIFI_ENABLE  = 0x09;
    public static final int API_IR_FREQ      = 0x0A;
    public static final int API_IR_TX        = 0x0B;
    public static final int API_SMS_SEND     = 0x0C;
    public static final int API_SMS_INBOX    = 0x0D;
    public static final int API_NFC          = 0x0E;
    public static final int API_FINGERPRINT  = 0x0F;
    public static final int API_TELEPHONY    = 0x10;
    public static final int API_MIC_RECORD   = 0x11;
    public static final int API_AUDIO_INFO   = 0x12;
    public static final int API_TTS          = 0x13;
    public static final int API_CONTACTS     = 0x14;
    public static final int API_CLIP_GET     = 0x15;
    public static final int API_CLIP_SET     = 0x16;
    public static final int API_BATTERY      = 0x17;
    public static final int API_BRIGHTNESS   = 0x18;
    public static final int API_VOLUME       = 0x19;
    public static final int API_NOTIFICATION = 0x1A;
    public static final int API_TOAST        = 0x1B;
    public static final int API_DIALOG       = 0x1C;
    public static final int API_USB          = 0x1D;
    public static final int API_STORAGE      = 0x1E;
    public static final int API_KEYSTORE     = 0x1F;
    public static final int API_CALL_LOG     = 0x20;
    public static final int API_TEL_DEVINFO  = 0x21;
    public static final int API_TEL_CELLINFO = 0x22;
    public static final int API_STT          = 0x23;

    /* ── Flag bits (mirroring api_lowlevel.h API_FL_*) ─────────────────── */
    public static final int FL_LOCK    = 1;
    public static final int FL_FLOW    = 1 << 1;
    public static final int FL_VOID    = 1 << 2;
    public static final int FL_TRICKST = 1 << 3;
    public static final int FL_VISCNEG = 1 << 4;
    public static final int FL_ATTJUMP = 1 << 5;
    public static final int FL_MERKLE  = 1 << 6;
    public static final int FL_GEOFAIL = 1 << 7;

    /* result packed format: hi32=crc32c lo32=event_count */
    public static int unpackCrc32c(long packed)     { return (int)(packed >>> 32); }
    public static int unpackEventCount(long packed) { return (int)(packed & 0xFFFFFFFFL); }

    /* ── JNI declarations ────────────────────────────────────────────────── */

    /** Initialize the freestanding BSS state. Call once at app start. */
    public static native long nativeInit();

    /**
     * Dispatch an API event via the branchless dispatch table.
     * payload must be a DirectByteBuffer (zero-copy, no allocation).
     * Returns packed jlong: hi32=CRC32C lo32=event_count
     */
    public static native long nativeDispatch(ByteBuffer payload, int apiId);

    /**
     * Query the current native state.
     * Returns packed jlong: hi32=phase|attractor|flags lo32=event_count
     */
    public static native long nativeStateQuery();

    /**
     * Hardware CRC32C of a DirectByteBuffer region.
     * Uses ARM64 crc32cx/crc32cw/crc32cb instructions when HAS_CRC32C_HW=1.
     */
    public static native long nativeCrc32c(ByteBuffer buf, int len);

    /* prevent instantiation */
    private ApiLowLevelBridge() {}
}
