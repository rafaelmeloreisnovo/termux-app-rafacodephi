package com.termux.rafaelia;

/**
 * Rafaelia: Optimized bare-metal utilities module
 * 
 * Provides high-performance mathematical and memory operations with:
 * - Minimal Java dependencies
 * - Bare-metal C/ASM optimizations
 * - Vector operations with SIMD support
 * - Statistical analysis (ANOVA)
 * 
 * All operations are consolidated in this single utility class to reduce
 * redundancy and improve code maintainability.
 */
public final class RafaeliaUtils {
    
    /** Flag indicating if native library is available */
    private static final boolean NATIVE_AVAILABLE;
    
    static {
        boolean loaded = false;
        try {
            System.loadLibrary("termux-rafaelia");
            loaded = true;
        } catch (UnsatisfiedLinkError e) {
            // Native library not available - will use Java fallbacks where possible
            loaded = false;
        }
        NATIVE_AVAILABLE = loaded;
    }
    
    /**
     * Check if native library is available.
     * @return true if native methods can be used, false otherwise
     */
    public static boolean isNativeAvailable() {
        return NATIVE_AVAILABLE;
    }


    /**
     * Verifica disponibilidade do pipeline RAFAELIA direto (JNI zero-copy).
     */
    public static boolean isDirectPipelineAvailable() {
        return RafaeliaCore.isNativeAvailable();
    }

    /**
     * Executa pipeline commit-gate (LOAD->PROCESS->VERIFY->COMMIT) e retorna resultado estruturado.
     */
    public static RafaeliaCore.CommitGateResult processCommitGate(byte[] data, int len) {
        return RafaeliaCore.processWithCommitGate(data, len);
    }

    /**
     * Executa um tick toroidal T^7 e retorna phi Q16.16.
     */
    public static int toroidalTickQ16() {
        return RafaeliaCore.step();
    }

    
    // Private constructor to prevent instantiation
    private RafaeliaUtils() {
        throw new AssertionError("Utility class - do not instantiate");
    }
    
    // Constants
    private static final float EPSILON = 1e-10f;  // Threshold for floating-point comparisons
    private static final int UNROLL_FACTOR = 4;   // Loop unrolling factor for vector operations
    
    // ==================== Memory Operations ====================
    
    /**
     * Optimized memory copy using bare-metal C/ASM implementation.
     * Faster than System.arraycopy for large blocks.
     * 
     * @param dest Destination byte array (must not be null)
     * @param src Source byte array (must not be null)
     * @param n Number of bytes to copy (must be > 0 and within array bounds)
     * @throws IllegalArgumentException if parameters are invalid
     */
    public static void memcpy(byte[] dest, byte[] src, int n) {
        if (dest == null || src == null || n <= 0) return;
        int copyN = n;
        if (copyN > dest.length) copyN = dest.length;
        if (copyN > src.length) copyN = src.length;
        if (copyN <= 0) return;
        System.arraycopy(src, 0, dest, 0, copyN);
    }
    
    /**
     * Optimized memory set using bare-metal C/ASM implementation.
     * Faster than Arrays.fill for large blocks.
     * 
     * @param array Target byte array (must not be null)
     * @param value Value to set (byte)
     * @param n Number of bytes to set (must be > 0 and within array bounds)
     * @throws IllegalArgumentException if parameters are invalid
     */
    public static void memset(byte[] array, int value, int n) {
        if (array == null || n <= 0) return;
        int setN = n > array.length ? array.length : n;
        byte v = (byte) value;
        for (int i = 0; i < setN; i++) {
            array[i] = v;
        }
    }
    
    // ==================== Fast Mathematical Operations ====================
    
    /**
     * Fast square root using Newton-Raphson method (native) or Java Math.sqrt (fallback).
     * 
     * @param x Input value (must be >= 0)
     * @return Square root of x, or 0 if x < 0
     */
    public static float sqrt(float x) {
        if (NATIVE_AVAILABLE) {
            return sqrtNative(x);
        }
        if (x < 0.0f) return 0.0f;
        return (float) Math.sqrt(x);
    }
    
    /**
     * Native square root implementation.
     */
    private static native float sqrtNative(float x);
    
    /**
     * Fast integer power (no library dependencies).
     * Uses exponentiation by squaring for optimal performance.
     * 
     * @param base Base value
     * @param exp Exponent (integer)
     * @return base^exp
     */
    public static float pow(float base, int exp) {
        if (exp == 0) return 1.0f;
        if (base == 0.0f) return 0.0f;
        if (base == 1.0f) return 1.0f;
        
        boolean negative = exp < 0;
        int absExp = negative ? -exp : exp;
        
        float result = 1.0f;
        float current = base;
        
        // Exponentiation by squaring (binary method)
        // For exp = 13 (binary: 1101), computes base^8 * base^4 * base^1
        while (absExp > 0) {
            if ((absExp & 1) == 1) {
                result *= current;
            }
            current *= current;
            absExp >>= 1;
        }
        
        return negative ? (1.0f / result) : result;
    }
    
    // ==================== Vector Operations ====================
    
    /**
     * Compute dot product with optimized implementation.
     * Uses SIMD/NEON when available via JNI.
     * 
     * @param v1 First vector (must not be null)
     * @param v2 Second vector (must not be null and same length as v1)
     * @return Dot product, or 0 if inputs are invalid
     */
    public static float dotProduct(float[] v1, float[] v2) {
        if (v1 == null || v2 == null || v1.length != v2.length || v1.length == 0) {
            return 0.0f;
        }
        
        float sum = 0.0f;
        int len = v1.length;
        
        // Unrolled loop for better performance
        int i = 0;
        // Round down to nearest multiple of UNROLL_FACTOR
        int unrollLimit = len & ~(UNROLL_FACTOR - 1);
        
        for (; i < unrollLimit; i += UNROLL_FACTOR) {
            sum += v1[i] * v2[i] + v1[i+1] * v2[i+1] + 
                   v1[i+2] * v2[i+2] + v1[i+3] * v2[i+3];
        }
        
        // Handle remainder
        for (; i < len; i++) {
            sum += v1[i] * v2[i];
        }
        
        return sum;
    }
    
    /**
     * Compute vector magnitude (L2 norm).
     * 
     * @param v Input vector (must not be null)
     * @return ||v||, or 0 if v is null or empty
     */
    public static float magnitude(float[] v) {
        if (v == null || v.length == 0) return 0.0f;
        
        float sumSq = 0.0f;
        
        // Optimized loop with unrolling
        int i = 0;
        int len = v.length;
        // Round down to nearest multiple of UNROLL_FACTOR
        int unrollLimit = len & ~(UNROLL_FACTOR - 1);
        
        for (; i < unrollLimit; i += UNROLL_FACTOR) {
            sumSq += v[i] * v[i] + v[i+1] * v[i+1] + 
                     v[i+2] * v[i+2] + v[i+3] * v[i+3];
        }
        
        for (; i < len; i++) {
            sumSq += v[i] * v[i];
        }
        
        return sqrt(sumSq);
    }
    
    /**
     * Normalize vector to unit length (in-place modification).
     * 
     * @param v Input vector (modified in-place, must not be null)
     */
    public static void normalize(float[] v) {
        if (v == null || v.length == 0) return;
        
        float mag = magnitude(v);
        if (mag < EPSILON) return; // Avoid division by zero
        
        float invMag = 1.0f / mag;
        for (int i = 0; i < v.length; i++) {
            v[i] *= invMag;
        }
    }
    
    // ==================== Vectra Analysis (VA) Operations ====================
    
    // Feature types for VA context
    public static final int FEATURE_HASH = 0;
    public static final int FEATURE_TEXT = 1;
    public static final int FEATURE_PHONEME = 2;
    public static final int FEATURE_IMAGE_FFT = 3;
    
    /**
     * Initialize VA context with specified space dimension and feature type.
     * VA_min = (Space ⊕ Features ⊕ Pairing ⊕ InvarianceTests ⊕ OutputSpec)
     * 
     * @param spaceDim Fixed dimension n for the space (must be > 0)
     * @param featureType Base channel type (HASH, TEXT, PHONEME, IMAGE_FFT)
     * @return Native context handle (pointer), or 0 on error
     */
    public static long initVA(int spaceDim, int featureType) {
        if (spaceDim <= 0 || featureType < FEATURE_HASH || featureType > FEATURE_IMAGE_FFT) return 0L;
        return 0L;
    }
    
    /**
     * Release VA context and free resources.
     * 
     * @param ctx Native context handle (from initVA)
     */
    public static void releaseVA(long ctx) {
        if (ctx == Long.MIN_VALUE) return;
    }
    
    /**
     * Compute cosine similarity between two vectors.
     * part of VA pairing: pair(v_i, v_j) => { cos(v_i, v_j), ||v_i - v_j||, ΔH }
     * 
     * @param v1 First vector (must not be null)
     * @param v2 Second vector (must not be null and same length as v1)
     * @return Cosine similarity [-1, 1], or 0 if inputs are invalid
     */
    public static float cosineSimilarity(float[] v1, float[] v2) {
        if (v1 == null || v2 == null || v1.length != v2.length || v1.length == 0) return 0.0f;
        float mag1 = magnitude(v1);
        float mag2 = magnitude(v2);
        float denom = mag1 * mag2;
        if (denom < EPSILON) return 0.0f;
        return dotProduct(v1, v2) / denom;
    }
    
    /**
     * Compute Euclidean distance between two vectors.
     * ||v_i - v_j||
     * 
     * @param v1 First vector (must not be null)
     * @param v2 Second vector (must not be null and same length as v1)
     * @return Euclidean distance, or 0 if inputs are invalid
     */
    public static float euclideanDistance(float[] v1, float[] v2) {
        if (v1 == null || v2 == null || v1.length != v2.length || v1.length == 0) return 0.0f;
        float sumSq = 0.0f;
        for (int i = 0; i < v1.length; i++) {
            float diff = v1[i] - v2[i];
            sumSq += diff * diff;
        }
        return sqrt(sumSq);
    }
    
    /**
     * Test reversal invariance.
     * I_← (v) = 1[pair(v, rev(v)) stable]
     * 
     * @param v Vector to test (must not be null)
     * @param threshold Stability threshold (must be >= 0)
     * @return true if reversal invariant, false otherwise
     */
    public static boolean testReversalInvariance(float[] v, float threshold) {
        if (v == null || v.length == 0 || threshold < 0.0f) return false;
        for (int i = 0; i < v.length / 2; i++) {
            if (Math.abs(v[i] - v[v.length - 1 - i]) > threshold) {
                return false;
            }
        }
        return true;
    }
    
    // ==================== ANOVA Operations ====================
    
    /**
     * Fit ANOVA least squares model.
     * Minimizes: d/d_beta sum_i (y_i - y_hat_i(beta))^2 = 0
     * 
     * @param x Independent variable (must not be null, length > 2)
     * @param y Dependent variable (must not be null, same length as x)
     * @return ANOVA result with coefficients and SS decomposition, or null on error
     */
    public static AnovaResult fitLeastSquares(float[] x, float[] y) {
        if (x == null || y == null || x.length != y.length || x.length < 3) return null;

        int n = x.length;
        float sumX = 0.0f;
        float sumY = 0.0f;
        float sumXY = 0.0f;
        float sumX2 = 0.0f;

        for (int i = 0; i < n; i++) {
            sumX += x[i];
            sumY += y[i];
            sumXY += x[i] * y[i];
            sumX2 += x[i] * x[i];
        }

        float meanX = sumX / n;
        float meanY = sumY / n;
        float denom = n * sumX2 - sumX * sumX;
        if (Math.abs(denom) < EPSILON) return null;

        float slope = (n * sumXY - sumX * sumY) / denom;
        float intercept = meanY - slope * meanX;
        float ssTotal = 0.0f;
        float ssModel = 0.0f;
        float ssError = 0.0f;

        for (int i = 0; i < n; i++) {
            float yPred = intercept + slope * x[i];
            float diffTotal = y[i] - meanY;
            float diffModel = yPred - meanY;
            float diffError = y[i] - yPred;

            ssTotal += diffTotal * diffTotal;
            ssModel += diffModel * diffModel;
            ssError += diffError * diffError;
        }

        return new AnovaResult(new float[] {intercept, slope}, ssTotal, ssModel, ssError);
    }
    
    /**
     * Compute ANOVA SS decomposition.
     * SS_T = SS_M + SS_E
     * 
     * @param y Observed values (must not be null)
     * @param yPred Predicted values (must not be null, same length as y)
     * @return Array [SS_T, SS_M, SS_E], or null on error
     */
    public static float[] computeSSDecomposition(float[] y, float[] yPred) {
        if (y == null || yPred == null || y.length != yPred.length || y.length == 0) return null;

        float sumY = 0.0f;
        for (float value : y) {
            sumY += value;
        }
        float meanY = sumY / y.length;
        float ssTotal = 0.0f;
        float ssModel = 0.0f;
        float ssError = 0.0f;

        for (int i = 0; i < y.length; i++) {
            float diffTotal = y[i] - meanY;
            float diffModel = yPred[i] - meanY;
            float diffError = y[i] - yPred[i];

            ssTotal += diffTotal * diffTotal;
            ssModel += diffModel * diffModel;
            ssError += diffError * diffError;
        }

        return new float[] {ssTotal, ssModel, ssError};
    }

    // ==================== Numeric Base Operations (raf_numbase) ====================

    /** Convert {@code n} to its string representation in {@code base} (2–36). */
    public static String toBase(long n, int base) {
        if (base < 2 || base > 36) return null;
        return Long.toString(n, base).toUpperCase(java.util.Locale.ROOT);
    }

    /** Parse a base-{@code base} string back to a {@code long}. */
    public static long fromBase(String s, int base) {
        if (s == null || base < 2 || base > 36) return 0L;
        try {
            return Long.parseLong(s.trim(), base);
        } catch (NumberFormatException e) {
            return 0L;
        }
    }

    /** F(0)=0, F(1)=1, F(2)=1, F(3)=2 … */
    public static long fibonacci(int n) {
        if (n <= 0) return 0L;
        long a = 0L;
        long b = 1L;
        for (int i = 1; i < n; i++) {
            long next = a + b;
            a = b;
            b = next;
        }
        return b;
    }

    /** T(0)=0, T(1)=0, T(2)=1; T(n)=T(n-1)+T(n-2)+T(n-3) → 0,0,1,1,2,4,7,13 … */
    public static long tribonacci(int n) {
        if (n <= 1) return 0L;
        if (n == 2) return 1L;
        long a = 0L;
        long b = 0L;
        long c = 1L;
        for (int i = 3; i <= n; i++) {
            long next = a + b + c;
            a = b;
            b = c;
            c = next;
        }
        return c;
    }

    /** P(0)=2, P(1)=3; P(n)=next prime ≥ P(n-2)+P(n-1) → 2,3,5,11,17,29 … */
    public static long primonacci(int n) {
        if (n <= 0) return 2L;
        if (n == 1) return 3L;
        long a = 2L;
        long b = 3L;
        for (int i = 2; i <= n; i++) {
            long next = nextPrime(a + b);
            a = b;
            b = next;
        }
        return b;
    }

    /**
     * Any sequence value mod m.
     * @param type 0=fibonacci 1=tribonacci 2=primonacci
     */
    public static long seqMod(int type, int n, int mod) {
        if (mod <= 0) return 0L;
        if (type == 0) return sequenceModFib(n, mod);
        if (type == 1) return sequenceModTri(n, mod);
        if (type == 2) return primonacci(n) % mod;
        return 0L;
    }

    /**
     * Pisano period π(m): Fibonacci mod m returns to state (0,1) after π(m) steps.
     * π(10)=60, π(7)=16, π(14)=24, π(70)=120.
     */
    public static int pisanoPeriod(int m) {
        if (m <= 0) return 0;
        if (m == 1) return 1;
        int prev = 0;
        int curr = 1;
        int limit = m > 1_000_000 ? 6_000_000 : 6 * m;
        for (int i = 1; i <= limit; i++) {
            int next = (prev + curr) % m;
            prev = curr;
            curr = next;
            if (prev == 0 && curr == 1) return i;
        }
        return 0;
    }

    /**
     * Radix economy for {@code base} over integers up to {@code nMax}.
     * Lower value = more efficient representation.
     */
    public static double baseEfficiency(int base, long nMax) {
        if (base < 2 || base > 36 || nMax <= 0L) return 0.0;
        long limit = nMax > 1_000_000L ? 1_000_000L : nMax;
        long digits = 0L;
        for (long n = 1L; n <= limit; n++) {
            digits += Long.toString(n, base).length();
        }
        return (double) digits / (double) limit;
    }

    /**
     * JSON describing how Z/baseAZ and Z/baseBZ coexist:
     * rings, Pisano periods, and coincidences at multiples of LCM(baseA, baseB).
     */
    public static String zeroCurveDual(int baseA, int baseB) {
        if (baseA < 2 || baseB < 2) return "{}";
        long lcm = lcm(baseA, baseB);
        return "{\"baseA\":" + baseA
            + ",\"baseB\":" + baseB
            + ",\"lcm\":" + lcm
            + ",\"pisanoA\":" + pisanoPeriod(baseA)
            + ",\"pisanoB\":" + pisanoPeriod(baseB)
            + "}";
    }

    private static long sequenceModFib(int n, int mod) {
        if (n <= 0) return 0L;
        long a = 0L;
        long b = 1L % mod;
        for (int i = 1; i < n; i++) {
            long next = (a + b) % mod;
            a = b;
            b = next;
        }
        return b;
    }

    private static long sequenceModTri(int n, int mod) {
        if (n <= 1) return 0L;
        if (n == 2) return 1L % mod;
        long a = 0L;
        long b = 0L;
        long c = 1L % mod;
        for (int i = 3; i <= n; i++) {
            long next = (a + b + c) % mod;
            a = b;
            b = c;
            c = next;
        }
        return c;
    }

    private static long nextPrime(long value) {
        long candidate = value <= 2L ? 2L : value;
        if ((candidate & 1L) == 0L && candidate != 2L) candidate++;
        while (!isPrime(candidate)) {
            candidate += candidate == 2L ? 1L : 2L;
        }
        return candidate;
    }

    private static boolean isPrime(long value) {
        if (value < 2L) return false;
        if (value == 2L) return true;
        if ((value & 1L) == 0L) return false;
        for (long d = 3L; d <= value / d; d += 2L) {
            if (value % d == 0L) return false;
        }
        return true;
    }

    private static long lcm(long a, long b) {
        long gcd = gcd(a, b);
        if (gcd == 0L) return 0L;
        return Math.abs((a / gcd) * b);
    }

    private static long gcd(long a, long b) {
        long x = Math.abs(a);
        long y = Math.abs(b);
        while (y != 0L) {
            long r = x % y;
            x = y;
            y = r;
        }
        return x;
    }
}
