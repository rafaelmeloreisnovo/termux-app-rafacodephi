package com.termux.lowlevel;

/**
 * ANOVA result with coefficients and sum of squares decomposition
 * SS_T = SS_M + SS_E
 */
public class AnovaResult {
    
    /** Model coefficients (e.g., [intercept, slope] for linear) */
    public final float[] coefficients;
    
    /** Total sum of squares: SS_T = sum_i (y_i - mean_y)^2 */
    public final float ssTotal;
    
    /** Model sum of squares: SS_M = sum_i (y_pred_i - mean_y)^2 */
    public final float ssModel;
    
    /** Error sum of squares: SS_E = sum_i (y_i - y_pred_i)^2 */
    public final float ssError;
    
    /**
     * Construct ANOVA result
     * 
     * @param coefficients Model coefficients
     * @param ssTotal Total sum of squares
     * @param ssModel Model sum of squares
     * @param ssError Error sum of squares
     */
    public AnovaResult(float[] coefficients, float ssTotal, float ssModel, float ssError) {
        this.coefficients = coefficients;
        this.ssTotal = ssTotal;
        this.ssModel = ssModel;
        this.ssError = ssError;
    }
    
    /**
     * Compute R-squared (coefficient of determination)
     * R^2 = SS_M / SS_T = 1 - SS_E / SS_T
     * 
     * @return R-squared value [0, 1]
     */
    public float getRSquared() {
        if (ssTotal < 1e-10f) return 0.0f;
        return ssModel / ssTotal;
    }
    
    /**
     * Compute adjusted R-squared
     * R^2_adj = 1 - (SS_E / (n - p)) / (SS_T / (n - 1))
     * 
     * @param n Number of observations
     * @return Adjusted R-squared
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
     * Compute F-statistic for model significance
     * F = (SS_M / (p - 1)) / (SS_E / (n - p))
     * 
     * @param n Number of observations
     * @return F-statistic
     */
    public float getFStatistic(int n) {
        if (n <= coefficients.length) return 0.0f;
        int p = coefficients.length;
        float ms_model = ssModel / (p - 1);
        float ms_error = ssError / (n - p);
        if (ms_error < 1e-10f) return Float.POSITIVE_INFINITY;
        return ms_model / ms_error;
    }
    
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("AnovaResult{\n");
        sb.append("  coefficients: [");
        for (int i = 0; i < coefficients.length; i++) {
            if (i > 0) sb.append(", ");
            sb.append(coefficients[i]);
        }
        sb.append("]\n");
        sb.append("  SS_T: ").append(ssTotal).append("\n");
        sb.append("  SS_M: ").append(ssModel).append("\n");
        sb.append("  SS_E: ").append(ssError).append("\n");
        sb.append("  R^2: ").append(getRSquared()).append("\n");
        sb.append("}");
        return sb.toString();
    }
}
