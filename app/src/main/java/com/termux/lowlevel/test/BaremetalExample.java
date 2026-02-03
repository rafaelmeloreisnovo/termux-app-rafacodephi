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
        System.out.println("=== Example 3: Matrix Operations (RAFAELIA) ===");
        
        // Create a 3x3 identity matrix
        BareMetal.Matrix I = new BareMetal.Matrix(3, 3);
        I.setIdentity();
        System.out.println("Created 3x3 identity matrix");
        System.out.println("Trace of I: " + I.trace());
        System.out.println("Determinant of I: " + I.determinant());
        
        // Create a test matrix
        BareMetal.Matrix A = new BareMetal.Matrix(3, 3);
        float[] dataA = {
            2.0f, 1.0f, 1.0f,
            1.0f, 3.0f, 2.0f,
            1.0f, 2.0f, 2.0f
        };
        A.setData(dataA);
        System.out.println("\nMatrix A created with determinant: " + A.determinant());
        System.out.println("Trace of A: " + A.trace());
        
        // RAFAELIA Flip operations - deterministic transformations
        BareMetal.Matrix Ah = new BareMetal.Matrix(3, 3);
        Ah.setData(dataA);
        Ah.flipHorizontal();
        System.out.println("\nApplied horizontal flip");
        System.out.println("Det after horizontal flip: " + Ah.determinant());
        Ah.close();
        
        BareMetal.Matrix Av = new BareMetal.Matrix(3, 3);
        Av.setData(dataA);
        Av.flipVertical();
        System.out.println("Applied vertical flip");
        System.out.println("Det after vertical flip: " + Av.determinant());
        Av.close();
        
        // Transpose (diagonal flip)
        BareMetal.Matrix At = new BareMetal.Matrix(3, 3);
        A.transposeInto(At);
        System.out.println("\nTranspose computed");
        System.out.println("Det of transpose: " + At.determinant());
        
        // Matrix multiplication
        BareMetal.Matrix AAt = new BareMetal.Matrix(3, 3);
        A.multiplyInto(At, AAt);
        System.out.println("\nA × A^T computed");
        System.out.println("Det(A × A^T): " + AAt.determinant());
        
        // Matrix addition and subtraction
        BareMetal.Matrix sum = new BareMetal.Matrix(3, 3);
        A.addInto(I, sum);
        System.out.println("\nA + I computed");
        System.out.println("Trace(A + I): " + sum.trace());
        
        BareMetal.Matrix diff = new BareMetal.Matrix(3, 3);
        A.subtractInto(I, diff);
        System.out.println("A - I computed");
        System.out.println("Trace(A - I): " + diff.trace());
        
        // Scalar multiplication
        BareMetal.Matrix A2 = new BareMetal.Matrix(3, 3);
        A2.setData(dataA);
        A2.scale(2.0f);
        System.out.println("\n2A computed");
        System.out.println("Det(2A): " + A2.determinant());
        System.out.println("Expected: " + (8 * A.determinant()));
        
        // Matrix inversion
        BareMetal.Matrix Ainv = new BareMetal.Matrix(3, 3);
        if (A.invertInto(Ainv)) {
            System.out.println("\nA^-1 computed");
            System.out.println("Det(A^-1): " + Ainv.determinant());
            System.out.println("Expected: " + (1.0f / A.determinant()));
            
            // Verify A × A^-1 ≈ I
            BareMetal.Matrix verify = new BareMetal.Matrix(3, 3);
            A.multiplyInto(Ainv, verify);
            System.out.println("A × A^-1 computed (should be ≈ I)");
            System.out.println("Trace(A × A^-1): " + verify.trace() + " (expected: 3.0)");
            verify.close();
            Ainv.close();
        } else {
            System.out.println("\nMatrix is singular (cannot invert)");
            Ainv.close();
        }
        
        // Solve linear system Ax = b
        float[] b = {6.0f, 11.0f, 9.0f};
        float[] x = new float[3];
        if (A.solveInto(b, x)) {
            System.out.println("\nSolved Ax = b");
            System.out.print("Solution x = [");
            for (int i = 0; i < x.length; i++) {
                System.out.printf("%.2f", x[i]);
                if (i < x.length - 1) System.out.print(", ");
            }
            System.out.println("]");
            
            // Verify solution
            float[] Ax = new float[3];
            float[] dataAArray = new float[9];
            A.getDataInto(dataAArray);
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    Ax[i] += dataAArray[i * 3 + j] * x[j];
                }
            }
            System.out.print("Verification Ax = [");
            for (int i = 0; i < Ax.length; i++) {
                System.out.printf("%.2f", Ax[i]);
                if (i < Ax.length - 1) System.out.print(", ");
            }
            System.out.println("] (should equal b)");
        } else {
            System.out.println("\nLinear system could not be solved");
        }
        
        // Clean up
        I.close();
        A.close();
        At.close();
        AAt.close();
        sum.close();
        diff.close();
        A2.close();
        
        System.out.println("\nAll matrix operations completed successfully!");
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
