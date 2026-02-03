package com.termux.lowlevel;

import android.util.Log;

/**
 * Example internal program using bare-metal operations
 * Demonstrates deterministic mathematical operations with matrices
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */
public class InternalPrograms {
    
    private static final String TAG = "TermuxInternal";
    
    /**
     * Image processing using matrix operations
     * Applies deterministic flip transformations
     */
    public static class ImageProcessor {
        
        /**
         * Apply horizontal flip to image represented as matrix
         * @param imageData Image pixels as flat array
         * @param width Image width
         * @param height Image height
         */
        public static void flipHorizontal(float[] imageData, int width, int height) {
            flipHorizontal(imageData, width, height, imageData);
        }

        /**
         * Apply horizontal flip to image represented as matrix into an output buffer.
         * @param imageData Image pixels as flat array
         * @param width Image width
         * @param height Image height
         * @param output Output buffer (may be same as input)
         */
        public static void flipHorizontal(float[] imageData, int width, int height, float[] output) {
            int size = width * height;
            if (imageData.length < size || output.length < size) {
                throw new IllegalArgumentException("Image buffer too small");
            }

            if (!BareMetal.isLoaded()) {
                if (output == imageData) {
                    for (int row = 0; row < height; row++) {
                        int base = row * width;
                        int left = base;
                        int right = base + width - 1;
                        while (left < right) {
                            float tmp = imageData[left];
                            imageData[left] = imageData[right];
                            imageData[right] = tmp;
                            left++;
                            right--;
                        }
                    }
                } else {
                    for (int row = 0; row < height; row++) {
                        int base = row * width;
                        for (int col = 0; col < width; col++) {
                            output[base + col] = imageData[base + (width - 1 - col)];
                        }
                    }
                }
                return;
            }

            BareMetal.Matrix m = new BareMetal.Matrix(height, width);
            try {
                m.setData(imageData);
                m.flipHorizontal();
                m.getDataInto(output);
            } finally {
                m.close();
            }
        }
        
        /**
         * Apply vertical flip
         */
        public static void flipVertical(float[] imageData, int width, int height) {
            flipVertical(imageData, width, height, imageData);
        }

        /**
         * Apply vertical flip into an output buffer.
         */
        public static void flipVertical(float[] imageData, int width, int height, float[] output) {
            int size = width * height;
            if (imageData.length < size || output.length < size) {
                throw new IllegalArgumentException("Image buffer too small");
            }

            if (!BareMetal.isLoaded()) {
                if (output == imageData) {
                    for (int row = 0; row < height / 2; row++) {
                        int topBase = row * width;
                        int bottomBase = (height - 1 - row) * width;
                        for (int col = 0; col < width; col++) {
                            float tmp = imageData[topBase + col];
                            imageData[topBase + col] = imageData[bottomBase + col];
                            imageData[bottomBase + col] = tmp;
                        }
                    }
                } else {
                    for (int row = 0; row < height; row++) {
                        int srcBase = (height - 1 - row) * width;
                        int dstBase = row * width;
                        System.arraycopy(imageData, srcBase, output, dstBase, width);
                    }
                }
                return;
            }

            BareMetal.Matrix m = new BareMetal.Matrix(height, width);
            try {
                m.setData(imageData);
                m.flipVertical();
                m.getDataInto(output);
            } finally {
                m.close();
            }
        }
        
        /**
         * Apply diagonal flip (transpose)
         */
        public static void transpose(float[] imageData, int width, int height) {
            transpose(imageData, width, height, imageData);
        }

        /**
         * Apply diagonal flip (transpose) into an output buffer.
         */
        public static void transpose(float[] imageData, int width, int height, float[] output) {
            int size = width * height;
            if (imageData.length < size || output.length < size) {
                throw new IllegalArgumentException("Image buffer too small");
            }

            if (!BareMetal.isLoaded()) {
                if (output == imageData) {
                    if (width != height) {
                        throw new IllegalArgumentException("In-place transpose requires a square image");
                    }
                    for (int row = 0; row < height; row++) {
                        for (int col = row + 1; col < width; col++) {
                            int idx1 = row * width + col;
                            int idx2 = col * width + row;
                            float tmp = imageData[idx1];
                            imageData[idx1] = imageData[idx2];
                            imageData[idx2] = tmp;
                        }
                    }
                } else {
                    for (int row = 0; row < height; row++) {
                        for (int col = 0; col < width; col++) {
                            output[col * height + row] = imageData[row * width + col];
                        }
                    }
                }
                return;
            }

            BareMetal.Matrix m = new BareMetal.Matrix(height, width);
            try {
                m.setData(imageData);
                m.flipDiagonal();
                m.getDataInto(output);
            } finally {
                m.close();
            }
        }
    }
    
    /**
     * Vector analysis using deterministic mathematics
     */
    public static class VectorAnalyzer {
        
        /**
         * Analyze similarity between two feature vectors
         * Returns cosine similarity using optimized SIMD operations
         */
        public static float analyzeSimilarity(float[] features1, float[] features2) {
            if (!BareMetal.isLoaded()) {
                return computeSimilarityFallback(features1, features2);
            }
            
            return BareMetal.cosineSimilarity(features1, features2);
        }
        
        /**
         * Fallback implementation if native not available
         */
        private static float computeSimilarityFallback(float[] a, float[] b) {
            float dot = 0.0f;
            float normA = 0.0f;
            float normB = 0.0f;
            
            for (int i = 0; i < a.length; i++) {
                dot += a[i] * b[i];
                normA += a[i] * a[i];
                normB += b[i] * b[i];
            }
            
            normA = (float) Math.sqrt(normA);
            normB = (float) Math.sqrt(normB);
            
            if (normA == 0 || normB == 0) return 0.0f;
            return dot / (normA * normB);
        }
        
        /**
         * Compute distance between vectors using optimized operations
         */
        public static float euclideanDistance(float[] a, float[] b) {
            if (!BareMetal.isLoaded()) {
                return computeDistanceFallback(a, b);
            }
            
            float[] diff = new float[a.length];
            BareMetal.vectorSub(a, b, diff);
            return BareMetal.vectorNorm(diff);
        }
        
        private static float computeDistanceFallback(float[] a, float[] b) {
            float sum = 0.0f;
            for (int i = 0; i < a.length; i++) {
                float d = a[i] - b[i];
                sum += d * d;
            }
            return (float) Math.sqrt(sum);
        }
    }
    
    /**
     * Fast mathematical computations for terminal operations
     */
    public static class FastMath {
        
        /**
         * Compute square root using bare-metal algorithm
         * Faster than Math.sqrt() for large datasets
         */
        public static float sqrt(float x) {
            if (!BareMetal.isLoaded()) {
                return (float) Math.sqrt(x);
            }
            return BareMetal.fastSqrt(x);
        }
        
        /**
         * Compute exponential
         */
        public static float exp(float x) {
            if (!BareMetal.isLoaded()) {
                return (float) Math.exp(x);
            }
            return BareMetal.fastExp(x);
        }
        
        /**
         * Compute natural logarithm
         */
        public static float log(float x) {
            if (!BareMetal.isLoaded()) {
                return (float) Math.log(x);
            }
            return BareMetal.fastLog(x);
        }
    }
    
    /**
     * Memory operations for efficient data handling
     */
    public static class MemoryOps {
        
        /**
         * Fast memory copy using SIMD
         */
        public static void copy(byte[] dst, byte[] src) {
            if (!BareMetal.isLoaded() || dst.length != src.length) {
                System.arraycopy(src, 0, dst, 0, src.length);
                return;
            }
            
            BareMetal.memCopy(dst, src);
        }
        
        /**
         * Fast memory fill using SIMD
         */
        public static void fill(byte[] array, byte value) {
            if (!BareMetal.isLoaded()) {
                java.util.Arrays.fill(array, value);
                return;
            }
            
            BareMetal.memSet(array, value);
        }
    }
    
    /**
     * Matrix solver using deterministic flip operations
     */
    public static class MatrixSolver {
        
        /**
         * Solve matrix equation using flip operations
         * Implements deterministic mathematical approach
         */
        public static BareMetal.Matrix solve(BareMetal.Matrix a, BareMetal.Matrix b) {
            if (!BareMetal.isLoaded()) {
                throw new UnsupportedOperationException("BareMetal not loaded");
            }
            
            // Check determinant
            float det = a.determinant();
            if (Math.abs(det) < 1e-6f) {
                throw new IllegalArgumentException("Matrix is singular (determinant ≈ 0)");
            }
            
            // Apply flip operations for solving
            // This is a simplified demonstration
            a.flipDiagonal();  // Transpose
            BareMetal.Matrix result = new BareMetal.Matrix(a.getRows(), b.getCols());
            a.multiplyInto(b, result);
            return result;
        }

        /**
         * Solve matrix equation using flip operations into an existing output matrix.
         * Implements deterministic mathematical approach without allocations.
         */
        public static void solveInto(BareMetal.Matrix a, BareMetal.Matrix b, BareMetal.Matrix output) {
            if (!BareMetal.isLoaded()) {
                throw new UnsupportedOperationException("BareMetal not loaded");
            }

            // Check determinant
            float det = a.determinant();
            if (Math.abs(det) < 1e-6f) {
                throw new IllegalArgumentException("Matrix is singular (determinant ≈ 0)");
            }

            // Apply flip operations for solving
            // This is a simplified demonstration
            a.flipDiagonal();  // Transpose
            a.multiplyInto(b, output);
        }
    }
    
    /**
     * System information and diagnostics
     */
    public static String getSystemInfo() {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Termux Internal Programs ===\n\n");
        
        if (BareMetal.isLoaded()) {
            sb.append(BareMetal.getInfo());
            sb.append("\nStatus: Native library loaded successfully\n");
            sb.append("Optimizations: SIMD enabled for architecture\n");
        } else {
            sb.append("Status: Native library not loaded\n");
            sb.append("Fallback: Using Java implementations\n");
        }
        
        return sb.toString();
    }
    
    /**
     * Run benchmark to demonstrate performance
     */
    public static String runBenchmark() {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Performance Benchmark ===\n\n");
        
        if (!BareMetal.isLoaded()) {
            sb.append("Native library not loaded, skipping benchmark\n");
            return sb.toString();
        }
        
        // Vector operations benchmark
        int vectorSize = 1000;
        float[] v1 = new float[vectorSize];
        float[] v2 = new float[vectorSize];
        for (int i = 0; i < vectorSize; i++) {
            v1[i] = (float) Math.random();
            v2[i] = (float) Math.random();
        }
        
        long start = System.nanoTime();
        for (int i = 0; i < 10000; i++) {
            BareMetal.vectorDot(v1, v2);
        }
        long end = System.nanoTime();
        
        sb.append("Vector dot product (10k iterations, 1000 dims):\n");
        sb.append(String.format("  Time: %.3f ms\n", (end - start) / 1e6));
        
        // Fast math benchmark
        start = System.nanoTime();
        for (int i = 0; i < 100000; i++) {
            BareMetal.fastSqrt(i);
        }
        end = System.nanoTime();
        
        sb.append("Fast sqrt (100k iterations):\n");
        sb.append(String.format("  Time: %.3f ms\n", (end - start) / 1e6));
        
        return sb.toString();
    }
}
