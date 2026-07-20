package com.termux.app.rafaelia;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import org.junit.Test;

public class RafaeliaZeroRuntimeTest {

    @Test
    public void statusAbiIsStable() {
        assertEquals(0, RafaeliaZeroRuntime.OK);
        assertEquals(-1, RafaeliaZeroRuntime.E_NULL);
        assertEquals(-5, RafaeliaZeroRuntime.E_CRC);
        assertEquals(-7, RafaeliaZeroRuntime.E_STATE);
        assertEquals(1024, RafaeliaZeroRuntime.MAX_PAYLOAD);
    }

    @Test
    public void hostWithoutAndroidSoFailsClosed() {
        if (!RafaeliaZeroRuntime.isAvailable()) {
            assertEquals(RafaeliaZeroRuntime.E_STATE, RafaeliaZeroRuntime.init());
            assertEquals(0, RafaeliaZeroRuntime.stateDigest());
            assertEquals(0, RafaeliaZeroRuntime.acceptedCount());
            assertEquals(0, RafaeliaZeroRuntime.rejectedCount());
            assertFalse(RafaeliaZeroRuntime.isAvailable());
        }
    }
}
