package com.termux.app.benchmark;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class PaBenchmarkReceiptStateTest {

    @Test
    public void passRequiresEveryRuntimePredicate() {
        assertEquals(PaBenchmarkReceipt.STATE_PASS,
            PaBenchmarkReceipt.classifyEvidenceState(false, false, 0, false, true));
    }

    @Test
    public void timeoutIsBlocked() {
        assertEquals(PaBenchmarkReceipt.STATE_BLOCKED,
            PaBenchmarkReceipt.classifyEvidenceState(true, false, -1, false, false));
        assertEquals("PROCESS_TIMEOUT",
            PaBenchmarkReceipt.classifyEvidenceReason(true, false, -1, false, false));
    }

    @Test
    public void executionErrorIsBlocked() {
        assertEquals(PaBenchmarkReceipt.STATE_BLOCKED,
            PaBenchmarkReceipt.classifyEvidenceState(false, true, -1, false, false));
    }

    @Test
    public void nonZeroExitIsFail() {
        assertEquals(PaBenchmarkReceipt.STATE_FAIL,
            PaBenchmarkReceipt.classifyEvidenceState(false, false, 7, false, true));
    }

    @Test
    public void truncatedStdoutInvalidatesOtherwiseSuccessfulRun() {
        assertEquals(PaBenchmarkReceipt.STATE_INVALIDATED,
            PaBenchmarkReceipt.classifyEvidenceState(false, false, 0, true, true));
    }

    @Test
    public void missingMarkersInvalidatesOtherwiseSuccessfulRun() {
        assertEquals(PaBenchmarkReceipt.STATE_INVALIDATED,
            PaBenchmarkReceipt.classifyEvidenceState(false, false, 0, false, false));
    }
}
