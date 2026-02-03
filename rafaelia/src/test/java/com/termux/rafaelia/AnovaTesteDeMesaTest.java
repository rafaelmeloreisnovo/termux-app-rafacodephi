package com.termux.rafaelia;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Teste de Mesa (Desk Checking / Trace Table Testing) for AnovaResult class.
 * 
 * This test class implements systematic manual testing methodology where we trace 
 * through code logic step-by-step with sample inputs to verify correctness.
 * 
 * These tests are pure Java and do not require native libraries.
 * 
 * The methodology follows the RAFAELIA ψχρΔΣΩ cycle:
 * - ψ (psi): Perception - Define test inputs
 * - χ (chi): Feedback - Check expected outcomes
 * - ρ (rho): Expansion - Transform and compute
 * - Δ (Delta): Validation - Verify results
 * - Σ (Sigma): Execution - Execute and measure
 * - Ω (Omega): Alignment - Ensure coherence
 * 
 * @author instituto-Rafael
 * @version 1.0
 */
public class AnovaTesteDeMesaTest {
    
    private static final float EPSILON = 1e-5f;
    private static final float STRICT_EPSILON = 1e-10f;
    
    // ==================== ANOVA TRACE TABLE TESTS ====================
    
    /**
     * Trace Table for AnovaResult R-squared calculation:
     * 
     * Given: ssTotal=100, ssModel=80, ssError=20
     * 
     * | Calculation      | Formula          | Value  |
     * |------------------|------------------|--------|
     * | R²               | ssModel/ssTotal  | 0.8    |
     * | 1 - R²           | ssError/ssTotal  | 0.2    |
     * 
     * Verification: R² + (1 - R²) = 1.0
     */
    @Test
    public void testAnovaResult_traceTable_RSquared() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        float rSquared = result.getRSquared();
        float oneMinusRSquared = 1.0f - rSquared;
        
        assertEquals("R² should be 0.8", 0.8f, rSquared, EPSILON);
        assertEquals("1 - R² should be 0.2", 0.2f, oneMinusRSquared, EPSILON);
        assertEquals("R² + (1-R²) should be 1.0", 1.0f, rSquared + oneMinusRSquared, EPSILON);
    }
    
    /**
     * Trace Table for AnovaResult Adjusted R-squared:
     * 
     * Given: ssTotal=100, ssError=20, n=10, p=2
     * 
     * | Calculation          | Formula                      | Value       |
     * |----------------------|------------------------------|-------------|
     * | numerator            | ssError/(n-p) = 20/8         | 2.5         |
     * | denominator          | ssTotal/(n-1) = 100/9        | 11.111...   |
     * | ratio                | num/denom                    | 0.225       |
     * | Adjusted R²          | 1 - ratio                    | 0.775       |
     */
    @Test
    public void testAnovaResult_traceTable_AdjustedRSquared() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        int n = 10;
        float adjustedRSquared = result.getAdjustedRSquared(n);
        
        // Manual calculation: 1 - (20/8) / (100/9) = 1 - 2.5/11.111 = 1 - 0.225 = 0.775
        float expected = 1.0f - (20.0f / 8.0f) / (100.0f / 9.0f);
        assertEquals("Adjusted R² calculation", expected, adjustedRSquared, EPSILON);
    }
    
    /**
     * Trace Table for AnovaResult F-statistic:
     * 
     * Given: ssModel=80, ssError=20, n=10, p=2
     * 
     * | Calculation | Formula               | Value  |
     * |-------------|-----------------------|--------|
     * | MS_Model    | ssModel/(p-1) = 80/1  | 80.0   |
     * | MS_Error    | ssError/(n-p) = 20/8  | 2.5    |
     * | F           | MS_Model/MS_Error     | 32.0   |
     */
    @Test
    public void testAnovaResult_traceTable_FStatistic() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        int n = 10;
        float fStatistic = result.getFStatistic(n);
        
        // Manual calculation: (80/1) / (20/8) = 80 / 2.5 = 32
        assertEquals("F-statistic should be 32", 32.0f, fStatistic, EPSILON);
    }
    
    // ==================== BOUNDARY CONDITION TESTS ====================
    
    @Test
    public void testAnovaResult_perfectFit() {
        // Perfect fit: SS_E = 0, R² = 1
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 100.0f, 0.0f);
        
        assertEquals("Perfect fit R² should be 1.0", 1.0f, result.getRSquared(), EPSILON);
    }
    
    @Test
    public void testAnovaResult_noFit() {
        // No fit: SS_M = 0, R² = 0
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 0.0f, 100.0f);
        
        assertEquals("No fit R² should be 0.0", 0.0f, result.getRSquared(), EPSILON);
    }
    
    @Test
    public void testAnovaResult_insufficientDataAdjustedR() {
        // When n <= p, adjusted R² returns 0
        float[] coefs = {1.0f, 2.0f}; // p = 2
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        assertEquals("n=2 should return 0", 0.0f, result.getAdjustedRSquared(2), EPSILON);
        assertEquals("n=1 should return 0", 0.0f, result.getAdjustedRSquared(1), EPSILON);
        assertEquals("n=0 should return 0", 0.0f, result.getAdjustedRSquared(0), EPSILON);
    }
    
    @Test
    public void testAnovaResult_insufficientDataFStatistic() {
        // When n <= p or p < 2, F returns 0
        float[] coefs = {1.0f, 2.0f}; // p = 2
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        assertEquals("n=2 should return 0", 0.0f, result.getFStatistic(2), EPSILON);
        assertEquals("n=1 should return 0", 0.0f, result.getFStatistic(1), EPSILON);
    }
    
    @Test
    public void testAnovaResult_singleCoefficient() {
        // p = 1 means F cannot be computed (division by p-1=0)
        float[] coefs = {5.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 50.0f, 50.0f);
        
        assertEquals("p=1 should return 0 for F", 0.0f, result.getFStatistic(10), EPSILON);
    }
    
    // ==================== INPUT VALIDATION TESTS ====================
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_nullCoefficientsValidation() {
        new AnovaResult(null, 100.0f, 80.0f, 20.0f);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_emptyCoefficientsValidation() {
        new AnovaResult(new float[0], 100.0f, 80.0f, 20.0f);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_negativeSSTotalValidation() {
        new AnovaResult(new float[]{1.0f}, -100.0f, 80.0f, 20.0f);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_negativeSSModelValidation() {
        new AnovaResult(new float[]{1.0f}, 100.0f, -80.0f, 20.0f);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_negativeSSErrorValidation() {
        new AnovaResult(new float[]{1.0f}, 100.0f, 80.0f, -20.0f);
    }
    
    // ==================== NUMERICAL STABILITY TESTS ====================
    
    @Test
    public void testAnovaResult_getRSquared_nearZeroTotal() {
        // When ssTotal is near zero, R² should handle gracefully
        float[] coefs = {1.0f};
        AnovaResult result = new AnovaResult(coefs, 1e-15f, 0.0f, 0.0f);
        
        // Should not throw and should return 0
        float rSquared = result.getRSquared();
        assertEquals("Near-zero ssTotal should return 0", 0.0f, rSquared, EPSILON);
    }
    
    @Test
    public void testAnovaResult_getAdjustedRSquared_nearZeroDenominator() {
        // When ssTotal is near zero, denominator approaches zero
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 1e-15f, 0.0f, 0.0f);
        
        // Should handle gracefully
        float adjR2 = result.getAdjustedRSquared(10);
        assertTrue("Should be a valid number", !Float.isNaN(adjR2));
    }
    
    @Test
    public void testAnovaResult_getFStatistic_zeroError() {
        // When ssError is zero (perfect fit), F should be infinity
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 100.0f, 0.0f);
        
        float fStat = result.getFStatistic(10);
        assertEquals("Perfect fit should give infinite F", Float.POSITIVE_INFINITY, fStat, 0);
    }
    
    @Test
    public void testAnovaResult_largeValues() {
        // Test with large values
        float[] coefs = {1e6f, 2e6f};
        AnovaResult result = new AnovaResult(coefs, 1e12f, 8e11f, 2e11f);
        
        // R² = 8e11 / 1e12 = 0.8
        assertEquals("R² with large values", 0.8f, result.getRSquared(), EPSILON);
    }
    
    // ==================== IMMUTABILITY TESTS ====================
    
    @Test
    public void testAnovaResult_constructorImmutability() {
        float[] originalCoefs = {1.0f, 2.0f, 3.0f};
        AnovaResult result = new AnovaResult(originalCoefs, 100.0f, 80.0f, 20.0f);
        
        // Modify original array
        originalCoefs[0] = 999.0f;
        
        // Result should be unchanged (defensive copy was made)
        // Use getCoefficients() to verify internal state wasn't affected
        float[] internalCoefs = result.getCoefficients();
        assertEquals("Constructor should make defensive copy", 1.0f, internalCoefs[0], EPSILON);
    }
    
    @Test
    public void testAnovaResult_getCoefficientsImmutability() {
        float[] coefs = {1.0f, 2.0f, 3.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // Get coefficients and modify
        float[] returned = result.getCoefficients();
        returned[0] = 888.0f;
        
        // Get coefficients again - should still be unchanged (defensive copy returned)
        float[] secondGet = result.getCoefficients();
        assertEquals("getCoefficients should return defensive copy", 1.0f, secondGet[0], EPSILON);
    }
    
    // ==================== COHERENCE TESTS (Ω - Omega) ====================
    
    @Test
    public void testAnovaResult_SSDecomposition_coherence() {
        // SS_T = SS_M + SS_E identity
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        assertTrue("SS decomposition identity should hold", 
            result.verifySumOfSquares(EPSILON));
        
        // Additional verification
        assertEquals("SS_T should equal SS_M + SS_E", 
            result.ssTotal, result.ssModel + result.ssError, EPSILON);
    }
    
    @Test
    public void testAnovaResult_verifySumOfSquares_withError() {
        // Test with intentional floating-point error
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 19.99f);
        
        // Should pass with larger tolerance
        assertTrue("Should pass with 0.02 tolerance", result.verifySumOfSquares(0.02f));
        // Should fail with smaller tolerance
        assertFalse("Should fail with 0.001 tolerance", result.verifySumOfSquares(0.001f));
    }
    
    @Test
    public void testAnovaResult_RSquaredComplementary() {
        // R² + ErrorRatio should be approximately 1
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 75.0f, 25.0f);
        
        float rSquared = result.getRSquared();  // 0.75
        float errorRatio = result.ssError / result.ssTotal;  // 0.25
        
        assertEquals("R² + error ratio should equal 1", 1.0f, rSquared + errorRatio, EPSILON);
    }
    
    // ==================== EQUALITY AND HASHING TESTS ====================
    
    @Test
    public void testAnovaResult_equals_identical() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 2.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertEquals("Identical results should be equal", result1, result2);
    }
    
    @Test
    public void testAnovaResult_equals_differentCoefficients() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 3.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertNotEquals("Different coefficients should not be equal", result1, result2);
    }
    
    @Test
    public void testAnovaResult_equals_differentSS() {
        float[] coefs = {1.0f, 2.0f};
        
        AnovaResult result1 = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs, 100.0f, 70.0f, 30.0f);
        
        assertNotEquals("Different SS values should not be equal", result1, result2);
    }
    
    @Test
    public void testAnovaResult_equals_differentLength() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 2.0f, 3.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertNotEquals("Different coefficient lengths should not be equal", result1, result2);
    }
    
    @Test
    public void testAnovaResult_hashCode_consistency() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 2.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertEquals("Equal objects should have equal hashCodes", 
            result1.hashCode(), result2.hashCode());
    }
    
    // ==================== STRING REPRESENTATION TESTS ====================
    
    @Test
    public void testAnovaResult_toString() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        String str = result.toString();
        
        assertNotNull("toString should not return null", str);
        assertTrue("Should contain AnovaResult", str.contains("AnovaResult"));
        assertTrue("Should contain SS_T", str.contains("SS_T"));
        assertTrue("Should contain SS_M", str.contains("SS_M"));
        assertTrue("Should contain SS_E", str.contains("SS_E"));
        assertTrue("Should contain R²", str.contains("R²"));
    }
    
    // ==================== DETERMINISM TESTS ====================
    
    @Test
    public void testAnovaResult_determinism() {
        float[] coefs = {1.0f, 2.0f};
        
        for (int i = 0; i < 100; i++) {
            AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
            
            assertEquals("R² should be deterministic", 0.8f, result.getRSquared(), STRICT_EPSILON);
            assertEquals("Adjusted R² should be deterministic", 
                result.getAdjustedRSquared(10), result.getAdjustedRSquared(10), STRICT_EPSILON);
            assertEquals("F should be deterministic", 
                32.0f, result.getFStatistic(10), STRICT_EPSILON);
        }
    }
}
