package com.termux.app.api.sensor;

import org.junit.Assert;
import org.junit.Test;

public class RafSensorContractTest {

    @Test
    public void acceptsSupportedSnapshotRequest() {
        RafSensorContract.ValidationResult result = RafSensorContract.validateSnapshotRequest(
            RafSensorContract.PROTOCOL_VERSION_1,
            "job-42",
            RafSensorContract.SENSOR_ACCELEROMETER,
            20_000,
            0,
            true,
            "com.example.client"
        );

        Assert.assertTrue(result.valid);
    }

    @Test
    public void rejectsUnknownSensor() {
        RafSensorContract.ValidationResult result = RafSensorContract.validateSnapshotRequest(
            RafSensorContract.PROTOCOL_VERSION_1,
            "job-42",
            "heart_rate",
            20_000,
            0,
            true,
            "com.example.client"
        );

        Assert.assertFalse(result.valid);
        Assert.assertEquals("ERR_SENSOR_NAME", result.errorCode);
    }

    @Test
    public void rejectsUnsafeRequestId() {
        RafSensorContract.ValidationResult result = RafSensorContract.validateSnapshotRequest(
            RafSensorContract.PROTOCOL_VERSION_1,
            "job 42",
            RafSensorContract.SENSOR_GYROSCOPE,
            20_000,
            0,
            true,
            "com.example.client"
        );

        Assert.assertFalse(result.valid);
        Assert.assertEquals("ERR_REQUEST_ID", result.errorCode);
    }

    @Test
    public void keepsPresetOrderStable() {
        Assert.assertArrayEquals(
            new String[]{"FASTEST", "GAME", "UI", "NORMAL"},
            RafSensorContract.samplingPresetsUs().keySet().toArray(new String[0])
        );
    }
}
