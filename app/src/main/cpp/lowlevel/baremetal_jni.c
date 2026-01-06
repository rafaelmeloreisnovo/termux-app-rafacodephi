/**
 * JNI Bridge for bare-metal operations
 * Minimal overhead, direct native calls
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */

#include <jni.h>
#include <android/log.h>
#include "baremetal.h"

#define LOG_TAG "TermuxBareMetal-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* ============================================================================
 * Architecture and Capability Detection
 * ========================================================================== */

JNIEXPORT jstring JNICALL
Java_com_termux_lowlevel_BareMetal_getArchitecture(JNIEnv *env, jclass clazz) {
    const char* arch = get_arch_name();
    return (*env)->NewStringUTF(env, arch);
}

JNIEXPORT jint JNICALL
Java_com_termux_lowlevel_BareMetal_getCapabilities(JNIEnv *env, jclass clazz) {
    return (jint)get_arch_caps();
}

/* ============================================================================
 * Vector Operations
 * ========================================================================== */

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_vectorDot(JNIEnv *env, jclass clazz,
                                               jfloatArray a, jfloatArray b) {
    jsize len_a = (*env)->GetArrayLength(env, a);
    jsize len_b = (*env)->GetArrayLength(env, b);
    
    if (len_a != len_b) {
        LOGE("Vector dimensions mismatch: %d != %d", len_a, len_b);
        return 0.0f;
    }
    
    jfloat *pa = (*env)->GetPrimitiveArrayCritical(env, a, NULL);
    jfloat *pb = (*env)->GetPrimitiveArrayCritical(env, b, NULL);
    
    if (!pa || !pb) {
        if (pa) (*env)->ReleasePrimitiveArrayCritical(env, a, pa, JNI_ABORT);
        if (pb) (*env)->ReleasePrimitiveArrayCritical(env, b, pb, JNI_ABORT);
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
    jsize len = (*env)->GetArrayLength(env, a);
    
    jfloat *pa = (*env)->GetPrimitiveArrayCritical(env, a, NULL);
    jfloat *pb = (*env)->GetPrimitiveArrayCritical(env, b, NULL);
    jfloat *pr = (*env)->GetPrimitiveArrayCritical(env, result, NULL);
    
    if (pa && pb && pr) {
        vop_add(pa, pb, pr, len);
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
    mx_t* m = mx_create(rows, cols);
    return (jlong)(intptr_t)m;
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFree(JNIEnv *env, jclass clazz,
                                                jlong handle) {
    mx_t* m = (mx_t*)(intptr_t)handle;
    mx_free(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixMultiply(JNIEnv *env, jclass clazz,
                                                    jlong handleA, jlong handleB,
                                                    jlong handleResult) {
    mx_t* a = (mx_t*)(intptr_t)handleA;
    mx_t* b = (mx_t*)(intptr_t)handleB;
    mx_t* r = (mx_t*)(intptr_t)handleResult;
    
    mx_mul(a, b, r);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFlipHorizontal(JNIEnv *env, jclass clazz,
                                                          jlong handle) {
    mx_t* m = (mx_t*)(intptr_t)handle;
    mx_flip_h(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFlipVertical(JNIEnv *env, jclass clazz,
                                                        jlong handle) {
    mx_t* m = (mx_t*)(intptr_t)handle;
    mx_flip_v(m);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_matrixFlipDiagonal(JNIEnv *env, jclass clazz,
                                                        jlong handle) {
    mx_t* m = (mx_t*)(intptr_t)handle;
    mx_flip_d(m);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_matrixDeterminant(JNIEnv *env, jclass clazz,
                                                       jlong handle) {
    mx_t* m = (mx_t*)(intptr_t)handle;
    return mx_det(m);
}

/* ============================================================================
 * Fast Math Operations
 * ========================================================================== */

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastSqrt(JNIEnv *env, jclass clazz, jfloat x) {
    return fm_sqrt(x);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastRsqrt(JNIEnv *env, jclass clazz, jfloat x) {
    return fm_rsqrt(x);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastExp(JNIEnv *env, jclass clazz, jfloat x) {
    return fm_exp(x);
}

JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_BareMetal_fastLog(JNIEnv *env, jclass clazz, jfloat x) {
    return fm_log(x);
}

/* ============================================================================
 * Memory Operations
 * ========================================================================== */

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_memCopy(JNIEnv *env, jclass clazz,
                                             jbyteArray dst, jbyteArray src) {
    jsize len = (*env)->GetArrayLength(env, src);
    
    jbyte *pd = (*env)->GetPrimitiveArrayCritical(env, dst, NULL);
    jbyte *ps = (*env)->GetPrimitiveArrayCritical(env, src, NULL);
    
    if (pd && ps) {
        bmem_cpy(pd, ps, len);
    }
    
    if (pd) (*env)->ReleasePrimitiveArrayCritical(env, dst, pd, 0);
    if (ps) (*env)->ReleasePrimitiveArrayCritical(env, src, ps, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_BareMetal_memSet(JNIEnv *env, jclass clazz,
                                            jbyteArray array, jint value) {
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
    
    /* Vector ops */
    {"vectorDot", "([F[F)F", (void*)Java_com_termux_lowlevel_BareMetal_vectorDot},
    {"vectorNorm", "([F)F", (void*)Java_com_termux_lowlevel_BareMetal_vectorNorm},
    {"vectorAdd", "([F[F[F)V", (void*)Java_com_termux_lowlevel_BareMetal_vectorAdd},
    
    /* Matrix ops */
    {"matrixCreate", "(II)J", (void*)Java_com_termux_lowlevel_BareMetal_matrixCreate},
    {"matrixFree", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFree},
    {"matrixMultiply", "(JJJ)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixMultiply},
    {"matrixFlipHorizontal", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFlipHorizontal},
    {"matrixFlipVertical", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFlipVertical},
    {"matrixFlipDiagonal", "(J)V", (void*)Java_com_termux_lowlevel_BareMetal_matrixFlipDiagonal},
    {"matrixDeterminant", "(J)F", (void*)Java_com_termux_lowlevel_BareMetal_matrixDeterminant},
    
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
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    
    jclass clazz = (*env)->FindClass(env, "com/termux/lowlevel/BareMetal");
    if (clazz == NULL) {
        LOGE("Cannot find BareMetal class");
        return JNI_ERR;
    }
    
    if ((*env)->RegisterNatives(env, clazz, methods, 
                                 sizeof(methods)/sizeof(methods[0])) < 0) {
        LOGE("Failed to register native methods");
        return JNI_ERR;
    }
    
    LOGD("BareMetal JNI loaded successfully - Architecture: %s", get_arch_name());
    
    return JNI_VERSION_1_6;
}
