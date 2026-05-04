package com.termux.rafaelia.runtime;

/**
 * Renderizador SVG simples para série temporal de 42 pontos.
 */
public final class RafaeliaAuditSvg {

    private RafaeliaAuditSvg() {}

    public static String renderPhiWindow(int[] phiWindow) {
        final int width = 840;
        final int height = 240;
        final int pad = 20;
        if (phiWindow == null || phiWindow.length == 0) {
            return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"840\" height=\"240\"></svg>";
        }

        int min = phiWindow[0];
        int max = phiWindow[0];
        for (int v : phiWindow) {
            if (v < min) min = v;
            if (v > max) max = v;
        }
        int range = Math.max(1, max - min);
        double dx = (double) (width - 2 * pad) / (double) (phiWindow.length - 1);

        StringBuilder points = new StringBuilder();
        for (int i = 0; i < phiWindow.length; i++) {
            double x = pad + (i * dx);
            double norm = (double) (phiWindow[i] - min) / (double) range;
            double y = (height - pad) - norm * (height - 2 * pad);
            if (i > 0) points.append(' ');
            points.append((int) x).append(',').append((int) y);
        }

        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" + width + "\" height=\"" + height + "\">"
            + "<rect x=\"0\" y=\"0\" width=\"" + width + "\" height=\"" + height + "\" fill=\"#0b1020\"/>"
            + "<polyline fill=\"none\" stroke=\"#22d3ee\" stroke-width=\"2\" points=\"" + points + "\"/>"
            + "<text x=\"20\" y=\"18\" fill=\"#e2e8f0\" font-size=\"12\">phiWindow(42)</text>"
            + "</svg>";
    }
}
