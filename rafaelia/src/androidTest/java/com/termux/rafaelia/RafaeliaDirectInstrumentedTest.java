package com.termux.rafaelia;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class RafaeliaDirectInstrumentedTest {

    @Test
    public void directBridgeLoadsOrFallsBackSafely() {
        // Em device/emulator real: valida que API responde com contrato estável.
        RafaeliaCore.CommitGateResult r = RafaeliaCore.processWithCommitGate(new byte[]{1,2,3,4}, 4);
        assertNotNull(r);
        assertTrue(RafaeliaCore.getCurrentCycle() >= 0 && RafaeliaCore.getCurrentCycle() < 42);
    }
}
