package com.termux.rafaelia;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Teste de Mesa (Desk Checking / Trace Table Testing) for mathematical operations.
 * 
 * This test class focuses on pure Java methods that do not require native libraries.
 * The pow() function is implemented in Java and can be tested without JNI.
 * 
 * @author instituto-Rafael
 * @version 1.0
 */
public class MathTesteDeMesaTest {
    
    private static final float EPSILON = 1e-5f;
    private static final float STRICT_EPSILON = 1e-10f;
    
    // ==================== TRACE TABLE: pow() Function ====================
    // Tests exponentiation by squaring algorithm step-by-step
    
    /**
     * Trace Table for pow(2.0, 5):
     * 
     * Algorithm: Exponentiation by squaring (binary method)
     * exp = 5 = 101 in binary
     * 
     * | Step | absExp | absExp (binary) | current | result | absExp & 1 | Action                    |
     * |------|--------|-----------------|---------|--------|------------|---------------------------|
     * | init | 5      | 101             | 2.0     | 1.0    | -          | initialize                |
     * | 0    | 5      | 101             | 2.0     | 1.0    | 1          | result *= current (1*2=2) |
     * |      | 2      | 010             | 4.0     | 2.0    | -          | absExp >>= 1, current²    |
     * | 1    | 2      | 010             | 4.0     | 2.0    | 0          | skip (bit is 0)           |
     * |      | 1      | 001             | 16.0    | 2.0    | -          | absExp >>= 1, current²    |
     * | 2    | 1      | 001             | 16.0    | 2.0    | 1          | result *= current (2*16)  |
     * |      | 0      | 000             | -       | 32.0   | -          | absExp becomes 0, loop ends |
     * | end  | 0      | 000             | -       | 32.0   | -          | exit loop (absExp == 0)   |
     * 
     * Expected: 32.0
     */
    @Test
    public void testPow_traceTable_positiveOddExponent() {
        // Input values
        float base = 2.0f;
        int exp = 5;
        
        // Execute computation
        float result = RafaeliaUtils.pow(base, exp);
        
        // Verify against trace table result
        assertEquals("pow(2, 5) should equal 32", 32.0f, result, EPSILON);
    }
    
    /**
     * Trace Table for pow(3.0, 4):
     * 
     * Algorithm: Exponentiation by squaring (binary method)
     * exp = 4 = 100 in binary
     * 
     * | Step | absExp | absExp (binary) | current | result | absExp & 1 | Action                    |
     * |------|--------|-----------------|---------|--------|------------|---------------------------|
     * | init | 4      | 100             | 3.0     | 1.0    | -          | initialize                |
     * | 0    | 4      | 100             | 3.0     | 1.0    | 0          | skip (bit is 0)           |
     * |      | 2      | 010             | 9.0     | 1.0    | -          | absExp >>= 1, current²    |
     * | 1    | 2      | 010             | 9.0     | 1.0    | 0          | skip (bit is 0)           |
     * |      | 1      | 001             | 81.0    | 1.0    | -          | absExp >>= 1, current²    |
     * | 2    | 1      | 001             | 81.0    | 1.0    | 1          | result *= current (1*81)  |
     * |      | 0      | 000             | -       | 81.0   | -          | absExp becomes 0, loop ends |
     * | end  | 0      | 000             | -       | 81.0   | -          | exit loop                 |
     * 
     * Expected: 81.0
     */
    @Test
    public void testPow_traceTable_positiveEvenExponent() {
        float result = RafaeliaUtils.pow(3.0f, 4);
        assertEquals("pow(3, 4) should equal 81", 81.0f, result, EPSILON);
    }
    
    /**
     * Trace Table for pow(2.0, -2):
     * 
     * Since exp < 0, we compute pow(2, 2) first, then take reciprocal.
     * exp = 2 = 10 in binary
     * 
     * | Step | absExp | absExp (binary) | current | result | absExp & 1 | Action                    |
     * |------|--------|-----------------|---------|--------|------------|---------------------------|
     * | init | 2      | 10              | 2.0     | 1.0    | -          | negative=true, absExp=2   |
     * | 0    | 2      | 10              | 2.0     | 1.0    | 0          | skip (bit is 0)           |
     * |      | 1      | 01              | 4.0     | 1.0    | -          | absExp >>= 1, current²    |
     * | 1    | 1      | 01              | 4.0     | 1.0    | 1          | result *= current (1*4=4) |
     * |      | 0      | 00              | -       | 4.0    | -          | absExp becomes 0, loop ends |
     * | end  | 0      | 00              | -       | 4.0    | -          | exit loop                 |
     * | final| -      | -               | -       | 0.25   | -          | return 1.0/4.0 = 0.25     |
     * 
     * Expected: 0.25
     */
    @Test
    public void testPow_traceTable_negativeExponent() {
        float result = RafaeliaUtils.pow(2.0f, -2);
        assertEquals("pow(2, -2) should equal 0.25", 0.25f, result, EPSILON);
    }
    
    /**
     * Trace Table for pow(10.0, 3):
     * 
     * exp = 3 = 11 in binary
     * 
     * | Step | absExp | current   | result   | absExp & 1 | Action                               |
     * |------|--------|-----------|----------|------------|--------------------------------------|
     * | init | 3      | 10.0      | 1.0      | -          | initialize                           |
     * | 0    | 3      | 10.0      | 1.0      | 1          | result *= 10 = 10                    |
     * |      | 1      | 100.0     | 10.0     | -          | shift and square                     |
     * | 1    | 1      | 100.0     | 10.0     | 1          | result *= 100 = 1000                 |
     * |      | 0      | -         | 1000.0   | -          | shift, absExp == 0 so loop will end  |
     * | end  | 0      | -         | 1000.0   | -          | exit loop                            |
     * 
     * Expected: 1000.0
     */
    @Test
    public void testPow_traceTable_consecutiveBits() {
        float result = RafaeliaUtils.pow(10.0f, 3);
        assertEquals("pow(10, 3) should equal 1000", 1000.0f, result, EPSILON);
    }
    
    // ==================== BOUNDARY CONDITION TESTS ====================
    
    /**
     * Boundary: Zero exponent
     * Any non-zero base raised to power 0 equals 1
     */
    @Test
    public void testPow_boundary_zeroExponent() {
        assertEquals("pow(100, 0) should be 1", 1.0f, RafaeliaUtils.pow(100.0f, 0), EPSILON);
        assertEquals("pow(-5, 0) should be 1", 1.0f, RafaeliaUtils.pow(-5.0f, 0), EPSILON);
        assertEquals("pow(0.001, 0) should be 1", 1.0f, RafaeliaUtils.pow(0.001f, 0), EPSILON);
        assertEquals("pow(999999, 0) should be 1", 1.0f, RafaeliaUtils.pow(999999.0f, 0), EPSILON);
    }
    
    /**
     * Boundary: Zero base
     * 0 raised to any positive power equals 0
     */
    @Test
    public void testPow_boundary_zeroBase() {
        assertEquals("pow(0, 1) should be 0", 0.0f, RafaeliaUtils.pow(0.0f, 1), EPSILON);
        assertEquals("pow(0, 10) should be 0", 0.0f, RafaeliaUtils.pow(0.0f, 10), EPSILON);
        assertEquals("pow(0, 100) should be 0", 0.0f, RafaeliaUtils.pow(0.0f, 100), EPSILON);
    }
    
    /**
     * Boundary: Base equals 1
     * 1 raised to any power equals 1
     */
    @Test
    public void testPow_boundary_oneBase() {
        assertEquals("pow(1, 0) should be 1", 1.0f, RafaeliaUtils.pow(1.0f, 0), EPSILON);
        assertEquals("pow(1, 100) should be 1", 1.0f, RafaeliaUtils.pow(1.0f, 100), EPSILON);
        assertEquals("pow(1, -100) should be 1", 1.0f, RafaeliaUtils.pow(1.0f, -100), EPSILON);
        assertEquals("pow(1, MAX_INT) should be 1", 1.0f, RafaeliaUtils.pow(1.0f, Integer.MAX_VALUE), EPSILON);
    }
    
    /**
     * Boundary: Exponent equals 1
     * Any base raised to power 1 equals the base
     */
    @Test
    public void testPow_boundary_oneExponent() {
        assertEquals("pow(5, 1) should be 5", 5.0f, RafaeliaUtils.pow(5.0f, 1), EPSILON);
        assertEquals("pow(-3, 1) should be -3", -3.0f, RafaeliaUtils.pow(-3.0f, 1), EPSILON);
        assertEquals("pow(0.5, 1) should be 0.5", 0.5f, RafaeliaUtils.pow(0.5f, 1), EPSILON);
    }
    
    /**
     * Boundary: Exponent equals 2 (squaring)
     */
    @Test
    public void testPow_boundary_twoExponent() {
        assertEquals("pow(5, 2) should be 25", 25.0f, RafaeliaUtils.pow(5.0f, 2), EPSILON);
        assertEquals("pow(-3, 2) should be 9", 9.0f, RafaeliaUtils.pow(-3.0f, 2), EPSILON);
        assertEquals("pow(0.5, 2) should be 0.25", 0.25f, RafaeliaUtils.pow(0.5f, 2), EPSILON);
    }
    
    // ==================== NEGATIVE BASE TESTS ====================
    
    @Test
    public void testPow_negativeBase_evenExponent() {
        // Negative base with even exponent yields positive result
        assertEquals("pow(-2, 2) should be 4", 4.0f, RafaeliaUtils.pow(-2.0f, 2), EPSILON);
        assertEquals("pow(-2, 4) should be 16", 16.0f, RafaeliaUtils.pow(-2.0f, 4), EPSILON);
        assertEquals("pow(-3, 2) should be 9", 9.0f, RafaeliaUtils.pow(-3.0f, 2), EPSILON);
    }
    
    @Test
    public void testPow_negativeBase_oddExponent() {
        // Negative base with odd exponent yields negative result
        assertEquals("pow(-2, 3) should be -8", -8.0f, RafaeliaUtils.pow(-2.0f, 3), EPSILON);
        assertEquals("pow(-2, 5) should be -32", -32.0f, RafaeliaUtils.pow(-2.0f, 5), EPSILON);
        assertEquals("pow(-3, 3) should be -27", -27.0f, RafaeliaUtils.pow(-3.0f, 3), EPSILON);
    }
    
    // ==================== MODERATELY LARGE EXPONENT TESTS ====================
    
    @Test
    public void testPow_powerOfTwo() {
        assertEquals("pow(2, 10) should be 1024", 1024.0f, RafaeliaUtils.pow(2.0f, 10), EPSILON);
        assertEquals("pow(2, 16) should be 65536", 65536.0f, RafaeliaUtils.pow(2.0f, 16), EPSILON);
        // Use a slightly looser tolerance for larger exponents on float to allow for
        // accumulated rounding error in the exponentiation algorithm, while still
        // verifying that the result is numerically close to 1048576 (2^20).
        assertEquals("pow(2, 20) should be 1048576", 1048576.0f, RafaeliaUtils.pow(2.0f, 20), 1.0f);
    }
    
    @Test
    public void testPow_moderateExponents() {
        // Verify algorithm works for various exponent patterns
        assertEquals("pow(3, 7) should be 2187", 2187.0f, RafaeliaUtils.pow(3.0f, 7), EPSILON);
        assertEquals("pow(5, 5) should be 3125", 3125.0f, RafaeliaUtils.pow(5.0f, 5), EPSILON);
        assertEquals("pow(7, 3) should be 343", 343.0f, RafaeliaUtils.pow(7.0f, 3), EPSILON);
    }
    
    // ==================== FRACTIONAL BASE TESTS ====================
    
    @Test
    public void testPow_fractionalBase() {
        assertEquals("pow(0.5, 2) should be 0.25", 0.25f, RafaeliaUtils.pow(0.5f, 2), EPSILON);
        assertEquals("pow(0.5, 3) should be 0.125", 0.125f, RafaeliaUtils.pow(0.5f, 3), EPSILON);
        assertEquals("pow(0.1, 2) should be 0.01", 0.01f, RafaeliaUtils.pow(0.1f, 2), EPSILON);
    }
    
    @Test
    public void testPow_fractionalBase_negativeExponent() {
        assertEquals("pow(0.5, -1) should be 2", 2.0f, RafaeliaUtils.pow(0.5f, -1), EPSILON);
        assertEquals("pow(0.5, -2) should be 4", 4.0f, RafaeliaUtils.pow(0.5f, -2), EPSILON);
        assertEquals("pow(0.25, -1) should be 4", 4.0f, RafaeliaUtils.pow(0.25f, -1), EPSILON);
    }
    
    // ==================== DETERMINISM TESTS ====================
    
    @Test
    public void testPow_determinism() {
        // Same inputs should always produce same outputs
        for (int i = 0; i < 100; i++) {
            assertEquals("pow(2, 3) should be deterministic", 
                8.0f, RafaeliaUtils.pow(2.0f, 3), STRICT_EPSILON);
            assertEquals("pow(3, 4) should be deterministic", 
                81.0f, RafaeliaUtils.pow(3.0f, 4), STRICT_EPSILON);
            assertEquals("pow(2, -2) should be deterministic", 
                0.25f, RafaeliaUtils.pow(2.0f, -2), STRICT_EPSILON);
            assertEquals("pow(-2, 3) should be deterministic", 
                -8.0f, RafaeliaUtils.pow(-2.0f, 3), STRICT_EPSILON);
        }
    }
    
    // ==================== COMMUTATIVITY OF BASE AND RESULT ====================
    
    @Test
    public void testPow_inverseRelationship() {
        // pow(x, n) * pow(x, -n) should equal 1
        float base = 3.0f;
        int exp = 4;
        
        float pos = RafaeliaUtils.pow(base, exp);
        float neg = RafaeliaUtils.pow(base, -exp);
        
        assertEquals("pow(3,4) * pow(3,-4) should be 1", 1.0f, pos * neg, 0.001f);
    }
    
    @Test
    public void testPow_exponentAddition() {
        // pow(x, a) * pow(x, b) = pow(x, a+b)
        float base = 2.0f;
        int a = 3;
        int b = 4;
        
        float powA = RafaeliaUtils.pow(base, a);
        float powB = RafaeliaUtils.pow(base, b);
        float powAB = RafaeliaUtils.pow(base, a + b);
        
        assertEquals("pow(2,3) * pow(2,4) should equal pow(2,7)", 
            powAB, powA * powB, EPSILON);
    }
    
    @Test
    public void testPow_exponentMultiplication() {
        // (pow(x, a))^b = pow(x, a*b)
        float base = 2.0f;
        int a = 3;
        int b = 2;
        
        float powA = RafaeliaUtils.pow(base, a);  // 8
        float powAPowB = RafaeliaUtils.pow(powA, b);  // 64
        float powATimesB = RafaeliaUtils.pow(base, a * b);  // 64
        
        assertEquals("(pow(2,3))^2 should equal pow(2,6)", 
            powATimesB, powAPowB, EPSILON);
    }
}
