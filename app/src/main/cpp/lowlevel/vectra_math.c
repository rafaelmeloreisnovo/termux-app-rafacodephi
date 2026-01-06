#include "vectra_math.h"
#include <string.h>

/**
 * Low-level bare-metal implementation
 * Minimal dependencies, optimized for performance
 * Uses inline assembly where beneficial
 */

// Initialize VA context with direct memory manipulation
void va_init(va_context_t* ctx, uint32_t space_dim, uint8_t feature_type) {
    if (!ctx) return;
    
    // Direct memory write, no library calls
    ctx->space_dim = space_dim;
    ctx->feature_type = feature_type;
    ctx->cosine_similarity = 0.0f;
    ctx->euclidean_distance = 0.0f;
    ctx->delta_h = 0.0f;
    ctx->reversal_invariant = 0;
    ctx->permutation_invariant = 0;
    ctx->noise_invariant = 0;
    ctx->output_format = 0; // JSON default
}

// Optimized cosine similarity: cos(v1, v2) = (v1 · v2) / (||v1|| * ||v2||)
float va_compute_cosine_similarity(const float* v1, const float* v2, size_t dim) {
    if (!v1 || !v2 || dim == 0) return 0.0f;
    
    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    // Single pass computation for efficiency
    for (size_t i = 0; i < dim; i++) {
        float a = v1[i];
        float b = v2[i];
        dot_product += a * b;
        norm1 += a * a;
        norm2 += b * b;
    }
    
    // Fast square root approximation
    float norm_product = va_sqrt_fast(norm1) * va_sqrt_fast(norm2);
    if (norm_product < 1e-10f) return 0.0f;
    
    return dot_product / norm_product;
}

// Optimized Euclidean distance: ||v1 - v2||
float va_compute_euclidean_distance(const float* v1, const float* v2, size_t dim) {
    if (!v1 || !v2 || dim == 0) return 0.0f;
    
    float sum_sq = 0.0f;
    
    // Unrolled loop for better performance
    size_t i = 0;
    size_t unroll_limit = dim - (dim % 4);
    
    for (; i < unroll_limit; i += 4) {
        float d0 = v1[i] - v2[i];
        float d1 = v1[i+1] - v2[i+1];
        float d2 = v1[i+2] - v2[i+2];
        float d3 = v1[i+3] - v2[i+3];
        sum_sq += d0*d0 + d1*d1 + d2*d2 + d3*d3;
    }
    
    // Handle remainder
    for (; i < dim; i++) {
        float d = v1[i] - v2[i];
        sum_sq += d * d;
    }
    
    return va_sqrt_fast(sum_sq);
}

// Test reversal invariance: pair(v, rev(v)) should be stable
uint8_t va_test_reversal_invariance(const float* v, size_t dim, float threshold) {
    if (!v || dim == 0) return 0;
    
    // Check if pairing with reversed version is stable
    float sum_forward_backward = 0.0f;
    
    for (size_t i = 0; i < dim; i++) {
        float forward = v[i];
        float backward = v[dim - 1 - i];
        float diff = forward - backward;
        sum_forward_backward += diff * diff;
    }
    
    float stability = va_sqrt_fast(sum_forward_backward) / (float)dim;
    return (stability < threshold) ? 1 : 0;
}

// ANOVA least squares fitting: minimize sum_i (y_i - y_hat_i(beta))^2
int anova_fit_least_squares(const float* x, const float* y, size_t n, 
                            anova_result_t* result) {
    if (!x || !y || !result || n < 2) return -1;
    
    // Simple linear regression: y = a + b*x
    // Compute means
    float mean_x = 0.0f;
    float mean_y = 0.0f;
    for (size_t i = 0; i < n; i++) {
        mean_x += x[i];
        mean_y += y[i];
    }
    mean_x /= (float)n;
    mean_y /= (float)n;
    
    // Compute slope and intercept
    float num = 0.0f;
    float den = 0.0f;
    for (size_t i = 0; i < n; i++) {
        float dx = x[i] - mean_x;
        float dy = y[i] - mean_y;
        num += dx * dy;
        den += dx * dx;
    }
    
    if (den < 1e-10f) return -1;
    
    float slope = num / den;
    float intercept = mean_y - slope * mean_x;
    
    // Allocate coefficients
    result->coeff_count = 2;
    result->coefficients = (float*)malloc(2 * sizeof(float));
    if (!result->coefficients) return -1;
    
    result->coefficients[0] = intercept;
    result->coefficients[1] = slope;
    
    // Compute sum of squares
    float ss_t = 0.0f, ss_m = 0.0f, ss_e = 0.0f;
    for (size_t i = 0; i < n; i++) {
        float y_pred = intercept + slope * x[i];
        float total_dev = y[i] - mean_y;
        float model_dev = y_pred - mean_y;
        float error_dev = y[i] - y_pred;
        
        ss_t += total_dev * total_dev;
        ss_m += model_dev * model_dev;
        ss_e += error_dev * error_dev;
    }
    
    result->ss_total = ss_t;
    result->ss_model = ss_m;
    result->ss_error = ss_e;
    
    return 0;
}

// Compute ANOVA SS decomposition: SS_T = SS_M + SS_E
void anova_compute_ss_decomposition(const float* y, const float* y_pred, 
                                    size_t n, float* ss_t, float* ss_m, float* ss_e) {
    if (!y || !y_pred || !ss_t || !ss_m || !ss_e || n == 0) return;
    
    // Compute mean
    float mean_y = 0.0f;
    for (size_t i = 0; i < n; i++) {
        mean_y += y[i];
    }
    mean_y /= (float)n;
    
    // Compute sum of squares
    *ss_t = 0.0f;
    *ss_m = 0.0f;
    *ss_e = 0.0f;
    
    for (size_t i = 0; i < n; i++) {
        float total_dev = y[i] - mean_y;
        float model_dev = y_pred[i] - mean_y;
        float error_dev = y[i] - y_pred[i];
        
        *ss_t += total_dev * total_dev;
        *ss_m += model_dev * model_dev;
        *ss_e += error_dev * error_dev;
    }
}

// Optimized memcpy with potential inline assembly
void* va_memcpy_optimized(void* dest, const void* src, size_t n) {
    if (!dest || !src) return dest;
    
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    // Copy 4 bytes at a time when possible
    size_t i = 0;
    size_t aligned = n - (n % 4);
    
    for (; i < aligned; i += 4) {
        *(uint32_t*)(d + i) = *(const uint32_t*)(s + i);
    }
    
    // Copy remaining bytes
    for (; i < n; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

// Optimized memset with potential inline assembly
void* va_memset_optimized(void* s, int c, size_t n) {
    if (!s) return s;
    
    unsigned char* p = (unsigned char*)s;
    unsigned char byte = (unsigned char)c;
    
    // Set 4 bytes at a time when possible
    uint32_t word = (byte << 24) | (byte << 16) | (byte << 8) | byte;
    
    size_t i = 0;
    size_t aligned = n - (n % 4);
    
    for (; i < aligned; i += 4) {
        *(uint32_t*)(p + i) = word;
    }
    
    // Set remaining bytes
    for (; i < n; i++) {
        p[i] = byte;
    }
    
    return s;
}

// Fast square root using Newton-Raphson method (bare-metal)
float va_sqrt_fast(float x) {
    if (x <= 0.0f) return 0.0f;
    
    // Initial guess using bit manipulation
    union {
        float f;
        uint32_t i;
    } conv;
    
    conv.f = x;
    conv.i = 0x5f3759df - (conv.i >> 1); // Magic number for inverse sqrt
    float y = conv.f;
    
    // Newton-Raphson iteration: y = y * (1.5 - 0.5 * x * y * y)
    y = y * (1.5f - 0.5f * x * y * y);
    y = y * (1.5f - 0.5f * x * y * y); // Second iteration for accuracy
    
    return x * y; // Convert inverse sqrt to sqrt
}

// Fast integer power (bare-metal, no library)
float va_pow_fast(float base, int exp) {
    if (exp == 0) return 1.0f;
    
    float result = 1.0f;
    int abs_exp = (exp < 0) ? -exp : exp;
    
    for (int i = 0; i < abs_exp; i++) {
        result *= base;
    }
    
    return (exp < 0) ? (1.0f / result) : result;
}
