package com.termux.lowlevel.test;

import com.termux.lowlevel.BareMetal;
import com.termux.lowlevel.InternalPrograms;

/**
 * Example usage and test cases for the bare-metal library
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */
public class BaremetalExample {
    
    public static void main(String[] args) {
        System.out.println("=== Termux Bare-Metal Library Examples ===\n");
        
        // Check if library is loaded
        if (!BareMetal.isLoaded()) {
            System.out.println("ERROR: Native library not loaded");
            return;
        }
        
        // Example 1: Architecture detection
        example1_ArchitectureInfo();
        
        // Example 2: Vector operations
        example2_VectorOperations();
        
        // Example 3: Matrix operations
        example3_MatrixOperations();
        
        // Example 4: Fast math
        example4_FastMath();
        
        // Example 5: Internal programs
        example5_InternalPrograms();
        
        // Run benchmark
        System.out.println(InternalPrograms.runBenchmark());
    }
    
    private static void example1_ArchitectureInfo() {
        System.out.println("=== Example 1: Architecture Detection ===");
        System.out.println(BareMetal.getInfo());
        System.out.println();
    }
    
    private static void example2_VectorOperations() {
        System.out.println("=== Example 2: Vector Operations ===");
        
        float[] v1 = {1.0f, 2.0f, 3.0f, 4.0f};
        float[] v2 = {5.0f, 6.0f, 7.0f, 8.0f};
        
        // Dot product
        float dot = BareMetal.vectorDot(v1, v2);
        System.out.println("Dot product: " + dot);
        
        // Norm
        float norm1 = BareMetal.vectorNorm(v1);
        float norm2 = BareMetal.vectorNorm(v2);
        System.out.println("Norm v1: " + norm1);
        System.out.println("Norm v2: " + norm2);
        
        // Cosine similarity
        float similarity = BareMetal.cosineSimilarity(v1, v2);
        System.out.println("Cosine similarity: " + similarity);
        
        // Vector addition
        float[] result = new float[4];
        BareMetal.vectorAdd(v1, v2, result);
        System.out.print("v1 + v2 = [");
        for (int i = 0; i < result.length; i++) {
            System.out.print(result[i]);
            if (i < result.length - 1) System.out.print(", ");
        }
        System.out.println("]\n");
    }
    
    private static void example3_MatrixOperations() {
        System.out.println("=== Example 3: Matrix Operations ===");
        
        // Create a 3x3 matrix
        BareMetal.Matrix m = new BareMetal.Matrix(3, 3);
        System.out.println("Created 3x3 matrix");
        
        // Apply flip operations
        m.flipHorizontal();
        System.out.println("Applied horizontal flip");
        
        m.flipVertical();
        System.out.println("Applied vertical flip");
        
        m.flipDiagonal();
        System.out.println("Applied diagonal flip (transpose)");
        
        // Compute determinant
        float det = m.determinant();
        System.out.println("Determinant: " + det);
        
        // Clean up
        m.close();
        System.out.println();
    }
    
    private static void example4_FastMath() {
        System.out.println("=== Example 4: Fast Math ===");
        
        // Square root
        float x = 16.0f;
        float sqrt = BareMetal.fastSqrt(x);
        System.out.println("sqrt(" + x + ") = " + sqrt);
        
        // Reciprocal square root
        float rsqrt = BareMetal.fastRsqrt(x);
        System.out.println("1/sqrt(" + x + ") = " + rsqrt);
        
        // Exponential
        float exp = BareMetal.fastExp(2.0f);
        System.out.println("exp(2.0) = " + exp);
        
        // Logarithm
        float log = BareMetal.fastLog(10.0f);
        System.out.println("log(10.0) = " + log);
        
        System.out.println();
    }
    
    private static void example5_InternalPrograms() {
        System.out.println("=== Example 5: Internal Programs ===");
        
        // Vector analysis
        float[] features1 = {1.0f, 2.0f, 3.0f};
        float[] features2 = {2.0f, 3.0f, 4.0f};
        
        float similarity = InternalPrograms.VectorAnalyzer.analyzeSimilarity(features1, features2);
        System.out.println("Feature similarity: " + similarity);
        
        float distance = InternalPrograms.VectorAnalyzer.euclideanDistance(features1, features2);
        System.out.println("Euclidean distance: " + distance);
        
        // Fast math through internal programs
        float sqrt = InternalPrograms.FastMath.sqrt(25.0f);
        System.out.println("FastMath.sqrt(25.0) = " + sqrt);
        
        // Memory operations
        byte[] src = new byte[1024];
        byte[] dst = new byte[1024];
        for (int i = 0; i < src.length; i++) {
            src[i] = (byte)(i % 256);
        }
        InternalPrograms.MemoryOps.copy(dst, src);
        System.out.println("Copied 1024 bytes using SIMD");
        
        InternalPrograms.MemoryOps.fill(dst, (byte)0xFF);
        System.out.println("Filled 1024 bytes using SIMD");
        
        System.out.println();
    }
}
