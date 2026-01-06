package com.termux.lowlevel;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Unit tests for low-level bare-metal operations
 * Tests VA (Vectra Analysis) and ANOVA functionality
 */
public class VectraMathTest {
    
    private static final float EPSILON = 1e-5f;
    
    @Test
    public void testCosineSimilarity_identicalVectors() {
        float[] v1 = {1.0f, 2.0f, 3.0f, 4.0f};
        float[] v2 = {1.0f, 2.0f, 3.0f, 4.0f};
        
        float similarity = VectraMath.computeCosineSimilarity(v1, v2);
        
        // Identical vectors should have cosine similarity of 1.0
        assertEquals(1.0f, similarity, EPSILON);
    }
    
    @Test
    public void testCosineSimilarity_orthogonalVectors() {
        float[] v1 = {1.0f, 0.0f, 0.0f};
        float[] v2 = {0.0f, 1.0f, 0.0f};
        
        float similarity = VectraMath.computeCosineSimilarity(v1, v2);
        
        // Orthogonal vectors should have cosine similarity of 0.0
        assertEquals(0.0f, similarity, EPSILON);
    }
    
    @Test
    public void testCosineSimilarity_oppositeVectors() {
        float[] v1 = {1.0f, 2.0f, 3.0f};
        float[] v2 = {-1.0f, -2.0f, -3.0f};
        
        float similarity = VectraMath.computeCosineSimilarity(v1, v2);
        
        // Opposite vectors should have cosine similarity of -1.0
        assertEquals(-1.0f, similarity, EPSILON);
    }
    
    @Test
    public void testEuclideanDistance_samePoint() {
        float[] v1 = {1.0f, 2.0f, 3.0f};
        float[] v2 = {1.0f, 2.0f, 3.0f};
        
        float distance = VectraMath.computeEuclideanDistance(v1, v2);
        
        // Same point should have distance 0.0
        assertEquals(0.0f, distance, EPSILON);
    }
    
    @Test
    public void testEuclideanDistance_knownDistance() {
        float[] v1 = {0.0f, 0.0f, 0.0f};
        float[] v2 = {3.0f, 4.0f, 0.0f};
        
        float distance = VectraMath.computeEuclideanDistance(v1, v2);
        
        // 3-4-5 triangle: distance should be 5.0
        assertEquals(5.0f, distance, EPSILON);
    }
    
    @Test
    public void testReversalInvariance_symmetric() {
        float[] v = {1.0f, 2.0f, 3.0f, 2.0f, 1.0f};
        
        boolean invariant = VectraMath.testReversalInvariance(v, 0.1f);
        
        // Symmetric vector should be reversal invariant
        assertTrue(invariant);
    }
    
    @Test
    public void testReversalInvariance_asymmetric() {
        float[] v = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        
        boolean invariant = VectraMath.testReversalInvariance(v, 0.1f);
        
        // Asymmetric vector should not be reversal invariant
        assertFalse(invariant);
    }
    
    @Test
    public void testLeastSquares_linearRelationship() {
        // y = 2x + 1
        float[] x = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        float[] y = {3.0f, 5.0f, 7.0f, 9.0f, 11.0f};
        
        AnovaResult result = VectraMath.fitLeastSquares(x, y);
        
        assertNotNull(result);
        assertEquals(2, result.coefficients.length);
        
        // Check intercept (approximately 1.0)
        assertEquals(1.0f, result.coefficients[0], EPSILON);
        
        // Check slope (approximately 2.0)
        assertEquals(2.0f, result.coefficients[1], EPSILON);
        
        // Perfect fit should have R^2 = 1.0
        assertEquals(1.0f, result.getRSquared(), EPSILON);
    }
    
    @Test
    public void testSSDecomposition() {
        float[] y = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        float[] yPred = {1.1f, 2.0f, 2.9f, 4.1f, 5.0f};
        
        float[] ss = VectraMath.computeSSDecomposition(y, yPred);
        
        assertNotNull(ss);
        assertEquals(3, ss.length);
        
        float ssTotal = ss[0];
        float ssModel = ss[1];
        float ssError = ss[2];
        
        // SS_T = SS_M + SS_E (approximately)
        assertEquals(ssTotal, ssModel + ssError, 0.01f);
        
        // All values should be non-negative
        assertTrue(ssTotal >= 0);
        assertTrue(ssModel >= 0);
        assertTrue(ssError >= 0);
    }
}
