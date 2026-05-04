package com.termux.rafaelia;

import org.junit.Test;
import java.nio.ByteBuffer;

import static org.junit.Assert.*;

public class RafaeliaCoreTest {

    @Test
    public void fallbackStateIsSafeWhenNativeUnavailable() {
        if (!RafaeliaCore.isNativeAvailable()) {
            assertEquals("{}", RafaeliaCore.getHwProfile());
            assertEquals(0, RafaeliaCore.getNativeArenaUsed());
            assertEquals(0, RafaeliaCore.process(new byte[]{1,2,3}, 3));
            assertEquals(0, RafaeliaCore.step());
        }
    }

    @Test
    public void commitGateNeverCrashesOnInput() {
        RafaeliaCore.CommitGateResult r = RafaeliaCore.processWithCommitGate(new byte[]{10,20,30,40}, 4);
        assertNotNull(r);
        if (!RafaeliaCore.isNativeAvailable()) {
            assertFalse(r.committed);
        }
    }

    @Test
    public void cycleAlwaysWithinRange() {
        int c = RafaeliaCore.getCurrentCycle();
        assertTrue(c >= 0 && c < 42);
        ByteBuffer dbg = ByteBuffer.allocateDirect(128);
        RafaeliaCore.debugSingleStep(dbg, 128);
        int c2 = RafaeliaCore.getCurrentCycle();
        assertTrue(c2 >= 0 && c2 < 42);
    }
}
