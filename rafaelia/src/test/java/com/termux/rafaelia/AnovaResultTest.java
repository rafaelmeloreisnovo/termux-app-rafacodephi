package com.termux.rafaelia;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Unit tests for ANOVA operations and AnovaResult class.
 */
public class AnovaResultTest {
    
    private static final float EPSILON = 1e-5f;
    
    @Test
    public void testAnovaResult_construction() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        assertNotNull(result);
        assertEquals(2, result.coefficients.length);
        assertEquals(100.0f, result.ssTotal, EPSILON);
        assertEquals(80.0f, result.ssModel, EPSILON);
        assertEquals(20.0f, result.ssError, EPSILON);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_nullCoefficients() {
        new AnovaResult(null, 100.0f, 80.0f, 20.0f);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_emptyCoefficients() {
        new AnovaResult(new float[0], 100.0f, 80.0f, 20.0f);
    }
    
    @Test(expected = IllegalArgumentException.class)
    public void testAnovaResult_negativeSS() {
        float[] coefs = {1.0f, 2.0f};
        new AnovaResult(coefs, -100.0f, 80.0f, 20.0f);
    }
    
    @Test
    public void testGetRSquared() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // R² = SS_M / SS_T = 80 / 100 = 0.8
        assertEquals(0.8f, result.getRSquared(), EPSILON);
    }
    
    @Test
    public void testGetRSquared_perfectFit() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 100.0f, 0.0f);
        
        assertEquals(1.0f, result.getRSquared(), EPSILON);
    }
    
    @Test
    public void testGetRSquared_noFit() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 0.0f, 100.0f);
        
        assertEquals(0.0f, result.getRSquared(), EPSILON);
    }
    
    @Test
    public void testGetAdjustedRSquared() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        int n = 10;
        float adjustedR2 = result.getAdjustedRSquared(n);
        
        // Adjusted R² should be less than or equal to R²
        assertTrue(adjustedR2 <= result.getRSquared());
    }
    
    @Test
    public void testGetAdjustedRSquared_insufficientData() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // n <= p should return 0
        assertEquals(0.0f, result.getAdjustedRSquared(2), EPSILON);
        assertEquals(0.0f, result.getAdjustedRSquared(1), EPSILON);
    }
    
    @Test
    public void testGetFStatistic() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        int n = 10;
        float fStat = result.getFStatistic(n);
        
        // F = (SS_M / (p - 1)) / (SS_E / (n - p))
        // F = (80 / 1) / (20 / 8) = 80 / 2.5 = 32
        assertEquals(32.0f, fStat, EPSILON);
    }
    
    @Test
    public void testGetFStatistic_perfectFit() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 100.0f, 0.0f);
        
        int n = 10;
        float fStat = result.getFStatistic(n);
        
        // With zero error, F should be infinite
        assertEquals(Float.POSITIVE_INFINITY, fStat, 0);
    }
    
    @Test
    public void testGetFStatistic_insufficientData() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // n <= p should return 0
        assertEquals(0.0f, result.getFStatistic(2), EPSILON);
    }
    
    @Test
    public void testVerifySumOfSquares() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // SS_T = SS_M + SS_E = 80 + 20 = 100
        assertTrue(result.verifySumOfSquares(0.01f));
    }
    
    @Test
    public void testVerifySumOfSquares_withError() {
        float[] coefs = {1.0f, 2.0f};
        // Intentional floating-point error
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 19.99f);
        
        assertTrue(result.verifySumOfSquares(0.02f));
        assertFalse(result.verifySumOfSquares(0.001f));
    }
    
    @Test
    public void testGetCoefficients_immutability() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // Modify returned array
        float[] returnedCoefs = result.getCoefficients();
        returnedCoefs[0] = 999.0f;
        
        // Original should be unchanged
        assertEquals(1.0f, result.coefficients[0], EPSILON);
    }
    
    @Test
    public void testGetCoefficients_constructorImmutability() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        // Modify original array
        coefs[0] = 999.0f;
        
        // Result should be unchanged
        assertEquals(1.0f, result.coefficients[0], EPSILON);
    }
    
    @Test
    public void testEquals() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 2.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertEquals(result1, result2);
    }
    
    @Test
    public void testEquals_different() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 3.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertNotEquals(result1, result2);
    }
    
    @Test
    public void testHashCode() {
        float[] coefs1 = {1.0f, 2.0f};
        float[] coefs2 = {1.0f, 2.0f};
        
        AnovaResult result1 = new AnovaResult(coefs1, 100.0f, 80.0f, 20.0f);
        AnovaResult result2 = new AnovaResult(coefs2, 100.0f, 80.0f, 20.0f);
        
        assertEquals(result1.hashCode(), result2.hashCode());
    }
    
    @Test
    public void testToString() {
        float[] coefs = {1.0f, 2.0f};
        AnovaResult result = new AnovaResult(coefs, 100.0f, 80.0f, 20.0f);
        
        String str = result.toString();
        
        assertNotNull(str);
        assertTrue(str.contains("AnovaResult"));
        assertTrue(str.contains("SS_T"));
        assertTrue(str.contains("SS_M"));
        assertTrue(str.contains("SS_E"));
        assertTrue(str.contains("R²"));
    }
}
