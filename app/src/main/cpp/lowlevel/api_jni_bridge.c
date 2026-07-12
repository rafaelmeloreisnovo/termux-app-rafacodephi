/* api_jni_bridge.c — JNI bridge for api_lowlevel and bench_vectras_port
 * zero-copy DirectByteBuffer · no NewByteArray in hot path · jlong packed
 */
#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include "api_lowlevel.h"
#include "bench_vectras_port.h"

/* ── ApiLowLevelBridge JNI functions ────────────────────────────────────── */

/*
 * Java: static native long nativeInit()
 * Initializes the api_lowlevel BSS state.
 */
JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeInit(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    api_ll_init();
    return (jlong)0x4C4C494E49544C4CULL; /* "LLINTLL" magic */
}

/*
 * Java: static native long nativeDispatch(ByteBuffer payload, int apiId)
 * Zero-copy: reads directly from DirectByteBuffer, no allocation.
 * Returns packed jlong: hi32=crc32c lo32=event_count
 */
JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeDispatch(
        JNIEnv *env, jclass cls, jobject payload, jint api_id) {
    (void)cls;
    if (!payload) return (jlong)-1;
    const uint8_t *buf = (const uint8_t*)(*env)->GetDirectBufferAddress(env, payload);
    jlong cap = (*env)->GetDirectBufferCapacity(env, payload);
    if (!buf || cap <= 0) return (jlong)-2;
    return (jlong)api_ll_dispatch((uint8_t)(api_id & 0xFF), buf, (uint32_t)cap);
}

/*
 * Java: static native long nativeStateQuery()
 * Returns packed state: hi32=phase<<24|attractor<<16|flags_lo8 lo32=event_count
 */
JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeStateQuery(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    const ApiLLState *s = api_ll_state();
    uint64_t hi = ((uint64_t)s->phase << 24u) |
                  ((uint64_t)s->attractor << 16u) |
                  ((uint64_t)(s->flags & 0xFFu) << 8u) |
                  (uint64_t)s->entropy;
    return (jlong)((hi << 32u) | (uint64_t)s->event_cnt);
}

/*
 * Java: static native long nativeCrc32c(ByteBuffer buf, int len)
 * Direct CRC32C (HW on arm64+crc, SW branchless fallback)
 */
JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeCrc32c(
        JNIEnv *env, jclass cls, jobject buf, jint len) {
    (void)cls;
    if (!buf) return (jlong)-1;
    const void *p = (*env)->GetDirectBufferAddress(env, buf);
    if (!p) return (jlong)-1;
    uint32_t n = (uint32_t)(len < 0 ? 0 : len);
#if defined(__aarch64__) && defined(HAS_CRC32C_HW)
    uint32_t crc = api_ll_crc32c_hw(0xFFFFFFFFu, p, n) ^ 0xFFFFFFFFu;
#else
    uint32_t crc = api_ll_crc32c_sw(p, n);
#endif
    return (jlong)crc;
}

/* ── BenchmarkMenuActivity JNI functions ─────────────────────────────────── */

/*
 * Java: static native long nativeBenchRun(int profile, int category)
 * Runs a single benchmark category.
 * Returns: packed jlong hi32=score lo32=cycles(>>8)
 */
JNIEXPORT jlong JNICALL
Java_com_termux_app_benchmark_BenchmarkMenuActivity_nativeBenchRun(
        JNIEnv *env, jclass cls, jint profile, jint category) {
    (void)env; (void)cls;
    VectrasBenchResult r;
    raf_vectras_bench_run((uint32_t)profile, (uint32_t)category, &r);
    /* pack: hi32=score lo32=cycles_hi (truncated — UI display only) */
    return (jlong)(((uint64_t)r.score << 32u) | (uint64_t)(r.cycles >> 8u));
}

/*
 * Java: static native long nativeCycleRead()
 * Reads ARM virtual cycle counter directly.
 */
JNIEXPORT jlong JNICALL
Java_com_termux_app_benchmark_BenchmarkMenuActivity_nativeCycleRead(
        JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
#if defined(__aarch64__)
    return (jlong)api_ll_cycle_read();
#else
    return (jlong)0;
#endif
}

/*
 * Java: static native int nativeHwCaps()
 * Returns bitmask of available HW features.
 */
JNIEXPORT jint JNICALL
Java_com_termux_app_benchmark_BenchmarkMenuActivity_nativeHwCaps(
        JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    uint32_t caps = 0u;
#if defined(__aarch64__)
    caps |= VECTRAS_CAP_CNTVCT;
    caps |= VECTRAS_CAP_NEON;
#endif
#if defined(HAS_CRC32C_HW)
    caps |= VECTRAS_CAP_CRC32C;
#endif
    return (jint)caps;
}
