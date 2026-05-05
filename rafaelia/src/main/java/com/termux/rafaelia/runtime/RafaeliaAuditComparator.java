package com.termux.rafaelia.runtime;

import org.json.JSONObject;
import org.json.JSONException;

/**
 * Comparador simples de auditorias entre builds consecutivos.
 */
public final class RafaeliaAuditComparator {

    private RafaeliaAuditComparator() {}

    public static final class Delta {
        public final double commitRateDelta;
        public final double rollbackRateDelta;
        public final int cycleDelta;

        Delta(double commitRateDelta, double rollbackRateDelta, int cycleDelta) {
            this.commitRateDelta = commitRateDelta;
            this.rollbackRateDelta = rollbackRateDelta;
            this.cycleDelta = cycleDelta;
        }

        public JSONObject toJson() {
            try {
                JSONObject out = new JSONObject();
                out.put("commitRateDelta", commitRateDelta);
                out.put("rollbackRateDelta", rollbackRateDelta);
                out.put("cycleDelta", cycleDelta);
                return out;
            } catch (JSONException e) {
                throw new IllegalStateException("Failed to serialize audit delta", e);
            }
        }
    }

    public static Delta compare(String baselineAuditJson, String candidateAuditJson) {
        try {
            JSONObject base = new JSONObject(baselineAuditJson);
            JSONObject cand = new JSONObject(candidateAuditJson);
            double cDelta = cand.optDouble("commitRate", 0.0) - base.optDouble("commitRate", 0.0);
            double rDelta = cand.optDouble("rollbackRate", 0.0) - base.optDouble("rollbackRate", 0.0);
            int cycDelta = cand.optInt("currentCycle", 0) - base.optInt("currentCycle", 0);
            return new Delta(cDelta, rDelta, cycDelta);
        } catch (JSONException e) {
            throw new IllegalArgumentException("Invalid audit json payload", e);
        }
    }
}
