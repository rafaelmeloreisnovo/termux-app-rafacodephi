package com.termux.rafaelia;

import org.junit.Test;

import static org.junit.Assert.*;

public class RafaeliaUtilsDirectPipelineTest {

    @Test
    public void directPipelineAvailabilityMatchesCore() {
        assertEquals(RafaeliaCore.isNativeAvailable(), RafaeliaUtils.isDirectPipelineAvailable());
    }

    @Test
    public void processCommitGateReturnsObject() {
        RafaeliaCore.CommitGateResult r = RafaeliaUtils.processCommitGate(new byte[]{1, 3, 5, 7}, 4);
        assertNotNull(r);
    }

    @Test
    public void toroidalTickQ16IsStableWhenUnavailable() {
        int phi = RafaeliaUtils.toroidalTickQ16();
        if (!RafaeliaCore.isNativeAvailable()) {
            assertEquals(0, phi);
        }
    }
}
