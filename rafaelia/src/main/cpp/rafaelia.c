#include <jni.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* Memory operations */

JNIEXPORT void JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_memcpy(JNIEnv *env, jclass clazz,
                                                jbyteArray dest, jbyteArray src, jint n) {
    (void)clazz;
    if (!dest || !src || n <= 0) return;
    
    jbyte *dest_ptr = (*env)->GetByteArrayElements(env, dest, NULL);
    jbyte *src_ptr = (*env)->GetByteArrayElements(env, src, NULL);
    
    if (dest_ptr && src_ptr) {
        memcpy(dest_ptr, src_ptr, (size_t)n);
    }
    
    if (dest_ptr) (*env)->ReleaseByteArrayElements(env, dest, dest_ptr, 0);
    if (src_ptr) (*env)->ReleaseByteArrayElements(env, src, src_ptr, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_memset(JNIEnv *env, jclass clazz,
                                                jbyteArray array, jint value, jint n) {
    (void)clazz;
    if (!array || n <= 0) return;
    
    jbyte *array_ptr = (*env)->GetByteArrayElements(env, array, NULL);
    
    if (array_ptr) {
        memset(array_ptr, (int)value, (size_t)n);
        (*env)->ReleaseByteArrayElements(env, array, array_ptr, 0);
    }
}

/* Mathematical operations */

JNIEXPORT jfloat JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_sqrt(JNIEnv *env, jclass clazz, jfloat x) {
    (void)env;
    (void)clazz;
    if (x < 0.0f) return 0.0f;
    return sqrtf(x);
}

/* Vector operations - these would use SIMD in production */

JNIEXPORT jfloat JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_cosineSimilarity(JNIEnv *env, jclass clazz,
                                                         jfloatArray v1, jfloatArray v2) {
    (void)clazz;
    if (!v1 || !v2) return 0.0f;
    
    jsize len1 = (*env)->GetArrayLength(env, v1);
    jsize len2 = (*env)->GetArrayLength(env, v2);
    
    if (len1 != len2 || len1 == 0) return 0.0f;
    
    jfloat *v1_ptr = (*env)->GetFloatArrayElements(env, v1, NULL);
    jfloat *v2_ptr = (*env)->GetFloatArrayElements(env, v2, NULL);
    
    if (!v1_ptr || !v2_ptr) {
        if (v1_ptr) (*env)->ReleaseFloatArrayElements(env, v1, v1_ptr, JNI_ABORT);
        if (v2_ptr) (*env)->ReleaseFloatArrayElements(env, v2, v2_ptr, JNI_ABORT);
        return 0.0f;
    }
    
    float dot = 0.0f, mag1 = 0.0f, mag2 = 0.0f;
    
    for (int i = 0; i < len1; i++) {
        dot += v1_ptr[i] * v2_ptr[i];
        mag1 += v1_ptr[i] * v1_ptr[i];
        mag2 += v2_ptr[i] * v2_ptr[i];
    }
    
    (*env)->ReleaseFloatArrayElements(env, v1, v1_ptr, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, v2, v2_ptr, JNI_ABORT);
    
    float denom = sqrtf(mag1) * sqrtf(mag2);
    if (denom < 1e-10f) return 0.0f;
    
    return dot / denom;
}

JNIEXPORT jfloat JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_euclideanDistance(JNIEnv *env, jclass clazz,
                                                          jfloatArray v1, jfloatArray v2) {
    (void)clazz;
    if (!v1 || !v2) return 0.0f;
    
    jsize len1 = (*env)->GetArrayLength(env, v1);
    jsize len2 = (*env)->GetArrayLength(env, v2);
    
    if (len1 != len2 || len1 == 0) return 0.0f;
    
    jfloat *v1_ptr = (*env)->GetFloatArrayElements(env, v1, NULL);
    jfloat *v2_ptr = (*env)->GetFloatArrayElements(env, v2, NULL);
    
    if (!v1_ptr || !v2_ptr) {
        if (v1_ptr) (*env)->ReleaseFloatArrayElements(env, v1, v1_ptr, JNI_ABORT);
        if (v2_ptr) (*env)->ReleaseFloatArrayElements(env, v2, v2_ptr, JNI_ABORT);
        return 0.0f;
    }
    
    float sum_sq = 0.0f;
    
    for (int i = 0; i < len1; i++) {
        float diff = v1_ptr[i] - v2_ptr[i];
        sum_sq += diff * diff;
    }
    
    (*env)->ReleaseFloatArrayElements(env, v1, v1_ptr, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, v2, v2_ptr, JNI_ABORT);
    
    return sqrtf(sum_sq);
}

JNIEXPORT jboolean JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_testReversalInvariance(JNIEnv *env, jclass clazz,
                                                                jfloatArray v, jfloat threshold) {
    (void)clazz;
    if (!v || threshold < 0.0f) return JNI_FALSE;
    
    jsize len = (*env)->GetArrayLength(env, v);
    if (len == 0) return JNI_FALSE;
    
    jfloat *v_ptr = (*env)->GetFloatArrayElements(env, v, NULL);
    if (!v_ptr) return JNI_FALSE;
    
    // Simple check: compare v with its reverse
    jboolean invariant = JNI_TRUE;
    
    for (int i = 0; i < len / 2; i++) {
        float diff = fabsf(v_ptr[i] - v_ptr[len - 1 - i]);
        if (diff > threshold) {
            invariant = JNI_FALSE;
            break;
        }
    }
    
    (*env)->ReleaseFloatArrayElements(env, v, v_ptr, JNI_ABORT);
    
    return invariant;
}

/* VA context operations */

JNIEXPORT jlong JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_initVA(JNIEnv *env, jclass clazz,
                                               jint spaceDim, jint featureType) {
    (void)env;
    (void)clazz;
    (void)featureType;
    // Stub: return a dummy context handle
    // In production, this would allocate and initialize a VA context
    if (spaceDim <= 0) return 0;
    return (jlong)1; // Dummy handle
}

JNIEXPORT void JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_releaseVA(JNIEnv *env, jclass clazz, jlong ctx) {
    (void)env;
    (void)clazz;
    (void)ctx;
    // Stub: release VA context
    // In production, this would free the allocated context
}

/* ANOVA operations */

JNIEXPORT jobject JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_fitLeastSquares(JNIEnv *env, jclass clazz,
                                                        jfloatArray x, jfloatArray y) {
    (void)clazz;
    if (!x || !y) return NULL;
    
    jsize len_x = (*env)->GetArrayLength(env, x);
    jsize len_y = (*env)->GetArrayLength(env, y);
    
    if (len_x != len_y || len_x < 3) return NULL;
    
    jfloat *x_ptr = (*env)->GetFloatArrayElements(env, x, NULL);
    jfloat *y_ptr = (*env)->GetFloatArrayElements(env, y, NULL);
    
    if (!x_ptr || !y_ptr) {
        if (x_ptr) (*env)->ReleaseFloatArrayElements(env, x, x_ptr, JNI_ABORT);
        if (y_ptr) (*env)->ReleaseFloatArrayElements(env, y, y_ptr, JNI_ABORT);
        return NULL;
    }
    
    // Simple linear regression: y = a + b*x
    int n = len_x;
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_x2 = 0.0f;
    
    for (int i = 0; i < n; i++) {
        sum_x += x_ptr[i];
        sum_y += y_ptr[i];
        sum_xy += x_ptr[i] * y_ptr[i];
        sum_x2 += x_ptr[i] * x_ptr[i];
    }
    
    float mean_x = sum_x / n;
    float mean_y = sum_y / n;
    
    float slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    float intercept = mean_y - slope * mean_x;
    
    // Calculate SS values
    float ss_total = 0.0f, ss_model = 0.0f, ss_error = 0.0f;
    
    for (int i = 0; i < n; i++) {
        float y_pred = intercept + slope * x_ptr[i];
        float diff_total = y_ptr[i] - mean_y;
        float diff_model = y_pred - mean_y;
        float diff_error = y_ptr[i] - y_pred;
        
        ss_total += diff_total * diff_total;
        ss_model += diff_model * diff_model;
        ss_error += diff_error * diff_error;
    }
    
    (*env)->ReleaseFloatArrayElements(env, x, x_ptr, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, y, y_ptr, JNI_ABORT);
    
    // Create AnovaResult object
    jclass anova_class = (*env)->FindClass(env, "com/termux/rafaelia/AnovaResult");
    if (!anova_class) return NULL;
    
    jmethodID constructor = (*env)->GetMethodID(env, anova_class, "<init>", "([FFFF)V");
    if (!constructor) return NULL;
    
    jfloatArray coefs = (*env)->NewFloatArray(env, 2);
    if (!coefs) return NULL;
    
    float coef_arr[2] = {intercept, slope};
    (*env)->SetFloatArrayRegion(env, coefs, 0, 2, coef_arr);
    
    jobject result = (*env)->NewObject(env, anova_class, constructor,
                                       coefs, ss_total, ss_model, ss_error);
    
    return result;
}

JNIEXPORT jfloatArray JNICALL
Java_com_termux_rafaelia_RafaeliaUtils_computeSSDecomposition(JNIEnv *env, jclass clazz,
                                                               jfloatArray y, jfloatArray y_pred) {
    (void)clazz;
    if (!y || !y_pred) return NULL;
    
    jsize len_y = (*env)->GetArrayLength(env, y);
    jsize len_pred = (*env)->GetArrayLength(env, y_pred);
    
    if (len_y != len_pred || len_y == 0) return NULL;
    
    jfloat *y_ptr = (*env)->GetFloatArrayElements(env, y, NULL);
    jfloat *pred_ptr = (*env)->GetFloatArrayElements(env, y_pred, NULL);
    
    if (!y_ptr || !pred_ptr) {
        if (y_ptr) (*env)->ReleaseFloatArrayElements(env, y, y_ptr, JNI_ABORT);
        if (pred_ptr) (*env)->ReleaseFloatArrayElements(env, y_pred, pred_ptr, JNI_ABORT);
        return NULL;
    }
    
    int n = len_y;
    float sum_y = 0.0f;
    
    for (int i = 0; i < n; i++) {
        sum_y += y_ptr[i];
    }
    
    float mean_y = sum_y / n;
    float ss_total = 0.0f, ss_model = 0.0f, ss_error = 0.0f;
    
    for (int i = 0; i < n; i++) {
        float diff_total = y_ptr[i] - mean_y;
        float diff_model = pred_ptr[i] - mean_y;
        float diff_error = y_ptr[i] - pred_ptr[i];
        
        ss_total += diff_total * diff_total;
        ss_model += diff_model * diff_model;
        ss_error += diff_error * diff_error;
    }
    
    (*env)->ReleaseFloatArrayElements(env, y, y_ptr, JNI_ABORT);
    (*env)->ReleaseFloatArrayElements(env, y_pred, pred_ptr, JNI_ABORT);
    
    jfloatArray result = (*env)->NewFloatArray(env, 3);
    if (!result) return NULL;
    
    float ss_arr[3] = {ss_total, ss_model, ss_error};
    (*env)->SetFloatArrayRegion(env, result, 0, 3, ss_arr);
    
    return result;
}
