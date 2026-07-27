package com.termux.app.api.sensor;

import java.util.Arrays;

/**
 * Dependency-free, one-sided periodogram for bounded Android sensor windows.
 *
 * <p>The implementation intentionally uses an O(N^2) DFT instead of adding an FFT
 * dependency. Requests are bounded to 16..512 samples, keeping runtime and memory
 * deterministic on ARMv7 and ARM64 devices.</p>
 */
public final class RafSpectralAnalyzer {

    public static final int MIN_SAMPLES = 16;
    public static final int MAX_SAMPLES = 512;
    public static final double DEFAULT_MAX_JITTER_RATIO = 0.15d;
    private static final double EPSILON = 1.0e-18d;

    private RafSpectralAnalyzer() {}

    public static Result analyze(double[] inputSamples,
                                 long[] timestampsNs,
                                 String windowName) {
        if (inputSamples == null || timestampsNs == null) {
            throw new IllegalArgumentException("samples and timestamps are required");
        }
        if (inputSamples.length != timestampsNs.length) {
            throw new IllegalArgumentException("samples and timestamps must have equal length");
        }
        if (inputSamples.length < MIN_SAMPLES || inputSamples.length > MAX_SAMPLES) {
            throw new IllegalArgumentException("sample count must be between 16 and 512");
        }

        final int count = inputSamples.length;
        double mean = 0.0d;
        for (double value : inputSamples) {
            if (!Double.isFinite(value)) {
                throw new IllegalArgumentException("samples must be finite");
            }
            mean += value;
        }
        mean /= count;

        long firstTimestamp = timestampsNs[0];
        long previousTimestamp = firstTimestamp;
        double sumDeltaNs = 0.0d;
        double sumDeltaSquaredNs = 0.0d;
        for (int index = 1; index < count; index++) {
            long timestamp = timestampsNs[index];
            if (timestamp <= previousTimestamp) {
                throw new IllegalArgumentException("timestamps must be strictly increasing");
            }
            double delta = (double) (timestamp - previousTimestamp);
            sumDeltaNs += delta;
            sumDeltaSquaredNs += delta * delta;
            previousTimestamp = timestamp;
        }

        double meanDeltaNs = sumDeltaNs / (count - 1);
        double varianceDeltaNs = Math.max(0.0d,
            (sumDeltaSquaredNs / (count - 1)) - (meanDeltaNs * meanDeltaNs));
        double jitterRatio = Math.sqrt(varianceDeltaNs) / Math.max(meanDeltaNs, EPSILON);
        double sampleRateHz = 1_000_000_000.0d / meanDeltaNs;
        double frequencyResolutionHz = sampleRateHz / count;

        String normalizedWindow = normalizeWindow(windowName);
        double[] centeredWindowed = new double[count];
        double windowEnergy = 0.0d;
        double rmsAccumulator = 0.0d;
        for (int index = 0; index < count; index++) {
            double centered = inputSamples[index] - mean;
            rmsAccumulator += centered * centered;
            double window = windowCoefficient(normalizedWindow, index, count);
            centeredWindowed[index] = centered * window;
            windowEnergy += window * window;
        }
        double rms = Math.sqrt(rmsAccumulator / count);

        int binCount = (count / 2) + 1;
        double[] frequenciesHz = new double[binCount];
        double[] powers = new double[binCount];
        double totalNonDcPower = 0.0d;
        double weightedFrequencyPower = 0.0d;
        int dominantIndex = 0;
        double dominantPower = 0.0d;
        double normalization = Math.max(windowEnergy * count, EPSILON);

        for (int bin = 0; bin < binCount; bin++) {
            double real = 0.0d;
            double imaginary = 0.0d;
            for (int sample = 0; sample < count; sample++) {
                double angle = (2.0d * Math.PI * bin * sample) / count;
                double value = centeredWindowed[sample];
                real += value * Math.cos(angle);
                imaginary -= value * Math.sin(angle);
            }
            double power = ((real * real) + (imaginary * imaginary)) / normalization;
            boolean isNyquist = count % 2 == 0 && bin == count / 2;
            if (bin > 0 && !isNyquist) power *= 2.0d;

            double frequency = bin * frequencyResolutionHz;
            frequenciesHz[bin] = frequency;
            powers[bin] = power;
            if (bin > 0) {
                totalNonDcPower += power;
                weightedFrequencyPower += frequency * power;
                if (power > dominantPower) {
                    dominantPower = power;
                    dominantIndex = bin;
                }
            }
        }

        boolean flatSignal = totalNonDcPower <= EPSILON;
        double dominantFrequencyHz = flatSignal ? 0.0d : frequenciesHz[dominantIndex];
        double spectralCentroidHz = flatSignal
            ? 0.0d
            : weightedFrequencyPower / totalNonDcPower;
        String qualityState;
        if (flatSignal) {
            qualityState = "TOKEN_VAZIO_FLAT_SIGNAL";
        } else if (jitterRatio > DEFAULT_MAX_JITTER_RATIO) {
            qualityState = "TOKEN_VAZIO_TIMING_JITTER";
        } else {
            qualityState = "EVIDENCIADO_COMPUTACIONAL";
        }

        return new Result(
            count,
            firstTimestamp,
            timestampsNs[count - 1],
            sampleRateHz,
            frequencyResolutionHz,
            mean,
            rms,
            jitterRatio,
            normalizedWindow,
            dominantFrequencyHz,
            dominantPower,
            spectralCentroidHz,
            totalNonDcPower,
            flatSignal,
            qualityState,
            frequenciesHz,
            powers
        );
    }

    public static String normalizeWindow(String windowName) {
        if (windowName == null || windowName.trim().isEmpty()) return "hann";
        String normalized = windowName.trim().toLowerCase();
        if ("hann".equals(normalized) || "rectangular".equals(normalized)) return normalized;
        throw new IllegalArgumentException("window must be hann or rectangular");
    }

    private static double windowCoefficient(String windowName, int index, int count) {
        if ("rectangular".equals(windowName)) return 1.0d;
        return 0.5d - (0.5d * Math.cos((2.0d * Math.PI * index) / (count - 1)));
    }

    public static final class Result {
        public final int sampleCount;
        public final long firstTimestampNs;
        public final long lastTimestampNs;
        public final double sampleRateHz;
        public final double frequencyResolutionHz;
        public final double mean;
        public final double rms;
        public final double timingJitterRatio;
        public final String window;
        public final double dominantFrequencyHz;
        public final double dominantPower;
        public final double spectralCentroidHz;
        public final double totalNonDcPower;
        public final boolean flatSignal;
        public final String qualityState;
        public final double[] frequenciesHz;
        public final double[] powers;

        private Result(int sampleCount,
                       long firstTimestampNs,
                       long lastTimestampNs,
                       double sampleRateHz,
                       double frequencyResolutionHz,
                       double mean,
                       double rms,
                       double timingJitterRatio,
                       String window,
                       double dominantFrequencyHz,
                       double dominantPower,
                       double spectralCentroidHz,
                       double totalNonDcPower,
                       boolean flatSignal,
                       String qualityState,
                       double[] frequenciesHz,
                       double[] powers) {
            this.sampleCount = sampleCount;
            this.firstTimestampNs = firstTimestampNs;
            this.lastTimestampNs = lastTimestampNs;
            this.sampleRateHz = sampleRateHz;
            this.frequencyResolutionHz = frequencyResolutionHz;
            this.mean = mean;
            this.rms = rms;
            this.timingJitterRatio = timingJitterRatio;
            this.window = window;
            this.dominantFrequencyHz = dominantFrequencyHz;
            this.dominantPower = dominantPower;
            this.spectralCentroidHz = spectralCentroidHz;
            this.totalNonDcPower = totalNonDcPower;
            this.flatSignal = flatSignal;
            this.qualityState = qualityState;
            this.frequenciesHz = Arrays.copyOf(frequenciesHz, frequenciesHz.length);
            this.powers = Arrays.copyOf(powers, powers.length);
        }
    }
}
