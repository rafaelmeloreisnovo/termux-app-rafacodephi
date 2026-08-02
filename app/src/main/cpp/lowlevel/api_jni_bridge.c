/* api_jni_bridge.c — existing Termux API low-level bridge only.
 * PA benchmark execution is not exposed through JNI.
 */
#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include "api_lowlevel.h"

JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeInit(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    api_ll_init();
    return (jlong)0x4C4C494E49544C4CULL;
}

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

JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeStateQuery(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    const ApiLLState *state = api_ll_state();
    uint64_t hi = ((uint64_t)state->phase << 24u) |
                  ((uint64_t)state->attractor << 16u) |
                  ((uint64_t)(state->flags & 0xFFu) << 8u) |
                  (uint64_t)state->entropy;
    return (jlong)((hi << 32u) | (uint64_t)state->event_cnt);
}

JNIEXPORT jlong JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeCrc32c(
        JNIEnv *env, jclass cls, jobject buf, jint len) {
    (void)cls;
    if (!buf) return (jlong)-1;
    const void *ptr = (*env)->GetDirectBufferAddress(env, buf);
    if (!ptr) return (jlong)-1;
    uint32_t size = (uint32_t)(len < 0 ? 0 : len);
#if defined(__aarch64__) && defined(HAS_CRC32C_HW)
    uint32_t crc = api_ll_crc32c_hw(0xFFFFFFFFu, ptr, size) ^ 0xFFFFFFFFu;
#else
    uint32_t crc = api_ll_crc32c_sw(ptr, size);
#endif
    return (jlong)crc;
}

JNIEXPORT jint JNICALL
Java_com_termux_app_api_ApiLowLevelBridge_nativeRecvFd(
        JNIEnv *env, jclass cls, jint fd, jint max_bytes) {
    (void)env; (void)cls;
    return (jint)api_ll_recv_fd((int)fd, (uint32_t)(max_bytes < 0 ? 0 : max_bytes));
}
