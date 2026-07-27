package com.termux.app.api.sensor;

import org.junit.Assert;
import org.junit.Test;

public class RafSpectralAnalyzerTest {

    @Test
    public void detectsTenHertzSineWave() {
        int count = 128;
        double sampleRateHz = 128.0d;
        double[] values = new double[count];
        long[] timestamps = new long[count];
        for (int index = 0; index < count; index++) {
            values[index] = Math.sin(2.0d * Math.PI * 10.0d * index / sampleRateHz);
            timestamps[index] = Math.round(index * 1_000_000_000.0d / sampleRateHz);
        }

        RafSpectralAnalyzer.Result result = RafSpectralAnalyzer.analyze(values, timestamps, "hann");

        Assert.assertEquals(10.0d, result.dominantFrequencyHz, 0.01d);
        Assert.assertEquals("EVIDENCIADO_COMPUTACIONAL", result.qualityState);
        Assert.assertFalse(result.flatSignal);
        Assert.assertEquals(1.0d, result.frequencyResolutionHz, 0.01d);
    }

    @Test
    public void removesDcAndMarksFlatSignal() {
        int count = 32;
        double[] values = new double[count];
        long[] timestamps = new long[count];
        for (int index = 0; index < count; index++) {
            values[index] = 42.0d;
            timestamps[index] = index * 10_000_000L;
        }

        RafSpectralAnalyzer.Result result = RafSpectralAnalyzer.analyze(values, timestamps, "rectangular");

        Assert.assertTrue(result.flatSignal);
        Assert.assertEquals(0.0d, result.dominantFrequencyHz, 0.0d);
        Assert.assertEquals("TOKEN_VAZIO_FLAT_SIGNAL", result.qualityState);
    }

    @Test
    public void exposesTimingJitterInsteadOfHidingIt() {
        int count = 32;
        double[] values = new double[count];
        long[] timestamps = new long[count];
        long elapsed = 0L;
        for (int index = 0; index < count; index++) {
            values[index] = Math.sin(2.0d * Math.PI * index / 8.0d);
            timestamps[index] = elapsed;
            if (index + 1 < count) elapsed += index % 2 == 0 ? 1_000_000L : 20_000_000L;
        }

        RafSpectralAnalyzer.Result result = RafSpectralAnalyzer.analyze(values, timestamps, "hann");

        Assert.assertTrue(result.timingJitterRatio > RafSpectralAnalyzer.DEFAULT_MAX_JITTER_RATIO);
        Assert.assertEquals("TOKEN_VAZIO_TIMING_JITTER", result.qualityState);
    }

    @Test(expected = IllegalArgumentException.class)
    public void rejectsNonIncreasingTimestamps() {
        double[] values = new double[16];
        long[] timestamps = new long[16];
        for (int index = 0; index < timestamps.length; index++) timestamps[index] = index;
        timestamps[8] = timestamps[7];
        RafSpectralAnalyzer.analyze(values, timestamps, "hann");
    }

    @Test
    public void validatesBoundedRequestAndTimeoutBudget() {
        RafSpectralContract.ValidationResult valid = RafSpectralContract.validateRequest(
            RafSpectralContract.PROTOCOL_VERSION,
            "spectrum-1",
            RafSensorContract.SENSOR_ACCELEROMETER,
            RafSpectralContract.AXIS_X,
            128,
            20_000,
            5_000,
            "hann",
            true,
            "com.termux.api"
        );
        Assert.assertTrue(valid.valid);

        RafSpectralContract.ValidationResult invalid = RafSpectralContract.validateRequest(
            RafSpectralContract.PROTOCOL_VERSION,
            "spectrum-2",
            RafSensorContract.SENSOR_ACCELEROMETER,
            RafSpectralContract.AXIS_X,
            512,
            200_000,
            5_000,
            "hann",
            true,
            "com.termux.api"
        );
        Assert.assertFalse(invalid.valid);
        Assert.assertEquals("ERR_TIMEOUT_BUDGET", invalid.errorCode);
    }
}
