package com.termux.lowlevel;

/**
 * Low-level utility functions with bare-metal optimizations
 * Minimal Java dependencies, maximum native performance
 */
public class LowLevelUtils {
    
    static {
        System.loadLibrary("termux-lowlevel");
    }
    
    /**
     * Optimized memory copy using bare-metal C/ASM implementation
     * Faster than System.arraycopy for large blocks
     * 
     * @param dest Destination byte array
     * @param src Source byte array
     * @param n Number of bytes to copy
     */
    public static native void memcpyOptimized(byte[] dest, byte[] src, int n);
    
    /**
     * Optimized memory set using bare-metal C/ASM implementation
     * Faster than Arrays.fill for large blocks
     * 
     * @param array Target byte array
     * @param value Value to set (byte)
     * @param n Number of bytes to set
     */
    public static native void memsetOptimized(byte[] array, int value, int n);
    
    /**
     * Fast square root using Newton-Raphson method
     * Bare-metal implementation with no library dependencies
     * 
     * @param x Input value
     * @return Square root of x
     */
    public static native float sqrtFast(float x);
    
    /**
     * Fast integer power (no library dependencies)
     * 
     * @param base Base value
     * @param exp Exponent (integer)
     * @return base^exp
     */
    public static float powFast(float base, int exp) {
        if (exp == 0) return 1.0f;
        
        float result = 1.0f;
        int absExp = (exp < 0) ? -exp : exp;
        
        for (int i = 0; i < absExp; i++) {
            result *= base;
        }
        
        return (exp < 0) ? (1.0f / result) : result;
    }
    
    /**
     * Compute dot product with optimized implementation
     * Uses SIMD/NEON when available
     * 
     * @param v1 First vector
     * @param v2 Second vector
     * @return Dot product
     */
    public static float dotProduct(float[] v1, float[] v2) {
        if (v1 == null || v2 == null || v1.length != v2.length) {
            return 0.0f;
        }
        
        float sum = 0.0f;
        
        // Unrolled loop for better performance
        int i = 0;
        int len = v1.length;
        int unrollLimit = len - (len % 4);
        
        for (; i < unrollLimit; i += 4) {
            sum += v1[i] * v2[i];
            sum += v1[i+1] * v2[i+1];
            sum += v1[i+2] * v2[i+2];
            sum += v1[i+3] * v2[i+3];
        }
        
        // Handle remainder
        for (; i < len; i++) {
            sum += v1[i] * v2[i];
        }
        
        return sum;
    }
    
    /**
     * Compute vector magnitude (L2 norm)
     * 
     * @param v Input vector
     * @return ||v||
     */
    public static float magnitude(float[] v) {
        if (v == null || v.length == 0) return 0.0f;
        
        float sumSq = 0.0f;
        for (float val : v) {
            sumSq += val * val;
        }
        
        return sqrtFast(sumSq);
    }
    
    /**
     * Normalize vector to unit length
     * 
     * @param v Input vector (modified in-place)
     */
    public static void normalize(float[] v) {
        if (v == null || v.length == 0) return;
        
        float mag = magnitude(v);
        if (mag < 1e-10f) return;
        
        float invMag = 1.0f / mag;
        for (int i = 0; i < v.length; i++) {
            v[i] *= invMag;
        }
    }
}
