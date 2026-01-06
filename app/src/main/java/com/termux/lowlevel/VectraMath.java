package com.termux.lowlevel;

/**
 * Low-level bare-metal mathematical operations for Vectra Analysis (VA)
 * Direct JNI bridge to optimized C/ASM implementation
 * 
 * VA_min = (Space ⊕ Features ⊕ Pairing ⊕ InvarianceTests ⊕ OutputSpec)
 * 
 * Provides:
 * - Minimal Java dependencies
 * - Bare-metal optimized operations
 * - ANOVA bridge with derivatives/antiderivatives support
 */
public class VectraMath {
    
    static {
        System.loadLibrary("termux-lowlevel");
    }
    
    // Feature types for VA context
    public static final int FEATURE_HASH = 0;
    public static final int FEATURE_TEXT = 1;
    public static final int FEATURE_PHONEME = 2;
    public static final int FEATURE_IMAGE_FFT = 3;
    
    /**
     * Initialize VA context with specified space dimension and feature type
     * 
     * @param spaceDim Fixed dimension n for the space
     * @param featureType Base channel type (HASH, TEXT, PHONEME, IMAGE_FFT)
     * @return Native context handle (pointer)
     */
    public static native long initVA(int spaceDim, int featureType);
    
    /**
     * Release VA context and free resources
     * 
     * @param ctx Native context handle
     */
    public static native void releaseVA(long ctx);
    
    /**
     * Compute cosine similarity between two vectors
     * pair(v_i, v_j) => { cos(v_i, v_j), ||v_i - v_j||, ΔH }
     * 
     * @param v1 First vector
     * @param v2 Second vector
     * @return Cosine similarity [-1, 1]
     */
    public static native float computeCosineSimilarity(float[] v1, float[] v2);
    
    /**
     * Compute Euclidean distance between two vectors
     * ||v_i - v_j||
     * 
     * @param v1 First vector
     * @param v2 Second vector
     * @return Euclidean distance
     */
    public static native float computeEuclideanDistance(float[] v1, float[] v2);
    
    /**
     * Test reversal invariance
     * I_← (v) = 1[pair(v, rev(v)) stable]
     * 
     * @param v Vector to test
     * @param threshold Stability threshold
     * @return true if reversal invariant
     */
    public static native boolean testReversalInvariance(float[] v, float threshold);
    
    /**
     * Fit ANOVA least squares model
     * Minimizes: d/d_beta sum_i (y_i - y_hat_i(beta))^2 = 0
     * 
     * @param x Independent variable
     * @param y Dependent variable
     * @return ANOVA result with coefficients and SS decomposition
     */
    public static native AnovaResult fitLeastSquares(float[] x, float[] y);
    
    /**
     * Compute ANOVA SS decomposition
     * SS_T = SS_M + SS_E
     * 
     * @param y Observed values
     * @param yPred Predicted values
     * @return Array [SS_T, SS_M, SS_E]
     */
    public static native float[] computeSSDecomposition(float[] y, float[] yPred);
}
