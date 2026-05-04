package com.termux.rafaelia.runtime;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class RafaeliaAuditComparatorTest {

    @Test
    public void compare_computesRateAndCycleDelta() {
        String base = "{\"commitRate\":0.80,\"rollbackRate\":0.20,\"currentCycle\":7}";
        String cand = "{\"commitRate\":0.90,\"rollbackRate\":0.10,\"currentCycle\":9}";

        RafaeliaAuditComparator.Delta d = RafaeliaAuditComparator.compare(base, cand);

        assertEquals(0.10, d.commitRateDelta, 0.000001);
        assertEquals(-0.10, d.rollbackRateDelta, 0.000001);
        assertEquals(2, d.cycleDelta);
    }
}
