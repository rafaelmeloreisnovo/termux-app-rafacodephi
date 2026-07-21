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
    public void acceptsProtocolTwoCatalogAndBatchEnvelope() {
        RafSensorContract.ValidationResult result = RafSensorContract.validateBridgeRequest(
            RafSensorContract.PROTOCOL_VERSION_2,
            "batch-2026.07.20",
            5_000,
            true,
            "com.termux.api"
        );
        Assert.assertTrue(result.valid);
    }

    @Test
    public void rejectsProtocolOneForBridgeEnvelope() {
        RafSensorContract.ValidationResult result = RafSensorContract.validateBridgeRequest(
            RafSensorContract.PROTOCOL_VERSION_1,
            "batch-1",
            5_000,
            true,
            "com.termux.api"
        );
        Assert.assertFalse(result.valid);
        Assert.assertEquals("ERR_PROTOCOL_VERSION", result.errorCode);
    }

    @Test
    public void callbackCreatorMustMatchClaimedClient() {
        Assert.assertTrue(RafSensorContract.callbackBelongsToClient("com.termux.api", "com.termux.api"));
        Assert.assertFalse(RafSensorContract.callbackBelongsToClient("com.evil.client", "com.termux.api"));
        Assert.assertFalse(RafSensorContract.callbackBelongsToClient(null, "com.termux.api"));
    }

    @Test
    public void clampsBatchTimeoutToContractBounds() {
        Assert.assertEquals(RafSensorContract.MIN_BATCH_TIMEOUT_MS,
            RafSensorContract.normalizeBatchTimeoutMs(1));
        Assert.assertEquals(RafSensorContract.MAX_BATCH_TIMEOUT_MS,
            RafSensorContract.normalizeBatchTimeoutMs(99_999));
        Assert.assertEquals(RafSensorContract.DEFAULT_BATCH_TIMEOUT_MS,
            RafSensorContract.normalizeBatchTimeoutMs(0));
    }

    @Test
    public void keepsPresetOrderStable() {
        Assert.assertArrayEquals(
            new String[]{"FASTEST", "GAME", "UI", "NORMAL"},
            RafSensorContract.samplingPresetsUs().keySet().toArray(new String[0])
        );
    }
}
