#include "lowlevel_jni.h"
#include "vectra_math.h"
#include <stdlib.h>
#include <string.h>

/**
 * JNI implementation for low-level bare-metal operations
 * Minimal overhead, direct native access
 */

// Initialize VA context
JNIEXPORT jlong JNICALL
Java_com_termux_lowlevel_VectraMath_initVA(JNIEnv *env, jclass clazz, 
                                            jint space_dim, jint feature_type) {
    va_context_t* ctx = (va_context_t*)malloc(sizeof(va_context_t));
    if (!ctx) return 0;
    
    va_init(ctx, (uint32_t)space_dim, (uint8_t)feature_type);
    return (jlong)ctx;
}

// Release VA context
JNIEXPORT void JNICALL
Java_com_termux_lowlevel_VectraMath_releaseVA(JNIEnv *env, jclass clazz, jlong ctx) {
    if (ctx) {
        free((void*)ctx);
    }
}

// Compute cosine similarity
JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_VectraMath_computeCosineSimilarity(JNIEnv *env, jclass clazz,
                                                             jfloatArray v1, jfloatArray v2) {
    if (!v1 || !v2) return 0.0f;
    
    jsize len1 = (*env)->GetArrayLength(env, v1);
    jsize len2 = (*env)->GetArrayLength(env, v2);
    
    if (len1 != len2 || len1 == 0) return 0.0f;
    
    // Get direct pointer access for minimal overhead
    jfloat* arr1 = (*env)->GetFloatArrayElements(env, v1, NULL);
    jfloat* arr2 = (*env)->GetFloatArrayElements(env, v2, NULL);
    
    if (!arr1 || !arr2) {
        if (arr1) (*env)->ReleaseFloatArrayElements(env, v1, arr1, JNI_ABORT);
        if (arr2) (*env)->ReleaseFloatArrayElements(env, v2, arr2, JNI_ABORT);
        return 0.0f;
    }
    
    float result = va_compute_cosine_similarity(arr1, arr2, (size_t)len1);
    
    // Release without copying back (JNI_ABORT)
    (*env)->ReleaseFloatArrayElements(env, v1, arr1, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, v2, arr2, JNI_ABORT);
    
    return result;
}

// Compute Euclidean distance
JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_VectraMath_computeEuclideanDistance(JNIEnv *env, jclass clazz,
                                                              jfloatArray v1, jfloatArray v2) {
    if (!v1 || !v2) return 0.0f;
    
    jsize len1 = (*env)->GetArrayLength(env, v1);
    jsize len2 = (*env)->GetArrayLength(env, v2);
    
    if (len1 != len2 || len1 == 0) return 0.0f;
    
    jfloat* arr1 = (*env)->GetFloatArrayElements(env, v1, NULL);
    jfloat* arr2 = (*env)->GetFloatArrayElements(env, v2, NULL);
    
    if (!arr1 || !arr2) {
        if (arr1) (*env)->ReleaseFloatArrayElements(env, v1, arr1, JNI_ABORT);
        if (arr2) (*env)->ReleaseFloatArrayElements(env, v2, arr2, JNI_ABORT);
        return 0.0f;
    }
    
    float result = va_compute_euclidean_distance(arr1, arr2, (size_t)len1);
    
    (*env)->ReleaseFloatArrayElements(env, v1, arr1, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, v2, arr2, JNI_ABORT);
    
    return result;
}

// Test reversal invariance
JNIEXPORT jboolean JNICALL
Java_com_termux_lowlevel_VectraMath_testReversalInvariance(JNIEnv *env, jclass clazz,
                                                            jfloatArray v, jfloat threshold) {
    if (!v) return JNI_FALSE;
    
    jsize len = (*env)->GetArrayLength(env, v);
    if (len == 0) return JNI_FALSE;
    
    jfloat* arr = (*env)->GetFloatArrayElements(env, v, NULL);
    if (!arr) return JNI_FALSE;
    
    uint8_t result = va_test_reversal_invariance(arr, (size_t)len, threshold);
    
    (*env)->ReleaseFloatArrayElements(env, v, arr, JNI_ABORT);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

// Fit least squares model
JNIEXPORT jobject JNICALL
Java_com_termux_lowlevel_VectraMath_fitLeastSquares(JNIEnv *env, jclass clazz,
                                                     jfloatArray x, jfloatArray y) {
    if (!x || !y) return NULL;
    
    jsize len_x = (*env)->GetArrayLength(env, x);
    jsize len_y = (*env)->GetArrayLength(env, y);
    
    if (len_x != len_y || len_x < 2) return NULL;
    
    jfloat* arr_x = (*env)->GetFloatArrayElements(env, x, NULL);
    jfloat* arr_y = (*env)->GetFloatArrayElements(env, y, NULL);
    
    if (!arr_x || !arr_y) {
        if (arr_x) (*env)->ReleaseFloatArrayElements(env, x, arr_x, JNI_ABORT);
        if (arr_y) (*env)->ReleaseFloatArrayElements(env, y, arr_y, JNI_ABORT);
        return NULL;
    }
    
    anova_result_t result;
    int status = anova_fit_least_squares(arr_x, arr_y, (size_t)len_x, &result);
    
    (*env)->ReleaseFloatArrayElements(env, x, arr_x, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, y, arr_y, JNI_ABORT);
    
    if (status != 0) return NULL;
    
    // Create Java AnovaResult object
    jclass resultClass = (*env)->FindClass(env, "com/termux/lowlevel/AnovaResult");
    if (!resultClass) {
        free(result.coefficients);
        return NULL;
    }
    
    jmethodID constructor = (*env)->GetMethodID(env, resultClass, "<init>", "([FFFF)V");
    if (!constructor) {
        free(result.coefficients);
        return NULL;
    }
    
    // Create coefficients array
    jfloatArray coeffs = (*env)->NewFloatArray(env, (jsize)result.coeff_count);
    if (coeffs) {
        (*env)->SetFloatArrayRegion(env, coeffs, 0, (jsize)result.coeff_count, result.coefficients);
    }
    
    jobject anovaResult = (*env)->NewObject(env, resultClass, constructor, 
                                           coeffs, 
                                           result.ss_total, 
                                           result.ss_model, 
                                           result.ss_error);
    
    free(result.coefficients);
    return anovaResult;
}

// Compute SS decomposition
JNIEXPORT jfloatArray JNICALL
Java_com_termux_lowlevel_VectraMath_computeSSDecomposition(JNIEnv *env, jclass clazz,
                                                            jfloatArray y, jfloatArray y_pred) {
    if (!y || !y_pred) return NULL;
    
    jsize len_y = (*env)->GetArrayLength(env, y);
    jsize len_pred = (*env)->GetArrayLength(env, y_pred);
    
    if (len_y != len_pred || len_y == 0) return NULL;
    
    jfloat* arr_y = (*env)->GetFloatArrayElements(env, y, NULL);
    jfloat* arr_pred = (*env)->GetFloatArrayElements(env, y_pred, NULL);
    
    if (!arr_y || !arr_pred) {
        if (arr_y) (*env)->ReleaseFloatArrayElements(env, y, arr_y, JNI_ABORT);
        if (arr_pred) (*env)->ReleaseFloatArrayElements(env, y_pred, arr_pred, JNI_ABORT);
        return NULL;
    }
    
    float ss_t, ss_m, ss_e;
    anova_compute_ss_decomposition(arr_y, arr_pred, (size_t)len_y, &ss_t, &ss_m, &ss_e);
    
    (*env)->ReleaseFloatArrayElements(env, y, arr_y, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, y_pred, arr_pred, JNI_ABORT);
    
    // Return [ss_t, ss_m, ss_e]
    jfloatArray result = (*env)->NewFloatArray(env, 3);
    if (result) {
        float values[3] = {ss_t, ss_m, ss_e};
        (*env)->SetFloatArrayRegion(env, result, 0, 3, values);
    }
    
    return result;
}

// Optimized memcpy
JNIEXPORT void JNICALL
Java_com_termux_lowlevel_LowLevelUtils_memcpyOptimized(JNIEnv *env, jclass clazz,
                                                        jbyteArray dest, jbyteArray src, jint n) {
    if (!dest || !src || n <= 0) return;
    
    jsize dest_len = (*env)->GetArrayLength(env, dest);
    jsize src_len = (*env)->GetArrayLength(env, src);
    
    if (n > dest_len || n > src_len) return;
    
    jbyte* dest_ptr = (*env)->GetByteArrayElements(env, dest, NULL);
    jbyte* src_ptr = (*env)->GetByteArrayElements(env, src, NULL);
    
    if (dest_ptr && src_ptr) {
        va_memcpy_optimized(dest_ptr, src_ptr, (size_t)n);
    }
    
    if (dest_ptr) (*env)->ReleaseByteArrayElements(env, dest, dest_ptr, 0);
    if (src_ptr) (*env)->ReleaseByteArrayElements(env, src, src_ptr, JNI_ABORT);
}

// Optimized memset
JNIEXPORT void JNICALL
Java_com_termux_lowlevel_LowLevelUtils_memsetOptimized(JNIEnv *env, jclass clazz,
                                                        jbyteArray array, jint value, jint n) {
    if (!array || n <= 0) return;
    
    jsize len = (*env)->GetArrayLength(env, array);
    if (n > len) n = len;
    
    jbyte* ptr = (*env)->GetByteArrayElements(env, array, NULL);
    if (ptr) {
        va_memset_optimized(ptr, value, (size_t)n);
        (*env)->ReleaseByteArrayElements(env, array, ptr, 0);
    }
}

// Fast square root
JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_LowLevelUtils_sqrtFast(JNIEnv *env, jclass clazz, jfloat x) {
    return va_sqrt_fast(x);
}
