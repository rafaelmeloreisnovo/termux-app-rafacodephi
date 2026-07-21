#include <jni.h>
#include "rafz.h"

/* APK runtime shell. The canonical RFZ1 core is compiled from :rafaelia. */
static rafz_image g_image;
static rafz_u8 g_frame[RAFZ_SLOT_BYTES];
static volatile rafz_u8 g_lock;
static volatile rafz_u8 g_ready;

static void runtime_lock(void) {
    while (__atomic_test_and_set(&g_lock, __ATOMIC_ACQUIRE)) { }
}

static void runtime_unlock(void) {
    __atomic_clear(&g_lock, __ATOMIC_RELEASE);
}

static rafz_status runtime_ensure_ready(void) {
    rafz_status status = RAFZ_OK;
    if (!g_ready) {
        status = rafz_image_init(&g_image);
        g_ready = (rafz_u8)(status == RAFZ_OK);
    }
    return status;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaZeroRuntime_nativeInit(JNIEnv *env, jclass type) {
    rafz_status status;
    (void)env;
    (void)type;
    runtime_lock();
    status = rafz_selfcheck();
    if (status == RAFZ_OK) status = rafz_image_init(&g_image);
    g_ready = (rafz_u8)(status == RAFZ_OK);
    runtime_unlock();
    return (jint)status;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaZeroRuntime_nativeEncodeIngestDirect(
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

    runtime_lock();
    status = runtime_ensure_ready();
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
    runtime_unlock();
    return (jint)status;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaZeroRuntime_nativeStateDigest(JNIEnv *env, jclass type) {
    rafz_u32 digest;
    (void)env;
    (void)type;
    runtime_lock();
    digest = g_ready ? rafz_state_digest(&g_image.ctx) : 0u;
    runtime_unlock();
    return (jint)digest;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaZeroRuntime_nativeArchitecture(JNIEnv *env, jclass type) {
    (void)env;
    (void)type;
    return (jint)rafz_get_build_info().arch_id;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaZeroRuntime_nativeAccepted(JNIEnv *env, jclass type) {
    jint value;
    (void)env;
    (void)type;
    runtime_lock();
    value = g_ready ? (jint)g_image.ctx.accepted : 0;
    runtime_unlock();
    return value;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaZeroRuntime_nativeRejected(JNIEnv *env, jclass type) {
    jint value;
    (void)env;
    (void)type;
    runtime_lock();
    value = g_ready ? (jint)g_image.ctx.rejected : 0;
    runtime_unlock();
    return value;
}
