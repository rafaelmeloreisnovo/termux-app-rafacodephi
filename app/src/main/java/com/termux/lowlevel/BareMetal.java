package com.termux.lowlevel;

/**
 * Bare-metal low-level operations for Termux
 * Direct native C/ASM implementation with no external dependencies
 * Architecture-optimized for ARM, ARM64, x86, x86_64
 * 
 * Copyright (c) instituto-Rafael
 * License: GPLv3
 */
public class BareMetal {
    
    private static boolean sLoaded = false;
    
    static {
        try {
            System.loadLibrary("termux-baremetal");
            sLoaded = true;
        } catch (UnsatisfiedLinkError e) {
            android.util.Log.e("BareMetal", "Failed to load native library", e);
        }
    }
    
    /**
     * Check if native library is loaded
     */
    public static boolean isLoaded() {
        return sLoaded;
    }

    private static int safeGetCapabilities() {
        if (!sLoaded) return 0;
        try {
            return getCapabilities();
        } catch (UnsatisfiedLinkError e) {
            sLoaded = false;
            android.util.Log.e("BareMetal", "Native library became unavailable", e);
            return 0;
        }
    }
    
    /* ========================================================================
     * Architecture Detection
     * ====================================================================== */
    
    /**
     * Get CPU architecture name
     * @return Architecture string (e.g., "arm64-v8a", "armeabi-v7a", "x86_64", "x86")
     */
    public static native String getArchitecture();
    
    /**
     * Get CPU capabilities bitmask
     * Returns runtime-detected bits when available, otherwise compile-time fallback bits.
     * @return Bitmask of capabilities (NEON, AVX, SSE, etc.)
     */
    public static native int getCapabilities();

    private static native int[] getCapabilitiesDetail();

    /**
     * Native hardware profile encoded as key/value pairs.
     */
    private static native String getHardwareProfile();

    public static final int HW_ACCESS_HAS_ABI = 1 << 0;
    public static final int HW_ACCESS_HAS_HWCAP = 1 << 1;
    public static final int HW_ACCESS_HAS_HWCAP2 = 1 << 2;
    public static final int HW_ACCESS_HAS_CPUS_ONLINE = 1 << 3;
    public static final int HW_ACCESS_HAS_CPU_CLUSTER_FREQ = 1 << 4;
    public static final int HW_ACCESS_HAS_PAGE_SIZE = 1 << 5;
    public static final int HW_ACCESS_HAS_CACHE_LINE = 1 << 6;
    public static final int HW_ACCESS_NO_PHYS_REG_ACCESS = 1 << 28;
    public static final int HW_ACCESS_NO_GPIO_PIN_ACCESS = 1 << 29;
    public static final int HW_ACCESS_NO_KERNEL_MMIO_ACCESS = 1 << 30;
    
    /**
     * Check if NEON SIMD is available
     */
    public static boolean hasNeon() {
        return (safeGetCapabilities() & 0x01) != 0;
    }
    
    /**
     * Check if AVX is available
     */
    public static boolean hasAvx() {
        return (safeGetCapabilities() & 0x02) != 0;
    }
    
    public static final class CapabilitiesDetail {
        public final int effective;
        public final int runtime;
        public final int binary;
        public final boolean runtimeValid;

        private CapabilitiesDetail(int effective, int runtime, int binary, boolean runtimeValid) {
            this.effective = effective;
            this.runtime = runtime;
            this.binary = binary;
            this.runtimeValid = runtimeValid;
        }
    }

    public static CapabilitiesDetail getCapabilitiesDetailParsed() {
        if (!sLoaded) {
            int caps = 0;
            return new CapabilitiesDetail(caps, 0, caps, false);
        }

        int[] raw;
        try {
            raw = getCapabilitiesDetail();
        } catch (UnsatisfiedLinkError e) {
            sLoaded = false;
            android.util.Log.e("BareMetal", "Failed to read detailed native capabilities", e);
            int caps = 0;
            return new CapabilitiesDetail(caps, 0, caps, false);
        }

        if (raw == null || raw.length < 4) {
            int caps = safeGetCapabilities();
            return new CapabilitiesDetail(caps, 0, caps, false);
        }
        return new CapabilitiesDetail(raw[0], raw[1], raw[2], raw[3] != 0);
    }

    /* ========================================================================
     * Vector Operations - Optimized for SIMD
     * ====================================================================== */
    
    /**
     * Compute dot product of two vectors
     * Optimized with NEON/AVX SIMD
     * 
     * @param a First vector
     * @param b Second vector
     * @return Dot product a·b
     */
    public static native float vectorDot(float[] a, float[] b);
    
    /**
     * Compute Euclidean norm (magnitude) of vector
     * 
     * @param a Vector
     * @return ||a|| = sqrt(Σ aᵢ²)
     */
    public static native float vectorNorm(float[] a);
    
    /**
     * Add two vectors element-wise
     * 
     * @param a First vector
     * @param b Second vector
     * @param result Output vector (must be pre-allocated)
     */
    public static native void vectorAdd(float[] a, float[] b, float[] result);
    
    /**
     * Subtract two vectors element-wise
     * 
     * @param a First vector
     * @param b Second vector
     * @param result Output vector (must be pre-allocated)
     */
    public static void vectorSub(float[] a, float[] b, float[] result) {
        if (a.length != b.length || a.length != result.length) {
            throw new IllegalArgumentException("Vector dimensions must match");
        }
        for (int i = 0; i < a.length; i++) {
            result[i] = a[i] - b[i];
        }
    }
    
    /**
     * Compute cosine similarity between two vectors
     * cos(θ) = (a·b) / (||a|| * ||b||)
     * 
     * @param a First vector
     * @param b Second vector
     * @return Cosine similarity [-1, 1]
     */
    public static float cosineSimilarity(float[] a, float[] b) {
        float dot = vectorDot(a, b);
        float normA = vectorNorm(a);
        float normB = vectorNorm(b);
        if (normA == 0 || normB == 0) return 0.0f;
        return dot / (normA * normB);
    }
    
    /* ========================================================================
     * Matrix Operations - Deterministic Mathematics
     * ====================================================================== */
    
    /**
     * Create matrix with given dimensions
     * @param rows Number of rows
     * @param cols Number of columns
     * @throws IllegalArgumentException if {@code rows <= 0} or {@code cols <= 0}
     * @throws OutOfMemoryError if native allocation fails for a valid dimension
     * @return Matrix handle (native pointer)
     */
    public static native long matrixCreate(int rows, int cols);
    
    /**
     * Free matrix memory.
     * After this call, the handle is invalid and must not be used in any other matrix API call.
     * @param handle Matrix handle
     */
    public static native void matrixFree(long handle);
    
    /**
     * Multiply two matrices: C = A × B
     * @param handleA First matrix
     * @param handleB Second matrix
     * @param handleResult Result matrix
     */
    public static native void matrixMultiply(long handleA, long handleB, long handleResult);
    
    /**
     * Flip matrix horizontally (mirror left-right)
     * Used for solving with deterministic mathematics
     * 
     * @param handle Matrix handle
     */
    public static native void matrixFlipHorizontal(long handle);
    
    /**
     * Flip matrix vertically (mirror top-bottom)
     * @param handle Matrix handle
     */
    public static native void matrixFlipVertical(long handle);
    
    /**
     * Flip matrix diagonally (transpose)
     * @param handle Matrix handle
     */
    public static native void matrixFlipDiagonal(long handle);
    
    /**
     * Compute matrix determinant
     * @param handle Matrix handle
     * @return Determinant value
     */
    public static native float matrixDeterminant(long handle);
    
    /**
     * Invert matrix using Gauss-Jordan elimination
     * RAFAELIA deterministic method
     * 
     * @param handle Input matrix handle
     * @param handleResult Result matrix handle (must be same size)
     * @return 0 on success, -1 on error (singular matrix)
     */
    public static native int matrixInvert(long handle, long handleResult);
    
    /**
     * Add two matrices element-wise: C = A + B
     * @param handleA First matrix
     * @param handleB Second matrix
     * @param handleResult Result matrix
     */
    public static native void matrixAdd(long handleA, long handleB, long handleResult);
    
    /**
     * Subtract two matrices element-wise: C = A - B
     * @param handleA First matrix
     * @param handleB Second matrix
     * @param handleResult Result matrix
     */
    public static native void matrixSubtract(long handleA, long handleB, long handleResult);
    
    /**
     * Scale matrix by scalar: M = s * M
     * @param handle Matrix handle
     * @param scalar Scaling factor
     */
    public static native void matrixScale(long handle, float scalar);
    
    /**
     * Compute trace (sum of diagonal elements)
     * @param handle Matrix handle
     * @return Trace value
     */
    public static native float matrixTrace(long handle);
    
    /**
     * Set matrix to identity matrix
     * @param handle Matrix handle (must be square)
     */
    public static native void matrixIdentity(long handle);
    
    /**
     * Solve linear system Ax = b using Gaussian elimination
     * RAFAELIA deterministic method with partial pivoting
     * 
     * @param handle Matrix A handle
     * @param b Right-hand side vector
     * @param x Solution vector (output)
     * @return 0 on success, -1 on error (singular matrix)
     */
    public static native int matrixSolveLinear(long handle, float[] b, float[] x);
    
    /**
     * Transpose matrix: B = Aᵀ
     * @param handle Input matrix handle
     * @param handleResult Result matrix handle (must be transposed dimensions)
     */
    public static native void matrixTranspose(long handle, long handleResult);
    
    /**
     * Get matrix data as flat array (row-major order)
     * @param handle Matrix handle
     * @param data Output array (must be at least rows*cols)
     */
    public static native void matrixGetData(long handle, float[] data);
    
    /**
     * Set matrix data from flat array (row-major order)
     * @param handle Matrix handle
     * @param data Input array (must be at least rows*cols)
     */
    public static native void matrixSetData(long handle, float[] data);
    
    /* ========================================================================
     * Fast Math - No legacy functions, bare-metal implementations
     * ====================================================================== */
    
    /**
     * Fast square root using bare-metal algorithm
     * @param x Input value
     * @return √x
     */
    public static native float fastSqrt(float x);
    
    /**
     * Fast reciprocal square root (1/√x)
     * Uses Quake III algorithm
     * 
     * @param x Input value
     * @return 1/√x
     */
    public static native float fastRsqrt(float x);
    
    /**
     * Fast exponential (eˣ)
     * @param x Input value
     * @return eˣ
     */
    public static native float fastExp(float x);
    
    /**
     * Fast natural logarithm (ln x)
     * @param x Input value
     * @return ln(x)
     */
    public static native float fastLog(float x);
    
    /**
     * Fast power of 2
     * @param x Input value
     * @return x²
     */
    public static float fastPow2(float x) {
        return x * x;
    }
    
    /* ========================================================================
     * Memory Operations - Bare-metal, no libc dependencies
     * ====================================================================== */
    
    /**
     * Fast memory copy using SIMD where available
     * @param dst Destination array
     * @param src Source array
     */
    public static native void memCopy(byte[] dst, byte[] src);
    
    /**
     * Fast memory set using SIMD where available
     * @param array Array to fill
     * @param value Byte value to fill with
     */
    public static native void memSet(byte[] array, int value);
    
    /* ========================================================================
     * Utility Methods
     * ====================================================================== */
    
    /**
     * Get library information
     */
    public static String getInfo() {
        if (!isLoaded()) {
            return "BareMetal library not loaded";
        }
        
        StringBuilder sb = new StringBuilder();
        sb.append("TermuxBareMetal Library\n");
        sb.append("Architecture: ").append(getArchitecture()).append("\n");
        sb.append("Capabilities: 0x").append(Integer.toHexString(getCapabilities())).append("\n");
        sb.append("NEON: ").append(hasNeon()).append("\n");
        sb.append("AVX: ").append(hasAvx()).append("\n");
        HardwareProfile hw = readHardwareProfile();
        sb.append("HW Profile ABI: ").append(hw.abi).append("\n");
        sb.append("HW Profile Flags: 0x").append(Integer.toHexString(hw.accessFlags)).append("\n");
        sb.append("HW Profile Clusters: ").append(hw.cpuClusters).append("\n");
        sb.append("Implementation: C + Assembly (bare-metal)\n");
        sb.append("Dependencies: None (libc only)\n");
        
        return sb.toString();
    }
    

    public static HardwareProfile readHardwareProfile() {
        if (!isLoaded()) {
            return new HardwareProfile("unknown", 0L, 0L, 0, 0, 0, 0, "n/a");
        }
        return HardwareProfile.fromNative(getHardwareProfile());
    }

    public static final class HardwareProfile {
        public final String abi;
        public final long hwcap;
        public final long hwcap2;
        public final int cpusOnline;
        public final int pageSize;
        public final int cacheLine;
        public final int accessFlags;
        public final String cpuClusters;

        private HardwareProfile(String abi, long hwcap, long hwcap2, int cpusOnline,
                                int pageSize, int cacheLine, int accessFlags, String cpuClusters) {
            this.abi = abi;
            this.hwcap = hwcap;
            this.hwcap2 = hwcap2;
            this.cpusOnline = cpusOnline;
            this.pageSize = pageSize;
            this.cacheLine = cacheLine;
            this.accessFlags = accessFlags;
            this.cpuClusters = cpuClusters;
        }

        private static HardwareProfile fromNative(String raw) {
            String abi = "unknown";
            long hwcap = 0L;
            long hwcap2 = 0L;
            int cpusOnline = 0;
            int pageSize = 0;
            int cacheLine = 0;
            int accessFlags = 0;
            String cpuClusters = "n/a";

            if (raw != null && !raw.isEmpty()) {
                String[] pairs = raw.split(";");
                for (String pair : pairs) {
                    int eq = pair.indexOf('=');
                    if (eq <= 0) continue;
                    String key = pair.substring(0, eq);
                    String value = pair.substring(eq + 1);
                    if ("abi".equals(key)) abi = value;
                    else if ("hwcap".equals(key)) hwcap = parseLongAuto(value);
                    else if ("hwcap2".equals(key)) hwcap2 = parseLongAuto(value);
                    else if ("cpus_online".equals(key)) cpusOnline = parseIntAuto(value);
                    else if ("page_size".equals(key)) pageSize = parseIntAuto(value);
                    else if ("cache_line".equals(key)) cacheLine = parseIntAuto(value);
                    else if ("flags".equals(key)) accessFlags = parseIntAuto(value);
                    else if ("clusters".equals(key)) cpuClusters = value;
                }
            }

            return new HardwareProfile(abi, hwcap, hwcap2, cpusOnline, pageSize, cacheLine, accessFlags, cpuClusters);
        }

        private static int parseIntAuto(String s) {
            if (s == null || s.isEmpty()) return 0;
            try {
                if (s.startsWith("0x") || s.startsWith("0X")) {
                    return (int)Long.parseLong(s.substring(2), 16);
                }
                return Integer.parseInt(s);
            } catch (NumberFormatException e) {
                return 0;
            }
        }

        private static long parseLongAuto(String s) {
            if (s == null || s.isEmpty()) return 0L;
            try {
                if (s.startsWith("0x") || s.startsWith("0X")) {
                    return Long.parseUnsignedLong(s.substring(2), 16);
                }
                return Long.parseLong(s);
            } catch (NumberFormatException e) {
                return 0L;
            }
        }
    }

    /**
     * Matrix helper class for easier usage.
     * Implements RAFAELIA deterministic matrix operations.
     * <p>
     * Once {@link #close()} is called, this object is permanently closed and can no longer be used.
     * Any matrix operation after close will throw {@link IllegalStateException}.
     */
    public static class Matrix implements AutoCloseable {
        private long handle;
        private int rows;
        private int cols;
        
        /**
         * Build a native-backed matrix.
         *
         * Contract: both dimensions must be at least 1 ({@code rows >= 1} and
         * {@code cols >= 1}).
         *
         * @throws IllegalArgumentException if any dimension is less than 1
         * @throws OutOfMemoryError if native allocation fails for a valid size
         */
        public Matrix(int rows, int cols) {
            this.rows = rows;
            this.cols = cols;
            this.handle = matrixCreate(rows, cols);
            if (this.handle == 0) {
                throw new OutOfMemoryError("Failed to create matrix");
            }
        }
        
        public int getRows() { return rows; }
        public int getCols() { return cols; }
        public long getHandle() { return handle; }

        private void ensureOpen() {
            if (handle == 0) {
                throw new IllegalStateException("Matrix is closed and cannot be used");
            }
        }
        
        /**
         * Get matrix data as flat array (row-major order)
         */
        public float[] getData() {
            ensureOpen();
            float[] data = new float[rows * cols];
            matrixGetData(handle, data);
            return data;
        }

        /**
         * Fill the provided array with matrix data (row-major order).
         * Reuses caller-provided buffer to avoid allocations on hot paths.
         */
        public void getDataInto(float[] data) {
            ensureOpen();
            if (data.length < rows * cols) {
                throw new IllegalArgumentException("Data array too small");
            }
            matrixGetData(handle, data);
        }
        
        /**
         * Set matrix data from flat array (row-major order)
         */
        public void setData(float[] data) {
            ensureOpen();
            if (data.length < rows * cols) {
                throw new IllegalArgumentException("Data array too small");
            }
            matrixSetData(handle, data);
        }
        
        /* Flip operations - RAFAELIA deterministic method */
        
        public void flipHorizontal() {
            ensureOpen();
            matrixFlipHorizontal(handle);
        }
        
        public void flipVertical() {
            ensureOpen();
            matrixFlipVertical(handle);
        }
        
        public void flipDiagonal() {
            ensureOpen();
            matrixFlipDiagonal(handle);
        }
        
        /* Basic operations */
        
        public float determinant() {
            ensureOpen();
            return matrixDeterminant(handle);
        }
        
        public float trace() {
            ensureOpen();
            return matrixTrace(handle);
        }
        
        public void scale(float scalar) {
            ensureOpen();
            matrixScale(handle, scalar);
        }
        
        public void setIdentity() {
            ensureOpen();
            matrixIdentity(handle);
        }
        
        /* Matrix algebra */

        public Matrix multiply(Matrix other) {
            ensureOpen();
            other.ensureOpen();
            if (this.cols != other.rows) {
                throw new IllegalArgumentException("Matrix dimensions incompatible for multiplication");
            }
            Matrix result = new Matrix(this.rows, other.cols);
            matrixMultiply(this.handle, other.handle, result.handle);
            return result;
        }

        /**
         * Multiply matrices into an existing output matrix to avoid allocations.
         */
        public void multiplyInto(Matrix other, Matrix output) {
            ensureOpen();
            other.ensureOpen();
            output.ensureOpen();
            if (this.cols != other.rows) {
                throw new IllegalArgumentException("Matrix dimensions incompatible for multiplication");
            }
            if (output.rows != this.rows || output.cols != other.cols) {
                throw new IllegalArgumentException("Output matrix dimensions incompatible for multiplication result");
            }
            matrixMultiply(this.handle, other.handle, output.handle);
        }

        public Matrix add(Matrix other) {
            ensureOpen();
            other.ensureOpen();
            if (this.rows != other.rows || this.cols != other.cols) {
                throw new IllegalArgumentException("Matrix dimensions must match for addition");
            }
            Matrix result = new Matrix(this.rows, this.cols);
            matrixAdd(this.handle, other.handle, result.handle);
            return result;
        }

        /**
         * Add matrices into an existing output matrix to avoid allocations.
         */
        public void addInto(Matrix other, Matrix output) {
            ensureOpen();
            other.ensureOpen();
            output.ensureOpen();
            if (this.rows != other.rows || this.cols != other.cols) {
                throw new IllegalArgumentException("Matrix dimensions must match for addition");
            }
            if (output.rows != this.rows || output.cols != this.cols) {
                throw new IllegalArgumentException("Output matrix dimensions incompatible for addition result");
            }
            matrixAdd(this.handle, other.handle, output.handle);
        }

        public Matrix subtract(Matrix other) {
            ensureOpen();
            other.ensureOpen();
            if (this.rows != other.rows || this.cols != other.cols) {
                throw new IllegalArgumentException("Matrix dimensions must match for subtraction");
            }
            Matrix result = new Matrix(this.rows, this.cols);
            matrixSubtract(this.handle, other.handle, result.handle);
            return result;
        }

        /**
         * Subtract matrices into an existing output matrix to avoid allocations.
         */
        public void subtractInto(Matrix other, Matrix output) {
            ensureOpen();
            other.ensureOpen();
            output.ensureOpen();
            if (this.rows != other.rows || this.cols != other.cols) {
                throw new IllegalArgumentException("Matrix dimensions must match for subtraction");
            }
            if (output.rows != this.rows || output.cols != this.cols) {
                throw new IllegalArgumentException("Output matrix dimensions incompatible for subtraction result");
            }
            matrixSubtract(this.handle, other.handle, output.handle);
        }

        public Matrix transpose() {
            ensureOpen();
            Matrix result = new Matrix(this.cols, this.rows);
            matrixTranspose(this.handle, result.handle);
            return result;
        }

        /**
         * Transpose into an existing output matrix to avoid allocations.
         */
        public void transposeInto(Matrix output) {
            ensureOpen();
            output.ensureOpen();
            if (output.rows != this.cols || output.cols != this.rows) {
                throw new IllegalArgumentException("Output matrix dimensions incompatible for transpose");
            }
            matrixTranspose(this.handle, output.handle);
        }
        
        /**
         * Invert matrix using Gauss-Jordan elimination
         * @return Inverted matrix, or null if singular
         */
        public Matrix invert() {
            ensureOpen();
            if (this.rows != this.cols) {
                throw new IllegalArgumentException("Only square matrices can be inverted");
            }
            Matrix result = new Matrix(this.rows, this.cols);
            int status = matrixInvert(this.handle, result.handle);
            if (status != 0) {
                result.close();
                return null;  // Singular matrix
            }
            return result;
        }

        /**
         * Invert matrix into an existing output matrix to avoid allocations.
         * @return true on success, false if singular
         */
        public boolean invertInto(Matrix output) {
            ensureOpen();
            output.ensureOpen();
            if (this.rows != this.cols) {
                throw new IllegalArgumentException("Only square matrices can be inverted");
            }
            if (output.rows != this.rows || output.cols != this.cols) {
                throw new IllegalArgumentException("Output matrix dimensions incompatible for inversion");
            }
            int status = matrixInvert(this.handle, output.handle);
            return status == 0;
        }
        
        /**
         * Solve linear system Ax = b
         * @param b Right-hand side vector
         * @return Solution vector x, or null if singular
         */
        public float[] solve(float[] b) {
            ensureOpen();
            if (b.length != this.rows) {
                throw new IllegalArgumentException("Vector b size must match matrix rows");
            }
            float[] x = new float[this.cols];
            int status = matrixSolveLinear(this.handle, b, x);
            if (status != 0) {
                return null;  // Singular matrix
            }
            return x;
        }

        /**
         * Solve linear system Ax = b into a provided solution buffer to avoid allocations.
         * @return true on success, false if singular
         */
        public boolean solveInto(float[] b, float[] x) {
            ensureOpen();
            if (b.length != this.rows) {
                throw new IllegalArgumentException("Vector b size must match matrix rows");
            }
            if (x.length < this.cols) {
                throw new IllegalArgumentException("Solution buffer too small");
            }
            int status = matrixSolveLinear(this.handle, b, x);
            return status == 0;
        }
        
        @Override
        protected void finalize() throws Throwable {
            close();
            super.finalize();
        }
        
        /**
         * Releases native resources for this matrix.
         * After close, this instance is permanently closed and cannot be used again.
         */
        @Override
        public void close() {
            if (handle != 0) {
                matrixFree(handle);
                handle = 0;
            }
        }
        
        @Override
        public String toString() {
            return String.format("Matrix[%dx%d]@0x%x", rows, cols, handle);
        }
    }
}
