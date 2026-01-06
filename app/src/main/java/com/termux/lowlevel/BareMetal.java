package com.termux.lowlevel;

/**
 * Bare-metal low-level operations for Termux
 * Direct native C/ASM implementation with no external dependencies
 * Architecture-optimized for ARM, ARM64, x86, x86_64
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */
public class BareMetal {
    
    private static boolean sLoaded = false;
    
    static {
        try {
            System.loadLibrary("termux-baremetal");
            sLoaded = true;
        } catch (UnsatisfiedLinkError e) {
            android.util.Log.e("BareMetal", "Failed to load native library", e);
        }
    }
    
    /**
     * Check if native library is loaded
     */
    public static boolean isLoaded() {
        return sLoaded;
    }
    
    /* ========================================================================
     * Architecture Detection
     * ====================================================================== */
    
    /**
     * Get CPU architecture name
     * @return Architecture string (e.g., "arm64-v8a", "armeabi-v7a", "x86_64", "x86")
     */
    public static native String getArchitecture();
    
    /**
     * Get CPU capabilities bitmask
     * @return Bitmask of capabilities (NEON, AVX, SSE, etc.)
     */
    public static native int getCapabilities();
    
    /**
     * Check if NEON SIMD is available
     */
    public static boolean hasNeon() {
        return (getCapabilities() & 0x01) != 0;
    }
    
    /**
     * Check if AVX is available
     */
    public static boolean hasAvx() {
        return (getCapabilities() & 0x02) != 0;
    }
    
    /* ========================================================================
     * Vector Operations - Optimized for SIMD
     * ====================================================================== */
    
    /**
     * Compute dot product of two vectors
     * Optimized with NEON/AVX SIMD
     * 
     * @param a First vector
     * @param b Second vector
     * @return Dot product a·b
     */
    public static native float vectorDot(float[] a, float[] b);
    
    /**
     * Compute Euclidean norm (magnitude) of vector
     * 
     * @param a Vector
     * @return ||a|| = sqrt(Σ aᵢ²)
     */
    public static native float vectorNorm(float[] a);
    
    /**
     * Add two vectors element-wise
     * 
     * @param a First vector
     * @param b Second vector
     * @param result Output vector (must be pre-allocated)
     */
    public static native void vectorAdd(float[] a, float[] b, float[] result);
    
    /**
     * Subtract two vectors element-wise
     * 
     * @param a First vector
     * @param b Second vector
     * @param result Output vector (must be pre-allocated)
     */
    public static void vectorSub(float[] a, float[] b, float[] result) {
        if (a.length != b.length || a.length != result.length) {
            throw new IllegalArgumentException("Vector dimensions must match");
        }
        for (int i = 0; i < a.length; i++) {
            result[i] = a[i] - b[i];
        }
    }
    
    /**
     * Compute cosine similarity between two vectors
     * cos(θ) = (a·b) / (||a|| * ||b||)
     * 
     * @param a First vector
     * @param b Second vector
     * @return Cosine similarity [-1, 1]
     */
    public static float cosineSimilarity(float[] a, float[] b) {
        float dot = vectorDot(a, b);
        float normA = vectorNorm(a);
        float normB = vectorNorm(b);
        if (normA == 0 || normB == 0) return 0.0f;
        return dot / (normA * normB);
    }
    
    /* ========================================================================
     * Matrix Operations - Deterministic Mathematics
     * ====================================================================== */
    
    /**
     * Create matrix with given dimensions
     * @param rows Number of rows
     * @param cols Number of columns
     * @return Matrix handle (native pointer)
     */
    public static native long matrixCreate(int rows, int cols);
    
    /**
     * Free matrix memory
     * @param handle Matrix handle
     */
    public static native void matrixFree(long handle);
    
    /**
     * Multiply two matrices: C = A × B
     * @param handleA First matrix
     * @param handleB Second matrix
     * @param handleResult Result matrix
     */
    public static native void matrixMultiply(long handleA, long handleB, long handleResult);
    
    /**
     * Flip matrix horizontally (mirror left-right)
     * Used for solving with deterministic mathematics
     * 
     * @param handle Matrix handle
     */
    public static native void matrixFlipHorizontal(long handle);
    
    /**
     * Flip matrix vertically (mirror top-bottom)
     * @param handle Matrix handle
     */
    public static native void matrixFlipVertical(long handle);
    
    /**
     * Flip matrix diagonally (transpose)
     * @param handle Matrix handle
     */
    public static native void matrixFlipDiagonal(long handle);
    
    /**
     * Compute matrix determinant
     * @param handle Matrix handle
     * @return Determinant value
     */
    public static native float matrixDeterminant(long handle);
    
    /* ========================================================================
     * Fast Math - No legacy functions, bare-metal implementations
     * ====================================================================== */
    
    /**
     * Fast square root using bare-metal algorithm
     * @param x Input value
     * @return √x
     */
    public static native float fastSqrt(float x);
    
    /**
     * Fast reciprocal square root (1/√x)
     * Uses Quake III algorithm
     * 
     * @param x Input value
     * @return 1/√x
     */
    public static native float fastRsqrt(float x);
    
    /**
     * Fast exponential (eˣ)
     * @param x Input value
     * @return eˣ
     */
    public static native float fastExp(float x);
    
    /**
     * Fast natural logarithm (ln x)
     * @param x Input value
     * @return ln(x)
     */
    public static native float fastLog(float x);
    
    /**
     * Fast power of 2
     * @param x Input value
     * @return x²
     */
    public static float fastPow2(float x) {
        return x * x;
    }
    
    /* ========================================================================
     * Memory Operations - Bare-metal, no libc dependencies
     * ====================================================================== */
    
    /**
     * Fast memory copy using SIMD where available
     * @param dst Destination array
     * @param src Source array
     */
    public static native void memCopy(byte[] dst, byte[] src);
    
    /**
     * Fast memory set using SIMD where available
     * @param array Array to fill
     * @param value Byte value to fill with
     */
    public static native void memSet(byte[] array, int value);
    
    /* ========================================================================
     * Utility Methods
     * ====================================================================== */
    
    /**
     * Get library information
     */
    public static String getInfo() {
        if (!isLoaded()) {
            return "BareMetal library not loaded";
        }
        
        StringBuilder sb = new StringBuilder();
        sb.append("TermuxBareMetal Library\n");
        sb.append("Architecture: ").append(getArchitecture()).append("\n");
        sb.append("Capabilities: 0x").append(Integer.toHexString(getCapabilities())).append("\n");
        sb.append("NEON: ").append(hasNeon()).append("\n");
        sb.append("AVX: ").append(hasAvx()).append("\n");
        sb.append("Implementation: C + Assembly (bare-metal)\n");
        sb.append("Dependencies: None (libc only)\n");
        
        return sb.toString();
    }
    
    /**
     * Matrix helper class for easier usage
     */
    public static class Matrix {
        private long handle;
        private int rows;
        private int cols;
        
        public Matrix(int rows, int cols) {
            this.rows = rows;
            this.cols = cols;
            this.handle = matrixCreate(rows, cols);
            if (this.handle == 0) {
                throw new OutOfMemoryError("Failed to create matrix");
            }
        }
        
        public int getRows() { return rows; }
        public int getCols() { return cols; }
        public long getHandle() { return handle; }
        
        public void flipHorizontal() {
            matrixFlipHorizontal(handle);
        }
        
        public void flipVertical() {
            matrixFlipVertical(handle);
        }
        
        public void flipDiagonal() {
            matrixFlipDiagonal(handle);
        }
        
        public float determinant() {
            return matrixDeterminant(handle);
        }
        
        public Matrix multiply(Matrix other) {
            if (this.cols != other.rows) {
                throw new IllegalArgumentException("Matrix dimensions incompatible for multiplication");
            }
            Matrix result = new Matrix(this.rows, other.cols);
            matrixMultiply(this.handle, other.handle, result.handle);
            return result;
        }
        
        @Override
        protected void finalize() throws Throwable {
            if (handle != 0) {
                matrixFree(handle);
                handle = 0;
            }
            super.finalize();
        }
        
        public void close() {
            if (handle != 0) {
                matrixFree(handle);
                handle = 0;
            }
        }
    }
}
