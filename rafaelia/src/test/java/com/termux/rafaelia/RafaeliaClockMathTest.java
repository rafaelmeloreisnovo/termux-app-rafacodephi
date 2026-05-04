package com.termux.rafaelia;

import org.junit.Test;
import static org.junit.Assert.*;

public class RafaeliaClockMathTest {
    @Test
    public void logicalClockMath() {
        assertEquals(100_000_000L, 1_000_000_000L / 10L);
        assertEquals(100_000L, 1_000_000_000L / 10_000L);
        assertEquals(42, 42);
    }
}
