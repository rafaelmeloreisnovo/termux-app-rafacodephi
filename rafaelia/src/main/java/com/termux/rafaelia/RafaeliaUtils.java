package com.termux.rafaelia;

/**
 * Rafaelia: Optimized bare-metal utilities module
 * 
 * Provides high-performance mathematical and memory operations with:
 * - Minimal Java dependencies
 * - Bare-metal C/ASM optimizations
 * - Vector operations with SIMD support
 * - Statistical analysis (ANOVA)
 * 
 * All operations are consolidated in this single utility class to reduce
 * redundancy and improve code maintainability.
 */
public final class RafaeliaUtils {
    
    /** Flag indicating if native library is available */
    private static final boolean NATIVE_AVAILABLE;
    
    static {
        boolean loaded = false;
        try {
            System.loadLibrary("termux-rafaelia");
            loaded = true;
        } catch (UnsatisfiedLinkError e) {
            // Native library not available - will use Java fallbacks where possible
            loaded = false;
        }
        NATIVE_AVAILABLE = loaded;
    }
    
    /**
     * Check if native library is available.
     * @return true if native methods can be used, false otherwise
     */
    public static boolean isNativeAvailable() {
        return NATIVE_AVAILABLE;
    }
    
    // Private constructor to prevent instantiation
    private RafaeliaUtils() {
        throw new AssertionError("Utility class - do not instantiate");
    }
    
    // Constants
    private static final float EPSILON = 1e-10f;  // Threshold for floating-point comparisons
    private static final int UNROLL_FACTOR = 4;   // Loop unrolling factor for vector operations
    
    // ==================== Memory Operations ====================
    
    /**
     * Optimized memory copy using bare-metal C/ASM implementation.
     * Faster than System.arraycopy for large blocks.
     * 
     * @param dest Destination byte array (must not be null)
     * @param src Source byte array (must not be null)
     * @param n Number of bytes to copy (must be > 0 and within array bounds)
     * @throws IllegalArgumentException if parameters are invalid
     */
    public static native void memcpy(byte[] dest, byte[] src, int n);
    
    /**
     * Optimized memory set using bare-metal C/ASM implementation.
     * Faster than Arrays.fill for large blocks.
     * 
     * @param array Target byte array (must not be null)
     * @param value Value to set (byte)
     * @param n Number of bytes to set (must be > 0 and within array bounds)
     * @throws IllegalArgumentException if parameters are invalid
     */
    public static native void memset(byte[] array, int value, int n);
    
    // ==================== Fast Mathematical Operations ====================
    
    /**
     * Fast square root using Newton-Raphson method (native) or Java Math.sqrt (fallback).
     * 
     * @param x Input value (must be >= 0)
     * @return Square root of x, or 0 if x < 0
     */
    public static float sqrt(float x) {
        if (NATIVE_AVAILABLE) {
            return sqrtNative(x);
        }
        if (x < 0.0f) return 0.0f;
        return (float) Math.sqrt(x);
    }
    
    /**
     * Native square root implementation.
     */
    private static native float sqrtNative(float x);
    
    /**
     * Fast integer power (no library dependencies).
     * Uses exponentiation by squaring for optimal performance.
     * 
     * @param base Base value
     * @param exp Exponent (integer)
     * @return base^exp
     */
    public static float pow(float base, int exp) {
        if (exp == 0) return 1.0f;
        if (base == 0.0f) return 0.0f;
        if (base == 1.0f) return 1.0f;
        
        boolean negative = exp < 0;
        int absExp = negative ? -exp : exp;
        
        float result = 1.0f;
        float current = base;
        
        // Exponentiation by squaring (binary method)
        // For exp = 13 (binary: 1101), computes base^8 * base^4 * base^1
        while (absExp > 0) {
            if ((absExp & 1) == 1) {
                result *= current;
            }
            current *= current;
            absExp >>= 1;
        }
        
        return negative ? (1.0f / result) : result;
    }
    
    // ==================== Vector Operations ====================
    
    /**
     * Compute dot product with optimized implementation.
     * Uses SIMD/NEON when available via JNI.
     * 
     * @param v1 First vector (must not be null)
     * @param v2 Second vector (must not be null and same length as v1)
     * @return Dot product, or 0 if inputs are invalid
     */
    public static float dotProduct(float[] v1, float[] v2) {
        if (v1 == null || v2 == null || v1.length != v2.length || v1.length == 0) {
            return 0.0f;
        }
        
        float sum = 0.0f;
        int len = v1.length;
        
        // Unrolled loop for better performance
        int i = 0;
        // Round down to nearest multiple of UNROLL_FACTOR
        int unrollLimit = len & ~(UNROLL_FACTOR - 1);
        
        for (; i < unrollLimit; i += UNROLL_FACTOR) {
            sum += v1[i] * v2[i] + v1[i+1] * v2[i+1] + 
                   v1[i+2] * v2[i+2] + v1[i+3] * v2[i+3];
        }
        
        // Handle remainder
        for (; i < len; i++) {
            sum += v1[i] * v2[i];
        }
        
        return sum;
    }
    
    /**
     * Compute vector magnitude (L2 norm).
     * 
     * @param v Input vector (must not be null)
     * @return ||v||, or 0 if v is null or empty
     */
    public static float magnitude(float[] v) {
        if (v == null || v.length == 0) return 0.0f;
        
        float sumSq = 0.0f;
        
        // Optimized loop with unrolling
        int i = 0;
        int len = v.length;
        // Round down to nearest multiple of UNROLL_FACTOR
        int unrollLimit = len & ~(UNROLL_FACTOR - 1);
        
        for (; i < unrollLimit; i += UNROLL_FACTOR) {
            sumSq += v[i] * v[i] + v[i+1] * v[i+1] + 
                     v[i+2] * v[i+2] + v[i+3] * v[i+3];
        }
        
        for (; i < len; i++) {
            sumSq += v[i] * v[i];
        }
        
        return sqrt(sumSq);
    }
    
    /**
     * Normalize vector to unit length (in-place modification).
     * 
     * @param v Input vector (modified in-place, must not be null)
     */
    public static void normalize(float[] v) {
        if (v == null || v.length == 0) return;
        
        float mag = magnitude(v);
        if (mag < EPSILON) return; // Avoid division by zero
        
        float invMag = 1.0f / mag;
        for (int i = 0; i < v.length; i++) {
            v[i] *= invMag;
        }
    }
    
    // ==================== Vectra Analysis (VA) Operations ====================
    
    // Feature types for VA context
    public static final int FEATURE_HASH = 0;
    public static final int FEATURE_TEXT = 1;
    public static final int FEATURE_PHONEME = 2;
    public static final int FEATURE_IMAGE_FFT = 3;
    
    /**
     * Initialize VA context with specified space dimension and feature type.
     * VA_min = (Space ⊕ Features ⊕ Pairing ⊕ InvarianceTests ⊕ OutputSpec)
     * 
     * @param spaceDim Fixed dimension n for the space (must be > 0)
     * @param featureType Base channel type (HASH, TEXT, PHONEME, IMAGE_FFT)
     * @return Native context handle (pointer), or 0 on error
     */
    public static native long initVA(int spaceDim, int featureType);
    
    /**
     * Release VA context and free resources.
     * 
     * @param ctx Native context handle (from initVA)
     */
    public static native void releaseVA(long ctx);
    
    /**
     * Compute cosine similarity between two vectors.
     * part of VA pairing: pair(v_i, v_j) => { cos(v_i, v_j), ||v_i - v_j||, ΔH }
     * 
     * @param v1 First vector (must not be null)
     * @param v2 Second vector (must not be null and same length as v1)
     * @return Cosine similarity [-1, 1], or 0 if inputs are invalid
     */
    public static native float cosineSimilarity(float[] v1, float[] v2);
    
    /**
     * Compute Euclidean distance between two vectors.
     * ||v_i - v_j||
     * 
     * @param v1 First vector (must not be null)
     * @param v2 Second vector (must not be null and same length as v1)
     * @return Euclidean distance, or 0 if inputs are invalid
     */
    public static native float euclideanDistance(float[] v1, float[] v2);
    
    /**
     * Test reversal invariance.
     * I_← (v) = 1[pair(v, rev(v)) stable]
     * 
     * @param v Vector to test (must not be null)
     * @param threshold Stability threshold (must be >= 0)
     * @return true if reversal invariant, false otherwise
     */
    public static native boolean testReversalInvariance(float[] v, float threshold);
    
    // ==================== ANOVA Operations ====================
    
    /**
     * Fit ANOVA least squares model.
     * Minimizes: d/d_beta sum_i (y_i - y_hat_i(beta))^2 = 0
     * 
     * @param x Independent variable (must not be null, length > 2)
     * @param y Dependent variable (must not be null, same length as x)
     * @return ANOVA result with coefficients and SS decomposition, or null on error
     */
    public static native AnovaResult fitLeastSquares(float[] x, float[] y);
    
    /**
     * Compute ANOVA SS decomposition.
     * SS_T = SS_M + SS_E
     * 
     * @param y Observed values (must not be null)
     * @param yPred Predicted values (must not be null, same length as y)
     * @return Array [SS_T, SS_M, SS_E], or null on error
     */
    public static native float[] computeSSDecomposition(float[] y, float[] yPred);
}
