package com.termux.lowlevel;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Unit tests for low-level utility functions
 */
public class LowLevelUtilsTest {
    
    private static final float EPSILON = 1e-5f;
    
    @Test
    public void testMemcpyOptimized() {
        byte[] src = {1, 2, 3, 4, 5, 6, 7, 8};
        byte[] dest = new byte[8];
        
        LowLevelUtils.memcpyOptimized(dest, src, 8);
        
        assertArrayEquals(src, dest);
    }
    
    @Test
    public void testMemcpyOptimized_partial() {
        byte[] src = {1, 2, 3, 4, 5, 6, 7, 8};
        byte[] dest = {0, 0, 0, 0, 0, 0, 0, 0};
        
        LowLevelUtils.memcpyOptimized(dest, src, 4);
        
        assertEquals(1, dest[0]);
        assertEquals(2, dest[1]);
        assertEquals(3, dest[2]);
        assertEquals(4, dest[3]);
        assertEquals(0, dest[4]); // Should remain unchanged
    }
    
    @Test
    public void testMemsetOptimized() {
        byte[] array = new byte[10];
        
        LowLevelUtils.memsetOptimized(array, 42, 10);
        
        for (byte b : array) {
            assertEquals(42, b);
        }
    }
    
    @Test
    public void testSqrtFast() {
        float result = LowLevelUtils.sqrtFast(25.0f);
        assertEquals(5.0f, result, EPSILON);
        
        result = LowLevelUtils.sqrtFast(100.0f);
        assertEquals(10.0f, result, EPSILON);
        
        result = LowLevelUtils.sqrtFast(2.0f);
        assertEquals(1.414f, result, 0.01f);
    }
    
    @Test
    public void testSqrtFast_zero() {
        float result = LowLevelUtils.sqrtFast(0.0f);
        assertEquals(0.0f, result, EPSILON);
    }
    
    @Test
    public void testPowFast() {
        float result = LowLevelUtils.powFast(2.0f, 3);
        assertEquals(8.0f, result, EPSILON);
        
        result = LowLevelUtils.powFast(5.0f, 2);
        assertEquals(25.0f, result, EPSILON);
        
        result = LowLevelUtils.powFast(10.0f, 0);
        assertEquals(1.0f, result, EPSILON);
    }
    
    @Test
    public void testPowFast_negativeExponent() {
        float result = LowLevelUtils.powFast(2.0f, -2);
        assertEquals(0.25f, result, EPSILON);
    }
    
    @Test
    public void testDotProduct() {
        float[] v1 = {1.0f, 2.0f, 3.0f};
        float[] v2 = {4.0f, 5.0f, 6.0f};
        
        float result = LowLevelUtils.dotProduct(v1, v2);
        
        // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
        assertEquals(32.0f, result, EPSILON);
    }
    
    @Test
    public void testDotProduct_orthogonal() {
        float[] v1 = {1.0f, 0.0f, 0.0f};
        float[] v2 = {0.0f, 1.0f, 0.0f};
        
        float result = LowLevelUtils.dotProduct(v1, v2);
        
        assertEquals(0.0f, result, EPSILON);
    }
    
    @Test
    public void testMagnitude() {
        float[] v = {3.0f, 4.0f, 0.0f};
        
        float mag = LowLevelUtils.magnitude(v);
        
        // 3-4-5 triangle
        assertEquals(5.0f, mag, EPSILON);
    }
    
    @Test
    public void testMagnitude_unitVector() {
        float[] v = {1.0f, 0.0f, 0.0f};
        
        float mag = LowLevelUtils.magnitude(v);
        
        assertEquals(1.0f, mag, EPSILON);
    }
    
    @Test
    public void testNormalize() {
        float[] v = {3.0f, 4.0f, 0.0f};
        
        LowLevelUtils.normalize(v);
        
        // Check magnitude is 1.0
        float mag = LowLevelUtils.magnitude(v);
        assertEquals(1.0f, mag, EPSILON);
        
        // Check direction is preserved (proportions)
        assertEquals(3.0f / 5.0f, v[0], EPSILON);
        assertEquals(4.0f / 5.0f, v[1], EPSILON);
        assertEquals(0.0f, v[2], EPSILON);
    }
}
