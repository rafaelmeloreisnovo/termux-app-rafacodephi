package com.termux.rafaelia;

/**
 * ANOVA result with coefficients and sum of squares decomposition.
 * SS_T = SS_M + SS_E
 * 
 * This is an immutable data class for thread-safety and reliability.
 */
public final class AnovaResult {
    
    /** Model coefficients (e.g., [intercept, slope] for linear) */
    public final float[] coefficients;
    
    /** Total sum of squares: SS_T = sum_i (y_i - mean_y)^2 */
    public final float ssTotal;
    
    /** Model sum of squares: SS_M = sum_i (y_pred_i - mean_y)^2 */
    public final float ssModel;
    
    /** Error sum of squares: SS_E = sum_i (y_i - y_pred_i)^2 */
    public final float ssError;
    
    /**
     * Construct ANOVA result.
     * 
     * @param coefficients Model coefficients (must not be null)
     * @param ssTotal Total sum of squares (must be >= 0)
     * @param ssModel Model sum of squares (must be >= 0)
     * @param ssError Error sum of squares (must be >= 0)
     * @throws IllegalArgumentException if parameters are invalid
     */
    public AnovaResult(float[] coefficients, float ssTotal, float ssModel, float ssError) {
        if (coefficients == null || coefficients.length == 0) {
            throw new IllegalArgumentException("Coefficients must not be null or empty");
        }
        if (ssTotal < 0 || ssModel < 0 || ssError < 0) {
            throw new IllegalArgumentException("Sum of squares values must be non-negative");
        }
        
        // Defensive copy to ensure immutability
        this.coefficients = coefficients.clone();
        this.ssTotal = ssTotal;
        this.ssModel = ssModel;
        this.ssError = ssError;
    }
    
    /**
     * Get a copy of the coefficients array.
     * 
     * @return Defensive copy of coefficients
     */
    public float[] getCoefficients() {
        return coefficients.clone();
    }
    
    /**
     * Compute R-squared (coefficient of determination).
     * R^2 = SS_M / SS_T = 1 - SS_E / SS_T
     * 
     * @return R-squared value [0, 1]
     */
    public float getRSquared() {
        if (ssTotal < 1e-10f) return 0.0f;
        return ssModel / ssTotal;
    }
    
    /**
     * Compute adjusted R-squared.
     * R^2_adj = 1 - (SS_E / (n - p)) / (SS_T / (n - 1))
     * 
     * @param n Number of observations (must be > number of coefficients)
     * @return Adjusted R-squared, or 0 if n is too small
     */
    public float getAdjustedRSquared(int n) {
        if (n <= coefficients.length) return 0.0f;
        
        int p = coefficients.length;
        float numerator = ssError / (n - p);
        float denominator = ssTotal / (n - 1);
        
        if (denominator < 1e-10f) return 0.0f;
        
        return 1.0f - numerator / denominator;
    }
    
    /**
     * Compute F-statistic for model significance.
     * F = (SS_M / (p - 1)) / (SS_E / (n - p))
     * 
     * @param n Number of observations (must be > number of coefficients)
     * @return F-statistic, or 0 if n is too small, or POSITIVE_INFINITY if error is near zero
     */
    public float getFStatistic(int n) {
        if (n <= coefficients.length || coefficients.length < 2) return 0.0f;
        
        int p = coefficients.length;
        float msModel = ssModel / (p - 1);
        float msError = ssError / (n - p);
        
        if (msError < 1e-10f) {
            return Float.POSITIVE_INFINITY;
        }
        
        return msModel / msError;
    }
    
    /**
     * Verify SS decomposition identity: SS_T ≈ SS_M + SS_E
     * 
     * @param tolerance Tolerance for floating-point comparison
     * @return true if identity holds within tolerance
     */
    public boolean verifySumOfSquares(float tolerance) {
        float sum = ssModel + ssError;
        return Math.abs(ssTotal - sum) <= tolerance;
    }
    
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("AnovaResult{\n");
        sb.append("  coefficients: [");
        for (int i = 0; i < coefficients.length; i++) {
            if (i > 0) sb.append(", ");
            sb.append(String.format("%.6f", coefficients[i]));
        }
        sb.append("]\n");
        sb.append(String.format("  SS_T: %.6f\n", ssTotal));
        sb.append(String.format("  SS_M: %.6f\n", ssModel));
        sb.append(String.format("  SS_E: %.6f\n", ssError));
        sb.append(String.format("  R²: %.6f\n", getRSquared()));
        sb.append("}");
        return sb.toString();
    }
    
    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (!(obj instanceof AnovaResult)) return false;
        
        AnovaResult other = (AnovaResult) obj;
        
        if (coefficients.length != other.coefficients.length) return false;
        
        for (int i = 0; i < coefficients.length; i++) {
            if (Math.abs(coefficients[i] - other.coefficients[i]) > 1e-6f) {
                return false;
            }
        }
        
        return Math.abs(ssTotal - other.ssTotal) < 1e-6f &&
               Math.abs(ssModel - other.ssModel) < 1e-6f &&
               Math.abs(ssError - other.ssError) < 1e-6f;
    }
    
    @Override
    public int hashCode() {
        int result = 17;
        result = 31 * result + Float.floatToIntBits(ssTotal);
        result = 31 * result + Float.floatToIntBits(ssModel);
        result = 31 * result + Float.floatToIntBits(ssError);
        for (float coef : coefficients) {
            result = 31 * result + Float.floatToIntBits(coef);
        }
        return result;
    }
}
