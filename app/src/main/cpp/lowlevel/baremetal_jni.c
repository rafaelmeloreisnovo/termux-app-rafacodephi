/**
 * JNI Bridge for bare-metal operations
 * Minimal overhead, direct native calls
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */

#include <jni.h>
#include <stdint.h>
#include <stddef.h>
#include "baremetal.h"

#include "freestanding_log.h"
#include "freestanding_string.h"

#define LOG_TAG "TermuxBareMetal-JNI"
#define LOG_TAG_LEN 19

static inline void logd_msg(const char *msg) {
    if (msg) freestanding_log_info(msg, (uint32_t)freestanding_strlen(msg));
}

static inline void loge_msg(const char *msg) {
    if (msg) freestanding_log_error(msg, (uint32_t)freestanding_strlen(msg));
}

static void throw_illegal_argument(JNIEnv *env, const char *message) {
    jclass illegal_argument = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    if (illegal_argument) {
        (*env)->ThrowNew(env, illegal_argument, message);
        (*env)->DeleteLocalRef(env, illegal_argument);
    }
}

static void throw_illegal_state(JNIEnv *env, const char *message) {
    jclass illegal_state = (*env)->FindClass(env, "java/lang/IllegalStateException");
    if (illegal_state) {
        (*env)->ThrowNew(env, illegal_state, message);
        (*env)->DeleteLocalRef(env, illegal_state);
    }
}

/* ============================================================================
 * Architecture and Capability Detection
 * ========================================================================== */

JNIEXPORT jstring JNICALL
Java_com_termux_lowlevel_BareMetal_getArchitecture(JNIEnv *env, jclass clazz) {
    (void)clazz;
    const char* arch = get_arch_name();
    return (*env)->NewStringUTF(env, arch);
}

JNIEXPORT jint JNICALL
Java_com_termux_lowlevel_BareMetal_getCapabilities(JNIEnv *env, jclass clazz) {
    (void)env;
    (void)clazz;
    return (jint)get_arch_caps();
}

JNIEXPORT jintArray JNICALL
Java_com_termux_lowlevel_BareMetal_getCapabilitiesDetail(JNIEnv *env, jclass clazz) {
    (void)clazz;
    jint raw[4];
    raw[0] = (jint)get_arch_caps();
    raw[1] = (jint)get_arch_runtime_caps();
    raw[2] = (jint)get_arch_binary_caps();
    raw[3] = (jint)get_arch_runtime_caps_valid();

    jintArray out = (*env)->NewIntArray(env, 4);
    if (out == NULL) {
        return NULL;
    }
    (*env)->SetIntArrayRegion(env, out, 0, 4, raw);
    return out;
}


JNIEXPORT jstring JNICALL
Java_com_termux_lowlevel_BareMetal_getHardwareProfile(JNIEnv *env, jclass clazz) {
    (void)clazz;
    hw_profile_t p;
    get_hw_profile(&p);

    /* Build profile string without snprintf — simplified format */
    char out[256];
    size_t off = 0;

    /* abi= */
    freestanding_memcpy(out + off, "abi=", 4); off += 4;
    uint32_t abi_len = (uint32_t)freestanding_strlen(p.abi);
    freestanding_memcpy(out + off, p.abi, abi_len); off += abi_len;

    /* ;page_size= */
    freestanding_memcpy(out + off, ";page_size=", 11); off += 11;
    /* page_size as decimal (max 5 digits) */
    char page_buf[8];
    int plen = 0;
    uint32_t ps = p.page_size;
    if (ps == 0) { page_buf[0] = '0'; plen = 1; }
    else {
        uint32_t d[] = {10000, 1000, 100, 10, 1};
        int started = 0;
        for (int i = 0; i < 5; i++) {
            int digit = ps / d[i];
            ps %= d[i];
            if (digit > 0 || started) {
                page_buf[plen++] = (char)('0' + digit);
                started = 1;
            }
        }
    }
    freestanding_memcpy(out + off, page_buf, plen); off += plen;

    /* ;clusters= */
    freestanding_memcpy(out + off, ";clusters=", 10); off += 10;
    uint32_t clusters_len = (uint32_t)freestanding_strlen(p.cpu_clusters[0] ? p.cpu_clusters : "n/a");
    freestanding_memcpy(out + off, p.cpu_clusters[0] ? p.cpu_clusters : "n/a", clusters_len); off += clusters_len;

    if (off >= sizeof(out)) {
        return (*env)->NewStringUTF(env, "abi=unknown");
    }
    out[off] = '\0';
    return (*env)->NewStringUTF(env, out);
}

/* ============================================================================
 * Vector Operations
 * ========================================================================== */

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_vectorDot(JNIEnv *env, jclass clazz,
                                               jfloatArray a, jfloatArray b) {
    (void)clazz;
    jsize len_a = (*env)->GetArrayLength(env, a);
    jsize len_b = (*env)->GetArrayLength(env, b);

    if (len_a != len_b) {
        loge_msg("Vector dimensions mismatch");
        jclass illegal_arg = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        if (illegal_arg) {
            (*env)->ThrowNew(env, illegal_arg,
                             "Vector dimensions mismatch: arrays must have equal length");
            (*env)->DeleteLocalRef(env, illegal_arg);
        }
        return 0.0f;
    }

    jfloat *pa = (*env)->GetPrimitiveArrayCritical(env, a, NULL);
    jfloat *pb = (*env)->GetPrimitiveArrayCritical(env, b, NULL);

    if (!pa || !pb) {
        if (pa) (*env)->ReleasePrimitiveArrayCritical(env, a, pa, JNI_ABORT);
        if (pb) (*env)->ReleasePrimitiveArrayCritical(env, b, pb, JNI_ABORT);

        jclass illegal_state = (*env)->FindClass(env, "java/lang/IllegalStateException");
        if (illegal_state) {
            (*env)->ThrowNew(env, illegal_state, "Failed to pin input arrays for vectorDot");
            (*env)->DeleteLocalRef(env, illegal_state);
        }
        return 0.0f;
    }

    float result = vop_dot(pa, pb, len_a);

    (*env)->ReleasePrimitiveArrayCritical(env, a, pa, JNI_ABORT);
    (*env)->ReleasePrimitiveArrayCritical(env, b, pb, JNI_ABORT);

    return result;
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_vectorNorm(JNIEnv *env, jclass clazz,
                                                jfloatArray a) {
    (void)clazz;
    jsize len = (*env)->GetArrayLength(env, a);
    jfloat *pa = (*env)->GetPrimitiveArrayCritical(env, a, NULL);
    
    if (!pa) return 0.0f;
    
    float result = vop_norm(pa, len);
    
    (*env)->ReleasePrimitiveArrayCritical(env, a, pa, JNI_ABORT);
    
    return result;
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_vectorAdd(JNIEnv *env, jclass clazz,
                                               jfloatArray a, jfloatArray b,
                                               jfloatArray result) {
    (void)clazz;
    jsize len_a = (*env)->GetArrayLength(env, a);
    jsize len_b = (*env)->GetArrayLength(env, b);
    jsize len_r = (*env)->GetArrayLength(env, result);

    if (len_a != len_b || len_a != len_r) {
        loge_msg("Vector dimensions mismatch");
        jclass illegal_arg = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        if (illegal_arg) {
            (*env)->ThrowNew(env, illegal_arg,
                             "Vector dimensions mismatch: arrays must have equal length");
            (*env)->DeleteLocalRef(env, illegal_arg);
        }
        return;
    }
    
    jfloat *pa = (*env)->GetPrimitiveArrayCritical(env, a, NULL);
    jfloat *pb = (*env)->GetPrimitiveArrayCritical(env, b, NULL);
    jfloat *pr = (*env)->GetPrimitiveArrayCritical(env, result, NULL);
    
    if (pa && pb && pr) {
        vop_add(pa, pb, pr, len_a);
    }
    
    if (pa) (*env)->ReleasePrimitiveArrayCritical(env, a, pa, JNI_ABORT);
    if (pb) (*env)->ReleasePrimitiveArrayCritical(env, b, pb, JNI_ABORT);
    if (pr) (*env)->ReleasePrimitiveArrayCritical(env, result, pr, 0);
}

/* ============================================================================
 * Matrix Operations
 * ========================================================================== */

JNIEXPORT jlong JNICALL
Java_com_termux_lowlevel_BareMetal_matrixCreate(JNIEnv *env, jclass clazz,
                                                  jint rows, jint cols) {
    (void)clazz;
    if (rows <= 0 || cols <= 0) {
        jclass illegal_arg = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        if (illegal_arg) {
            (*env)->ThrowNew(env, illegal_arg,
                             "Matrix dimensions must be > 0");
            (*env)->DeleteLocalRef(env, illegal_arg);
        }
        return 0;
    }

    mx_t* m = mx_create(rows, cols);
    if (!m && !(*env)->ExceptionCheck(env)) {
        jclass oom = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        if (oom) {
            (*env)->ThrowNew(env, oom, "Failed to allocate matrix");
            (*env)->DeleteLocalRef(env, oom);
        }
    }
    return (jlong)(intptr_t)m;
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFree(JNIEnv *env, jclass clazz,
                                                jlong handle) {
    (void)env;
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        return;
    }
    mx_free(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixMultiply(JNIEnv *env, jclass clazz,
                                                    jlong handleA, jlong handleB,
                                                    jlong handleResult) {
    (void)clazz;
    mx_t* a = (mx_t*)(intptr_t)handleA;
    mx_t* b = (mx_t*)(intptr_t)handleB;
    mx_t* r = (mx_t*)(intptr_t)handleResult;
    if (!a || !b || !r) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    
    if (a->c != b->r || r->r != a->r || r->c != b->c) {
        throw_illegal_argument(env, "Invalid matrix dimensions for multiplication");
        return;
    }
    mx_mul(a, b, r);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFlipHorizontal(JNIEnv *env, jclass clazz,
                                                          jlong handle) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_flip_h(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFlipVertical(JNIEnv *env, jclass clazz,
                                                        jlong handle) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_flip_v(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFlipDiagonal(JNIEnv *env, jclass clazz,
                                                        jlong handle) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_flip_d(m);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_matrixDeterminant(JNIEnv *env, jclass clazz,
                                                       jlong handle) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return 0.0f;
    }
    return mx_det(m);
}

JNIEXPORT jint JNICALL
Java_com_termux_lowlevel_BareMetal_matrixInvert(JNIEnv *env, jclass clazz,
                                                  jlong handle, jlong handleResult) {
    (void)env;
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    mx_t* r = (mx_t*)(intptr_t)handleResult;
    if (!m || !r) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return -1;
    }
    if (m->r != m->c || r->r != m->r || r->c != m->c) {
        throw_illegal_argument(env, "Invalid matrix dimensions for inversion");
        return -1;
    }
    return mx_inv(m, r);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixAdd(JNIEnv *env, jclass clazz,
                                               jlong handleA, jlong handleB,
                                               jlong handleResult) {
    (void)clazz;
    mx_t* a = (mx_t*)(intptr_t)handleA;
    mx_t* b = (mx_t*)(intptr_t)handleB;
    mx_t* r = (mx_t*)(intptr_t)handleResult;
    if (!a || !b || !r) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_add(a, b, r);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixSubtract(JNIEnv *env, jclass clazz,
                                                    jlong handleA, jlong handleB,
                                                    jlong handleResult) {
    (void)clazz;
    mx_t* a = (mx_t*)(intptr_t)handleA;
    mx_t* b = (mx_t*)(intptr_t)handleB;
    mx_t* r = (mx_t*)(intptr_t)handleResult;
    if (!a || !b || !r) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_sub(a, b, r);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixScale(JNIEnv *env, jclass clazz,
                                                 jlong handle, jfloat scalar) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_scale(m, scalar);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_matrixTrace(JNIEnv *env, jclass clazz,
                                                 jlong handle) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return 0.0f;
    }
    return mx_trace(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixIdentity(JNIEnv *env, jclass clazz,
                                                    jlong handle) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_identity(m);
}

JNIEXPORT jint JNICALL
Java_com_termux_lowlevel_BareMetal_matrixSolveLinear(JNIEnv *env, jclass clazz,
                                                       jlong handle, jfloatArray b,
                                                       jfloatArray x) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return -1;
    }
    
    jsize len_b = (*env)->GetArrayLength(env, b);
    jsize len_x = (*env)->GetArrayLength(env, x);
    
    if (len_b != (jsize)m->r || len_x != (jsize)m->c) {
        loge_msg("Size mismatch in matrixSolveLinear");
        throw_illegal_argument(env, "Invalid dimensions for linear solve");
        return -1;
    }
    
    jfloat *pb = (*env)->GetPrimitiveArrayCritical(env, b, NULL);
    jfloat *px = (*env)->GetPrimitiveArrayCritical(env, x, NULL);
    
    int result = -1;
    if (pb && px) {
        result = mx_solve_linear(m, pb, px);
    }
    
    if (pb) (*env)->ReleasePrimitiveArrayCritical(env, b, pb, JNI_ABORT);
    if (px) (*env)->ReleasePrimitiveArrayCritical(env, x, px, 0);
    
    return result;
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixTranspose(JNIEnv *env, jclass clazz,
                                                     jlong handle, jlong handleResult) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    mx_t* r = (mx_t*)(intptr_t)handleResult;
    if (!m || !r) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    mx_transpose(m, r);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixGetData(JNIEnv *env, jclass clazz,
                                                   jlong handle, jfloatArray data) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    jsize len = (*env)->GetArrayLength(env, data);
    
    if (len < (jsize)(m->r * m->c)) {
        loge_msg("Array too small for matrix data");
        throw_illegal_argument(env, "Array too small for matrix data");
        return;
    }
    
    jfloat *pd = (*env)->GetPrimitiveArrayCritical(env, data, NULL);
    if (pd) {
        bmem_cpy(pd, m->m, m->r * m->c * sizeof(float));
        (*env)->ReleasePrimitiveArrayCritical(env, data, pd, 0);
    }
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixSetData(JNIEnv *env, jclass clazz,
                                                   jlong handle, jfloatArray data) {
    (void)clazz;
    mx_t* m = (mx_t*)(intptr_t)handle;
    if (!m) {
        throw_illegal_state(env, "Matrix is closed or invalid");
        return;
    }
    jsize len = (*env)->GetArrayLength(env, data);
    
    if (len < (jsize)(m->r * m->c)) {
        loge_msg("Array too small for matrix data");
        throw_illegal_argument(env, "Array too small for matrix data");
        return;
    }
    
    jfloat *pd = (*env)->GetPrimitiveArrayCritical(env, data, NULL);
    if (pd) {
        bmem_cpy(m->m, pd, m->r * m->c * sizeof(float));
        (*env)->ReleasePrimitiveArrayCritical(env, data, pd, JNI_ABORT);
    }
}

JNIEXPORT jlong JNICALL
Java_com_termux_lowlevel_BareMetal_arenaCreate(JNIEnv *env, jclass clazz, jlong capacityBytes) {
    (void)clazz;
    if (capacityBytes <= 0) {
        throw_illegal_argument(env, "Arena capacity must be > 0");
        return 0;
    }
    mx_arena_t* arena = arena_create((size_t)capacityBytes);
    if (!arena) throw_illegal_state(env, "Failed to allocate native arena");
    return (jlong)(intptr_t)arena;
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_arenaReset(JNIEnv *env, jclass clazz, jlong arenaHandle) {
    (void)env; (void)clazz;
    mx_arena_t* arena = (mx_arena_t*)(intptr_t)arenaHandle;
    if (arena) arena_reset(arena);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_arenaDestroy(JNIEnv *env, jclass clazz, jlong arenaHandle) {
    (void)env; (void)clazz;
    mx_arena_t* arena = (mx_arena_t*)(intptr_t)arenaHandle;
    if (arena) arena_destroy(arena);
}

JNIEXPORT jlong JNICALL
Java_com_termux_lowlevel_BareMetal_matrixCreateInArena(JNIEnv *env, jclass clazz, jlong arenaHandle, jint rows, jint cols) {
    (void)clazz;
    mx_arena_t* arena = (mx_arena_t*)(intptr_t)arenaHandle;
    if (!arena) { throw_illegal_state(env, "Arena is closed or invalid"); return 0; }
    if (rows <= 0 || cols <= 0) { throw_illegal_argument(env, "Matrix dimensions must be > 0"); return 0; }
    mx_t* m = mx_create_in_arena(arena, (uint32_t)rows, (uint32_t)cols);
    if (!m) throw_illegal_state(env, "Failed to allocate matrix in arena");
    return (jlong)(intptr_t)m;
}

/* ============================================================================
 * Fast Math Operations
 * ========================================================================== */

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastSqrt(JNIEnv *env, jclass clazz, jfloat x) {
    (void)env;
    (void)clazz;
    return fm_sqrt(x);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastRsqrt(JNIEnv *env, jclass clazz, jfloat x) {
    (void)env;
    (void)clazz;
    return fm_rsqrt(x);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastExp(JNIEnv *env, jclass clazz, jfloat x) {
    (void)env;
    (void)clazz;
    return fm_exp(x);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastLog(JNIEnv *env, jclass clazz, jfloat x) {
    (void)env;
    (void)clazz;
    return fm_log(x);
}

/* ============================================================================
 * Memory Operations
 * ========================================================================== */

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_memCopy(JNIEnv *env, jclass clazz,
                                             jbyteArray dst, jbyteArray src) {
    (void)clazz;
    jsize src_len = (*env)->GetArrayLength(env, src);
    jsize dst_len = (*env)->GetArrayLength(env, dst);

    if (dst_len < src_len) {
        loge_msg("memCopy destination too small");
        jclass illegal_arg = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        if (illegal_arg) {
            (*env)->ThrowNew(env, illegal_arg,
                             "Destination array length must be >= source array length");
            (*env)->DeleteLocalRef(env, illegal_arg);
        }
        return;
    }

    jbyte *pd = (*env)->GetPrimitiveArrayCritical(env, dst, NULL);
    jbyte *ps = (*env)->GetPrimitiveArrayCritical(env, src, NULL);

    if (pd && ps) {
        bmem_cpy(pd, ps, src_len);
    } else {
        jclass illegal_state = (*env)->FindClass(env, "java/lang/IllegalStateException");
        if (illegal_state) {
            (*env)->ThrowNew(env, illegal_state, "Failed to pin arrays for memCopy");
            (*env)->DeleteLocalRef(env, illegal_state);
        }
    }

    if (pd) (*env)->ReleasePrimitiveArrayCritical(env, dst, pd, 0);
    if (ps) (*env)->ReleasePrimitiveArrayCritical(env, src, ps, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_memSet(JNIEnv *env, jclass clazz,
                                            jbyteArray array, jint value) {
    (void)clazz;
    jsize len = (*env)->GetArrayLength(env, array);
    jbyte *pa = (*env)->GetPrimitiveArrayCritical(env, array, NULL);
    
    if (pa) {
        bmem_set(pa, value, len);
        (*env)->ReleasePrimitiveArrayCritical(env, array, pa, 0);
    }
}

/* ============================================================================
 * JNI Registration
 * ========================================================================== */

static JNINativeMethod methods[] = {
    /* Architecture */
    {"getArchitecture", "()Ljava/lang/String;", (void*)Java_com_termux_lowlevel_BareMetal_getArchitecture},
    {"getCapabilities", "()I", (void*)Java_com_termux_lowlevel_BareMetal_getCapabilities},
    /* getCapabilitiesDetail(): int[] -> JNI signature ()[I */
    {"getCapabilitiesDetail", "()[I", (void*)Java_com_termux_lowlevel_BareMetal_getCapabilitiesDetail},
    {"getHardwareProfile", "()Ljava/lang/String;", (void*)Java_com_termux_lowlevel_BareMetal_getHardwareProfile},
    
    /* Vector ops */
    {"vectorDot", "([F[F)F", (void*)Java_com_termux_lowlevel_BareMetal_vectorDot},
    {"vectorNorm", "([F)F", (void*)Java_com_termux_lowlevel_BareMetal_vectorNorm},
    {"vectorAdd", "([F[F[F)V", (void*)Java_com_termux_lowlevel_BareMetal_vectorAdd},
    
    /* Matrix ops - basic */
    {"matrixCreate", "(II)J", (void*)Java_com_termux_lowlevel_BareMetal_matrixCreate},
    {"matrixCreateInArena", "(JII)J", (void*)Java_com_termux_lowlevel_BareMetal_matrixCreateInArena},
    {"arenaCreate", "(J)J", (void*)Java_com_termux_lowlevel_BareMetal_arenaCreate},
    {"arenaReset", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_arenaReset},
    {"arenaDestroy", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_arenaDestroy},
    {"matrixFree", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFree},
    {"matrixMultiply", "(JJJ)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixMultiply},
    {"matrixTranspose", "(JJ)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixTranspose},
    {"matrixGetData", "(J[F)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixGetData},
    {"matrixSetData", "(J[F)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixSetData},
    
    /* Matrix ops - flip operations (RAFAELIA deterministic method) */
    {"matrixFlipHorizontal", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFlipHorizontal},
    {"matrixFlipVertical", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFlipVertical},
    {"matrixFlipDiagonal", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFlipDiagonal},
    
    /* Matrix ops - advanced */
    {"matrixDeterminant", "(J)F", (void*)Java_com_termux_lowlevel_BareMetal_matrixDeterminant},
    {"matrixInvert", "(JJ)I", (void*)Java_com_termux_lowlevel_BareMetal_matrixInvert},
    {"matrixAdd", "(JJJ)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixAdd},
    {"matrixSubtract", "(JJJ)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixSubtract},
    {"matrixScale", "(JF)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixScale},
    {"matrixTrace", "(J)F", (void*)Java_com_termux_lowlevel_BareMetal_matrixTrace},
    {"matrixIdentity", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixIdentity},
    {"matrixSolveLinear", "(J[F[F)I", (void*)Java_com_termux_lowlevel_BareMetal_matrixSolveLinear},
    
    /* Fast math */
    {"fastSqrt", "(F)F", (void*)Java_com_termux_lowlevel_BareMetal_fastSqrt},
    {"fastRsqrt", "(F)F", (void*)Java_com_termux_lowlevel_BareMetal_fastRsqrt},
    {"fastExp", "(F)F", (void*)Java_com_termux_lowlevel_BareMetal_fastExp},
    {"fastLog", "(F)F", (void*)Java_com_termux_lowlevel_BareMetal_fastLog},
    
    /* Memory ops */
    {"memCopy", "([B[B)V", (void*)Java_com_termux_lowlevel_BareMetal_memCopy},
    {"memSet", "([BI)V", (void*)Java_com_termux_lowlevel_BareMetal_memSet},
};

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    
    jclass clazz = (*env)->FindClass(env, "com/termux/lowlevel/BareMetal");
    if (clazz == NULL) {
        loge_msg("Cannot find BareMetal class");
        return JNI_ERR;
    }
    
    if ((*env)->RegisterNatives(env, clazz, methods, 
                                 sizeof(methods)/sizeof(methods[0])) < 0) {
        loge_msg("Failed to register native methods");
        return JNI_ERR;
    }
    
    { char buf[128]; freestanding_strcpy(buf, "BareMetal JNI loaded - "); freestanding_strcat(buf, get_arch_name()); logd_msg(buf); }
    
    return JNI_VERSION_1_6;
}
