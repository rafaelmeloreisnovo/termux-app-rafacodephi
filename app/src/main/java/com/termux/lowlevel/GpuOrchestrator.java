package com.termux.lowlevel;

/**
 * Entry-only Java facade for the RAFAELIA native GPU/CPU orchestrator.
 *
 * Policy and routing remain native. Java exposes capability, evidence and
 * telemetry to the app/benchmark layer; it does not choose a backend.
 */
public final class GpuOrchestrator {
    private GpuOrchestrator() {}

    public static final int BACKEND_NONE = 0;
    public static final int BACKEND_OPENCL = 1;
    public static final int BACKEND_VULKAN = 2;

    public static final int CAP_OPENCL = 1 << 8;
    public static final int CAP_VULKAN = 1 << 9;
    public static final int CAP_GPU_QUALIFIED = 1 << 10;

    public static final int OK = 0;
    public static final int ERR_NOT_QUALIFIED = -7;

    private static native int nativeRefreshCapabilities();
    private static native long[] nativeSnapshot();
    private static native void nativeSetThermal(int thermal0To100);
    private static native int nativeRecordTotalCost(
            int backend,
            long cpuTotalNs,
            long gpuTotalNs,
            int sampleCount,
            int workBytes,
            boolean correctnessPass,
            boolean stableEnvironment);

    public static int refreshCapabilities() {
        if (!BareMetal.isLoaded()) return 0;
        return nativeRefreshCapabilities();
    }

    public static void setThermalState(int thermal0To100) {
        if (!BareMetal.isLoaded()) return;
        nativeSetThermal(Math.max(0, Math.min(100, thermal0To100)));
    }

    /**
     * Records a repeated same-workload end-to-end measurement.
     * cpuTotalNs and gpuTotalNs must include the complete comparable route;
     * for GPU this includes transfer/dispatch/synchronization costs.
     */
    public static int recordTotalCostEvidence(
            int backend,
            long cpuTotalNs,
            long gpuTotalNs,
            int sampleCount,
            int workBytes,
            boolean correctnessPass,
            boolean stableEnvironment) {
        if (!BareMetal.isLoaded()) return -4;
        return nativeRecordTotalCost(
                backend, cpuTotalNs, gpuTotalNs, sampleCount, workBytes,
                correctnessPass, stableEnvironment);
    }

    public static Snapshot snapshot() {
        if (!BareMetal.isLoaded()) return Snapshot.unavailable();
        long[] r = nativeSnapshot();
        if (r == null || r.length < 26 || r[0] != 1) return Snapshot.unavailable();
        return new Snapshot(r);
    }

    public static final class Snapshot {
        public final boolean available;
        public final int state;
        public final int runtimeCaps;
        public final int qualifiedCaps;
        public final int qualifiedBackend;
        public final int cpuCoresOnline;
        public final int thermal0To100;
        public final int minSamples;
        public final int minWorkBytes;
        public final int maxThermal;
        public final int minGainPermille;
        public final long cpuRouteCount;
        public final long gpuRouteCount;
        public final int lastRouteReason;
        public final Measurement opencl;
        public final Measurement vulkan;

        private Snapshot(long[] r) {
            available = true;
            state = (int)r[1];
            runtimeCaps = (int)r[2];
            qualifiedCaps = (int)r[3];
            qualifiedBackend = (int)r[4];
            cpuCoresOnline = (int)r[5];
            thermal0To100 = (int)r[6];
            minSamples = (int)r[7];
            minWorkBytes = (int)r[8];
            maxThermal = (int)r[9];
            minGainPermille = (int)r[10];
            cpuRouteCount = r[11];
            gpuRouteCount = r[12];
            lastRouteReason = (int)r[13];
            opencl = new Measurement(r[14], r[15], (int)r[16], (int)r[17], r[18] != 0, r[19] != 0);
            vulkan = new Measurement(r[20], r[21], (int)r[22], (int)r[23], r[24] != 0, r[25] != 0);
        }

        private Snapshot() {
            available = false;
            state = 0;
            runtimeCaps = 0;
            qualifiedCaps = 0;
            qualifiedBackend = BACKEND_NONE;
            cpuCoresOnline = 0;
            thermal0To100 = 0;
            minSamples = 0;
            minWorkBytes = 0;
            maxThermal = 0;
            minGainPermille = 0;
            cpuRouteCount = 0;
            gpuRouteCount = 0;
            lastRouteReason = 0;
            opencl = Measurement.empty();
            vulkan = Measurement.empty();
        }

        private static Snapshot unavailable() {
            return new Snapshot();
        }

        public boolean hasOpenClCapability() {
            return (runtimeCaps & CAP_OPENCL) != 0;
        }

        public boolean hasVulkanCapability() {
            return (runtimeCaps & CAP_VULKAN) != 0;
        }

        public boolean gpuQualified() {
            return (qualifiedCaps & CAP_GPU_QUALIFIED) != 0;
        }
    }

    public static final class Measurement {
        public final long cpuTotalNs;
        public final long gpuTotalNs;
        public final int sampleCount;
        public final int workBytes;
        public final boolean correctnessPass;
        public final boolean stableEnvironment;

        private Measurement(long cpuTotalNs, long gpuTotalNs,
                            int sampleCount, int workBytes,
                            boolean correctnessPass, boolean stableEnvironment) {
            this.cpuTotalNs = cpuTotalNs;
            this.gpuTotalNs = gpuTotalNs;
            this.sampleCount = sampleCount;
            this.workBytes = workBytes;
            this.correctnessPass = correctnessPass;
            this.stableEnvironment = stableEnvironment;
        }

        private static Measurement empty() {
            return new Measurement(0L, 0L, 0, 0, false, false);
        }
    }
}
