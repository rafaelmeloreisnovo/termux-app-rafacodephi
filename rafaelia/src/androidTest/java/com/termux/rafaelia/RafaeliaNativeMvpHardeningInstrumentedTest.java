package com.termux.rafaelia;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import org.junit.Assume;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class RafaeliaNativeMvpHardeningInstrumentedTest {

    @Test
    public void phaseClosesAt42Steps() {
        Assume.assumeTrue(RafaeliaCore.isNativeAvailable());
        ByteBuffer dbg = ByteBuffer.allocateDirect(512).order(ByteOrder.nativeOrder());
        int lastPhi = 0;
        for (int i = 0; i < 42; i++) {
            lastPhi = RafaeliaCore.debugSingleStep(dbg, 512);
        }
        assertTrue(lastPhi >= 0);
        assertEquals(0, RafaeliaCore.getCurrentCycle());
    }

    @Test
    public void invalidVcpuReturnsError() {
        Assume.assumeTrue(RafaeliaCore.isNativeAvailable());
        assertEquals(-1, RafaeliaCore.stepVcpu(-1));
        assertEquals(-1, RafaeliaCore.stepVcpu(99));
    }

    @Test
    public void crcRejectsLenGreaterThanCapacity() {
        Assume.assumeTrue(RafaeliaCore.isNativeAvailable());
        ByteBuffer tiny = ByteBuffer.allocateDirect(8).order(ByteOrder.nativeOrder());
        assertEquals(-1, RafaeliaCore.crc32Native(tiny, 16));
    }

    @Test
    public void scheduler10HzAnd10kHzPeriod() {
        Assume.assumeTrue(RafaeliaCore.isNativeAvailable());
        assertEquals(0, RafaeliaCore.initVcpuScheduler(10));
        assertTrue(RafaeliaCore.getClockProfile().contains("\"period_ns\":100000000"));
        assertEquals(0, RafaeliaCore.initVcpuScheduler(10000));
        assertTrue(RafaeliaCore.getClockProfile().contains("\"period_ns\":100000"));
    }
}
