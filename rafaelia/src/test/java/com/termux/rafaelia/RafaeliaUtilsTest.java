package com.termux.rafaelia;

import org.junit.Test;
import org.junit.Before;
import org.junit.Assume;
import static org.junit.Assert.*;

/**
 * Unit tests for Rafaelia utility functions.
 * Tests memory operations, mathematical functions, and vector operations.
 * 
 * Tests that require native library are skipped when the library is not available.
 */
public class RafaeliaUtilsTest {
    
    private static final float EPSILON = 1e-5f;
    
    /**
     * Skip test if native library is required but not available.
     */
    private void requireNativeLibrary() {
        Assume.assumeTrue("Native library not available - skipping test", 
            RafaeliaUtils.isNativeAvailable());
    }
    
    @Before
    public void setUp() {
        // Native library availability is checked in individual tests that need it
    }
    
    // ==================== Memory Operations Tests (require native) ====================
    
    @Test
    public void testMemcpy() {
        requireNativeLibrary();
        byte[] src = {1, 2, 3, 4, 5, 6, 7, 8};
        byte[] dest = new byte[8];
        
        RafaeliaUtils.memcpy(dest, src, 8);
        
        assertArrayEquals("Full copy should match source", src, dest);
    }
    
    @Test
    public void testMemcpy_partial() {
        requireNativeLibrary();
        byte[] src = {1, 2, 3, 4, 5, 6, 7, 8};
        byte[] dest = {0, 0, 0, 0, 0, 0, 0, 0};
        
        RafaeliaUtils.memcpy(dest, src, 4);
        
        assertEquals(1, dest[0]);
        assertEquals(2, dest[1]);
        assertEquals(3, dest[2]);
        assertEquals(4, dest[3]);
        assertEquals(0, dest[4]); // Should remain unchanged
    }
    
    @Test
    public void testMemset() {
        requireNativeLibrary();
        byte[] array = new byte[10];
        
        RafaeliaUtils.memset(array, 42, 10);
        
        for (byte b : array) {
            assertEquals(42, b);
        }
    }
    
    @Test
    public void testMemset_partial() {
        requireNativeLibrary();
        byte[] array = new byte[10];
        
        RafaeliaUtils.memset(array, 99, 5);
        
        for (int i = 0; i < 5; i++) {
            assertEquals(99, array[i]);
        }
        for (int i = 5; i < 10; i++) {
            assertEquals(0, array[i]);
        }
    }
    
    // ==================== Mathematical Operations Tests (pure Java fallback available) ====================
    
    @Test
    public void testSqrt() {
        assertEquals(5.0f, RafaeliaUtils.sqrt(25.0f), EPSILON);
        assertEquals(10.0f, RafaeliaUtils.sqrt(100.0f), EPSILON);
        assertEquals(1.414f, RafaeliaUtils.sqrt(2.0f), 0.01f);
    }
    
    @Test
    public void testSqrt_zero() {
        assertEquals(0.0f, RafaeliaUtils.sqrt(0.0f), EPSILON);
    }
    
    @Test
    public void testSqrt_edgeCases() {
        assertEquals(1.0f, RafaeliaUtils.sqrt(1.0f), EPSILON);
        assertEquals(0.0f, RafaeliaUtils.sqrt(-1.0f), EPSILON); // Negative should return 0
    }
    
    @Test
    public void testPow() {
        assertEquals(8.0f, RafaeliaUtils.pow(2.0f, 3), EPSILON);
        assertEquals(25.0f, RafaeliaUtils.pow(5.0f, 2), EPSILON);
        assertEquals(1.0f, RafaeliaUtils.pow(10.0f, 0), EPSILON);
    }
    
    @Test
    public void testPow_negativeExponent() {
        assertEquals(0.25f, RafaeliaUtils.pow(2.0f, -2), EPSILON);
        assertEquals(0.1f, RafaeliaUtils.pow(10.0f, -1), EPSILON);
    }
    
    @Test
    public void testPow_edgeCases() {
        assertEquals(0.0f, RafaeliaUtils.pow(0.0f, 5), EPSILON);
        assertEquals(1.0f, RafaeliaUtils.pow(1.0f, 100), EPSILON);
        assertEquals(1.0f, RafaeliaUtils.pow(1.0f, -100), EPSILON);
    }
    
    @Test
    public void testPow_commonExponents() {
        // Test optimized cases
        assertEquals(4.0f, RafaeliaUtils.pow(2.0f, 2), EPSILON);
        assertEquals(16.0f, RafaeliaUtils.pow(2.0f, 4), EPSILON);
    }
    
    // ==================== Vector Operations Tests ====================
    
    @Test
    public void testDotProduct() {
        float[] v1 = {1.0f, 2.0f, 3.0f};
        float[] v2 = {4.0f, 5.0f, 6.0f};
        
        float result = RafaeliaUtils.dotProduct(v1, v2);
        
        // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
        assertEquals(32.0f, result, EPSILON);
    }
    
    @Test
    public void testDotProduct_orthogonal() {
        float[] v1 = {1.0f, 0.0f, 0.0f};
        float[] v2 = {0.0f, 1.0f, 0.0f};
        
        float result = RafaeliaUtils.dotProduct(v1, v2);
        
        assertEquals(0.0f, result, EPSILON);
    }
    
    @Test
    public void testDotProduct_nullHandling() {
        float[] v = {1.0f, 2.0f};
        assertEquals(0.0f, RafaeliaUtils.dotProduct(null, v), EPSILON);
        assertEquals(0.0f, RafaeliaUtils.dotProduct(v, null), EPSILON);
        assertEquals(0.0f, RafaeliaUtils.dotProduct(null, null), EPSILON);
    }
    
    @Test
    public void testDotProduct_mismatchedLengths() {
        float[] v1 = {1.0f, 2.0f, 3.0f};
        float[] v2 = {4.0f, 5.0f};
        
        assertEquals(0.0f, RafaeliaUtils.dotProduct(v1, v2), EPSILON);
    }
    
    @Test
    public void testMagnitude() {
        float[] v = {3.0f, 4.0f, 0.0f};
        
        float mag = RafaeliaUtils.magnitude(v);
        
        // 3-4-5 triangle
        assertEquals(5.0f, mag, EPSILON);
    }
    
    @Test
    public void testMagnitude_unitVector() {
        float[] v = {1.0f, 0.0f, 0.0f};
        
        float mag = RafaeliaUtils.magnitude(v);
        
        assertEquals(1.0f, mag, EPSILON);
    }
    
    @Test
    public void testMagnitude_nullHandling() {
        assertEquals(0.0f, RafaeliaUtils.magnitude(null), EPSILON);
        assertEquals(0.0f, RafaeliaUtils.magnitude(new float[0]), EPSILON);
    }
    
    @Test
    public void testNormalize() {
        float[] v = {3.0f, 4.0f, 0.0f};
        
        RafaeliaUtils.normalize(v);
        
        // Check magnitude is 1.0
        float mag = RafaeliaUtils.magnitude(v);
        assertEquals(1.0f, mag, EPSILON);
        
        // Check direction is preserved (proportions)
        assertEquals(3.0f / 5.0f, v[0], EPSILON);
        assertEquals(4.0f / 5.0f, v[1], EPSILON);
        assertEquals(0.0f, v[2], EPSILON);
    }
    
    @Test
    public void testNormalize_nullHandling() {
        // Should not throw exception
        RafaeliaUtils.normalize(null);
        RafaeliaUtils.normalize(new float[0]);
    }
    
    @Test
    public void testNormalize_zeroVector() {
        float[] v = {0.0f, 0.0f, 0.0f};
        RafaeliaUtils.normalize(v);
        
        // Should remain unchanged
        assertEquals(0.0f, v[0], EPSILON);
        assertEquals(0.0f, v[1], EPSILON);
        assertEquals(0.0f, v[2], EPSILON);
    }
    
    // ==================== Integration Tests ====================
    
    @Test
    public void testVectorOperations_integration() {
        float[] v1 = {1.0f, 2.0f, 3.0f};
        float[] v2 = {4.0f, 5.0f, 6.0f};
        
        // Test dot product
        float dot = RafaeliaUtils.dotProduct(v1, v2);
        assertEquals(32.0f, dot, EPSILON);
        
        // Test magnitudes
        float mag1 = RafaeliaUtils.magnitude(v1);
        float mag2 = RafaeliaUtils.magnitude(v2);
        
        assertTrue(mag1 > 0);
        assertTrue(mag2 > 0);
        
        // Normalize and test
        float[] v1Norm = v1.clone();
        RafaeliaUtils.normalize(v1Norm);
        assertEquals(1.0f, RafaeliaUtils.magnitude(v1Norm), EPSILON);
    }
}
