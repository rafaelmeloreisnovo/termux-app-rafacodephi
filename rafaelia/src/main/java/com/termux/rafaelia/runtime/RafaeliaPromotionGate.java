package com.termux.rafaelia.runtime;

/** Política formal de gate por release baseada em rollback rate. */
public final class RafaeliaPromotionGate {

    private RafaeliaPromotionGate() {}

    /** Threshold default: máximo 10% de rollback por janela/release. */
    public static final double DEFAULT_MAX_ROLLBACK_RATE = 0.10;

    public static boolean isPromotable(RafaeliaPipelineWorker.Snapshot s) {
        return isPromotable(s, DEFAULT_MAX_ROLLBACK_RATE);
    }

    public static boolean isPromotable(RafaeliaPipelineWorker.Snapshot s, double maxRollbackRate) {
        if (s == null || s.total <= 0) return false;
        double rollbackRate = (double) s.rolledBack / (double) s.total;
        return rollbackRate <= maxRollbackRate;
    }
}
