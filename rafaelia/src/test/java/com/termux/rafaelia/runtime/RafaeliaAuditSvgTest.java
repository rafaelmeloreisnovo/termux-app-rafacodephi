package com.termux.rafaelia.runtime;

import org.junit.Test;

import static org.junit.Assert.assertTrue;

public class RafaeliaAuditSvgTest {

    @Test
    public void renderPhiWindow_generatesSvg() {
        int[] phi = new int[42];
        for (int i = 0; i < phi.length; i++) phi[i] = i * 10;
        String svg = RafaeliaAuditSvg.renderPhiWindow(phi);
        assertTrue(svg.contains("<svg"));
        assertTrue(svg.contains("polyline"));
        assertTrue(svg.contains("phiWindow(42)"));
    }
}
