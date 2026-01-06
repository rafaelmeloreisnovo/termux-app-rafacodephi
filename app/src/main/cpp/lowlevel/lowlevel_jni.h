#ifndef TERMUX_LOWLEVEL_JNI_H
#define TERMUX_LOWLEVEL_JNI_H

#include <jni.h>

/**
 * JNI bridge for low-level bare-metal operations
 * Minimal Java dependencies, maximum native performance
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize VA (Vectra Analysis) context
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    initVA
 * Signature: (II)J
 */
JNIEXPORT jlong JNICALL
Java_com_termux_lowlevel_VectraMath_initVA(JNIEnv *env, jclass clazz, 
                                            jint space_dim, jint feature_type);

/**
 * Compute cosine similarity between two vectors
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    computeCosineSimilarity
 * Signature: ([F[F)F
 */
JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_VectraMath_computeCosineSimilarity(JNIEnv *env, jclass clazz,
                                                             jfloatArray v1, jfloatArray v2);

/**
 * Compute Euclidean distance between two vectors
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    computeEuclideanDistance
 * Signature: ([F[F)F
 */
JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_VectraMath_computeEuclideanDistance(JNIEnv *env, jclass clazz,
                                                              jfloatArray v1, jfloatArray v2);

/**
 * Test reversal invariance
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    testReversalInvariance
 * Signature: ([FF)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_termux_lowlevel_VectraMath_testReversalInvariance(JNIEnv *env, jclass clazz,
                                                            jfloatArray v, jfloat threshold);

/**
 * Fit ANOVA least squares model
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    fitLeastSquares
 * Signature: ([F[F)Lcom/termux/lowlevel/AnovaResult;
 */
JNIEXPORT jobject JNICALL
Java_com_termux_lowlevel_VectraMath_fitLeastSquares(JNIEnv *env, jclass clazz,
                                                     jfloatArray x, jfloatArray y);

/**
 * Compute ANOVA SS decomposition
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    computeSSDecomposition
 * Signature: ([F[F)[F
 */
JNIEXPORT jfloatArray JNICALL
Java_com_termux_lowlevel_VectraMath_computeSSDecomposition(JNIEnv *env, jclass clazz,
                                                            jfloatArray y, jfloatArray y_pred);

/**
 * Optimized memory copy
 * Class:     com_termux_lowlevel_LowLevelUtils
 * Method:    memcpyOptimized
 * Signature: ([B[BI)V
 */
JNIEXPORT void JNICALL
Java_com_termux_lowlevel_LowLevelUtils_memcpyOptimized(JNIEnv *env, jclass clazz,
                                                        jbyteArray dest, jbyteArray src, jint n);

/**
 * Optimized memory set
 * Class:     com_termux_lowlevel_LowLevelUtils
 * Method:    memsetOptimized
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL
Java_com_termux_lowlevel_LowLevelUtils_memsetOptimized(JNIEnv *env, jclass clazz,
                                                        jbyteArray array, jint value, jint n);

/**
 * Fast square root
 * Class:     com_termux_lowlevel_LowLevelUtils
 * Method:    sqrtFast
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL
Java_com_termux_lowlevel_LowLevelUtils_sqrtFast(JNIEnv *env, jclass clazz, jfloat x);

/**
 * Release VA context
 * Class:     com_termux_lowlevel_VectraMath
 * Method:    releaseVA
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_termux_lowlevel_VectraMath_releaseVA(JNIEnv *env, jclass clazz, jlong ctx);

#ifdef __cplusplus
}
#endif

#endif // TERMUX_LOWLEVEL_JNI_H
