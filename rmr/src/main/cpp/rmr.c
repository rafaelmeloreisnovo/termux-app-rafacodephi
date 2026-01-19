#include <jni.h>
#include <stdint.h>

#define RMR_HASH_INIT 0x811c9dc5u
#define RMR_HASH_PRIME 0x01000193u

static inline uint32_t rmr_flip_u32(uint32_t v) {
    __asm__ __volatile__("" : "+r"(v));
    return ((v & 0x000000ffu) << 24) |
           ((v & 0x0000ff00u) << 8) |
           ((v & 0x00ff0000u) >> 8) |
           ((v & 0xff000000u) >> 24);
}

static inline int rmr_clamp_i32(int v, int lo, int hi) {
    if (lo > hi) return v;
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline uint32_t rmr_hash_bytes(const uint8_t *p, uint32_t n) {
    uint32_t h = RMR_HASH_INIT;
    uint32_t i = 0;
    for (i = 0; i < n; ++i) {
        h ^= p[i];
        h *= RMR_HASH_PRIME;
    }
    return h;
}

static void rmr_flip_f32(float *p, uint32_t n) {
    uint32_t i = 0;
    uint32_t j = n ? n - 1 : 0;
    while (i < j) {
        float t = p[i];
        p[i] = p[j];
        p[j] = t;
        ++i;
        --j;
    }
}

JNIEXPORT jint JNICALL
Java_com_termux_rmr_RmrCore_nativeClamp(JNIEnv *e, jclass c, jint v, jint lo, jint hi) {
    (void)e;
    (void)c;
    return (jint) rmr_clamp_i32(v, lo, hi);
}

JNIEXPORT jint JNICALL
Java_com_termux_rmr_RmrCore_nativeStableHash(JNIEnv *e, jclass c, jstring s) {
    if (s == NULL) return 0;
    const char *p = (*e)->GetStringUTFChars(e, s, 0);
    if (p == NULL) return 0;
    uint32_t h = rmr_hash_bytes((const uint8_t *) p, (uint32_t) (*e)->GetStringUTFLength(e, s));
    (*e)->ReleaseStringUTFChars(e, s, p);
    return (jint) rmr_flip_u32(h);
}

JNIEXPORT void JNICALL
Java_com_termux_rmr_RmrCore_nativeFlipInPlace(JNIEnv *e, jclass c, jfloatArray a) {
    (void)c;
    if (a == NULL) return;
    jsize n = (*e)->GetArrayLength(e, a);
    if (n <= 1) return;
    jfloat *p = (*e)->GetFloatArrayElements(e, a, 0);
    if (p == NULL) return;
    rmr_flip_f32(p, (uint32_t) n);
    (*e)->ReleaseFloatArrayElements(e, a, p, 0);
}
