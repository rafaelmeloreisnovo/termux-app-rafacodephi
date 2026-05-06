package com.termux.rafaelia.runtime;

import com.termux.rafaelia.RafaeliaCore;
import com.termux.rafaelia.RafaeliaUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

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

        public double rollbackRate() {
            return total == 0 ? 0.0 : ((double) rolledBack / (double) total);
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
        return new Snapshot(total, committed, rolledBack, lastPhiQ16, lastPhase, lastStep, safeCurrentCycle());
    }

    private int safeCurrentCycle() {
        try {
            return RafaeliaCore.getCurrentCycle();
        } catch (Throwable ignored) {
            return 0;
        }
    }


    public String exportAuditJson() {
        Snapshot s = snapshot();
        try {
            JSONObject root = new JSONObject();
            root.put("schemaVersion", 1);
            root.put("total", s.total);
            root.put("committed", s.committed);
            root.put("rolledBack", s.rolledBack);
            root.put("commitRate", s.commitRate());
            root.put("rollbackRate", s.rollbackRate());
            root.put("lastPhiQ16", s.lastPhiQ16);
            root.put("lastPhase", s.lastPhase);
            root.put("lastStep", s.lastStep);
            root.put("currentCycle", s.currentCycle);

            JSONArray arr = new JSONArray();
            for (int v : getPhiWindowOrdered()) arr.put(v);
            root.put("phiWindow", arr);
            return root.toString();
        } catch (JSONException e) {
            return "{\"schemaVersion\":1,\"error\":\"json_serialize_failed\"}";
        }
    }

    public int[] getPhiWindowCopy() {
        int[] out = new int[42];
        System.arraycopy(phiWindow, 0, out, 0, 42);
        return out;
    }

    /**
     * Retorna a janela de 42 elementos em ordem temporal (mais antigo -> mais novo).
     */
    public int[] getPhiWindowOrdered() {
        int[] out = new int[42];
        for (int i = 0; i < 42; i++) {
            out[i] = phiWindow[(winPos + i) % 42];
        }
        return out;
    }
}
