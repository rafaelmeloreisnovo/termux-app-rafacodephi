#include <jni.h>
#include "rafz.h"

/* Android shell only. The imported core remains heap-free and OS-free. */
static rafz_image g_image;
static rafz_u8 g_frame[RAFZ_SLOT_BYTES];
static volatile rafz_u8 g_lock;
static volatile rafz_u8 g_ready;

static void rafz_android_lock(void) {
    while (__atomic_test_and_set(&g_lock, __ATOMIC_ACQUIRE)) { }
}

static void rafz_android_unlock(void) {
    __atomic_clear(&g_lock, __ATOMIC_RELEASE);
}

static rafz_status rafz_android_ensure_ready(void) {
    rafz_status status = RAFZ_OK;
    if (!g_ready) {
        status = rafz_image_init(&g_image);
        g_ready = (rafz_u8)(status == RAFZ_OK);
    }
    return status;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeInit(JNIEnv *env, jclass type) {
    rafz_status status;
    (void)env;
    (void)type;
    rafz_android_lock();
    status = rafz_selfcheck();
    if (status == RAFZ_OK) status = rafz_image_init(&g_image);
    g_ready = (rafz_u8)(status == RAFZ_OK);
    rafz_android_unlock();
    return (jint)status;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeEncodeIngestDirect(
    JNIEnv *env,
    jclass type,
    jobject direct_buffer,
    jint offset,
    jint payload_bytes,
    jint flags,
    jlong source,
    jlong sequence) {
    rafz_u8 *base;
    jlong capacity;
    rafz_status status;
    rafz_u32 frame_bytes = 0u;
    __UINT64_TYPE__ source_bits = (__UINT64_TYPE__)source;
    __UINT64_TYPE__ sequence_bits = (__UINT64_TYPE__)sequence;
    (void)type;
    if (direct_buffer == (jobject)0 || offset < 0 || payload_bytes < 0) return (jint)RAFZ_E_NULL;
    base = (rafz_u8 *)(*env)->GetDirectBufferAddress(env, direct_buffer);
    capacity = (*env)->GetDirectBufferCapacity(env, direct_buffer);
    if (base == (rafz_u8 *)0 || capacity < (jlong)offset + (jlong)payload_bytes) return (jint)RAFZ_E_SIZE;
    if ((rafz_u32)payload_bytes > RAFZ_MAX_PAYLOAD) return (jint)RAFZ_E_RANGE;

    rafz_android_lock();
    status = rafz_android_ensure_ready();
    if (status == RAFZ_OK) {
        status = rafz_frame_encode(
            g_frame,
            (rafz_u32)sizeof(g_frame),
            base + (rafz_u32)offset,
            (rafz_u32)payload_bytes,
            (rafz_u32)flags,
            (rafz_u32)source_bits,
            (rafz_u32)(source_bits >> 32u),
            (rafz_u32)sequence_bits,
            (rafz_u32)(sequence_bits >> 32u),
            &frame_bytes);
    }
    if (status == RAFZ_OK) status = rafz_ingest(&g_image.ctx, g_frame, frame_bytes);
    rafz_android_unlock();
    return (jint)status;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeIngestFrameDirect(
    JNIEnv *env,
    jclass type,
    jobject direct_buffer,
    jint offset,
    jint frame_bytes) {
    rafz_u8 *base;
    jlong capacity;
    rafz_status status;
    (void)type;
    if (direct_buffer == (jobject)0 || offset < 0 || frame_bytes < 0) return (jint)RAFZ_E_NULL;
    base = (rafz_u8 *)(*env)->GetDirectBufferAddress(env, direct_buffer);
    capacity = (*env)->GetDirectBufferCapacity(env, direct_buffer);
    if (base == (rafz_u8 *)0 || capacity < (jlong)offset + (jlong)frame_bytes) return (jint)RAFZ_E_SIZE;

    rafz_android_lock();
    status = rafz_android_ensure_ready();
    if (status == RAFZ_OK) status = rafz_ingest(&g_image.ctx, base + (rafz_u32)offset, (rafz_u32)frame_bytes);
    rafz_android_unlock();
    return (jint)status;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeStateDigest(JNIEnv *env, jclass type) {
    rafz_u32 digest;
    (void)env;
    (void)type;
    rafz_android_lock();
    digest = g_ready ? rafz_state_digest(&g_image.ctx) : 0u;
    rafz_android_unlock();
    return (jint)digest;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeArchitecture(JNIEnv *env, jclass type) {
    (void)env;
    (void)type;
    return (jint)rafz_get_build_info().arch_id;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeMaxPayload(JNIEnv *env, jclass type) {
    (void)env;
    (void)type;
    return (jint)RAFZ_MAX_PAYLOAD;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeAccepted(JNIEnv *env, jclass type) {
    jint value;
    (void)env;
    (void)type;
    rafz_android_lock();
    value = g_ready ? (jint)g_image.ctx.accepted : 0;
    rafz_android_unlock();
    return value;
}

JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_RafaeliaZero_nativeRejected(JNIEnv *env, jclass type) {
    jint value;
    (void)env;
    (void)type;
    rafz_android_lock();
    value = g_ready ? (jint)g_image.ctx.rejected : 0;
    rafz_android_unlock();
    return value;
}
