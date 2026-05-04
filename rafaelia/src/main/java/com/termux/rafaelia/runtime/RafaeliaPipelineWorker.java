package com.termux.rafaelia.runtime;

import com.termux.rafaelia.RafaeliaCore;
import com.termux.rafaelia.RafaeliaUtils;

/**
 * Worker lógico para execução cíclica do pipeline RAFAELIA.
 * Mantém métricas de commit/rollback e telemetria de convergência (phi/ciclo).
 */
public final class RafaeliaPipelineWorker {

    public static final class Snapshot {
        public final long total;
        public final long committed;
        public final long rolledBack;
        public final int lastPhiQ16;
        public final int lastPhase;
        public final int lastStep;
        public final int currentCycle;

        Snapshot(long total, long committed, long rolledBack, int lastPhiQ16, int lastPhase, int lastStep, int currentCycle) {
            this.total = total;
            this.committed = committed;
            this.rolledBack = rolledBack;
            this.lastPhiQ16 = lastPhiQ16;
            this.lastPhase = lastPhase;
            this.lastStep = lastStep;
            this.currentCycle = currentCycle;
        }

        public double commitRate() {
            return total == 0 ? 0.0 : ((double) committed / (double) total);
        }
    }

    private long total = 0;
    private long committed = 0;
    private long rolledBack = 0;
    private int lastPhiQ16 = 0;
    private int lastPhase = 0;
    private int lastStep = 0;

    // janela de 42 ciclos para auditoria periódica
    private final int[] phiWindow = new int[42];
    private int winPos = 0;

    public Snapshot process(byte[] payload, int len) {
        total++;
        RafaeliaCore.CommitGateResult r = RafaeliaUtils.processCommitGate(payload, len);
        if (r.committed) committed++; else rolledBack++;
        lastPhiQ16 = r.phiQ16;
        lastPhase = r.phase;
        lastStep = r.step;

        phiWindow[winPos] = r.phiQ16;
        winPos = (winPos + 1) % 42;

        return snapshot();
    }

    public Snapshot snapshot() {
        return new Snapshot(total, committed, rolledBack, lastPhiQ16, lastPhase, lastStep, RafaeliaCore.getCurrentCycle());
    }

    public int[] getPhiWindowCopy() {
        int[] out = new int[42];
        System.arraycopy(phiWindow, 0, out, 0, 42);
        return out;
    }
}
