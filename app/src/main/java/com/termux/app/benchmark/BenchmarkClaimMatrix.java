package com.termux.app.benchmark;

import android.content.Context;
import android.util.AtomicFile;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

/**
 * Seven-gate claim matrix. Gates are evaluated independently; PASS does not
 * propagate from a narrower claim to a broader one.
 */
public final class BenchmarkClaimMatrix {

    public static final String SCHEMA = "rafcodephi.benchmark-claim-matrix/v1";
    public static final String FILE_NAME = "benchmark_claim_matrix_v1.json";

    private BenchmarkClaimMatrix() {}

    public static JSONObject evaluate(Context context) throws Exception {
        JSONObject out = new JSONObject();
        out.put("schema", SCHEMA);
        out.put("gate_inheritance", false);
        out.put("composite_score_allowed", false);

        JSONObject receipt = PaBenchmarkReceipt.read(context);
        JSONObject seriesReport = PaBenchmarkSeriesAnalyzer.analyze(context);
        JSONArray gates = new JSONArray();

        boolean executionPass = receipt != null
            && receipt.optBoolean("claim_allowed_runtime_execution", false);
        boolean timingPass = receipt != null
            && receipt.optBoolean("claim_allowed_timing_measurement", false);

        gates.put(gate("EXECUTION_PROOF",
            receipt == null ? "NOT_MEASURED" : executionPass ? "PASS" : receipt.optString("evidence_state", "INVALIDATED"),
            executionPass,
            receipt == null ? "NO_READABLE_PA_RECEIPT" : receipt.optString("evidence_reason", "UNKNOWN")));

        gates.put(gate("MEASUREMENT_VALIDITY",
            receipt == null ? "NOT_MEASURED" : timingPass ? "PASS" : "BLOCKED",
            timingPass,
            receipt == null ? "NO_READABLE_PA_RECEIPT"
                : receipt.optInt("pa_protocol_version", 0) < 2
                    ? "LEGACY_PROTOCOL_EXECUTION_ONLY"
                    : "PROTOCOL_V2_TIMING_PREDICATES_NOT_COMPLETE"));

        SeriesGate seriesGate = evaluateSeries(seriesReport);
        gates.put(gate("SERIES_VALIDITY", seriesGate.state, seriesGate.claimAllowed, seriesGate.reason));

        EnvironmentGate environmentGate = evaluateEnvironment(seriesReport);
        gates.put(gate("ENVIRONMENT_VALIDITY", environmentGate.state, false, environmentGate.reason));

        gates.put(gate("COMPARABILITY_VALIDITY", "BLOCKED", false,
            "NO_VERSIONED_REFERENCE_BASELINE_WITH_MATCHING_WORKLOAD_INPUT_BUILD_AND_UNCERTAINTY"));
        gates.put(gate("ENERGY_VALIDITY", "BLOCKED", false,
            "NO_CALIBRATED_ENERGY_MEASUREMENT_SOURCE"));
        gates.put(gate("PUBLICATION_VALIDITY", "BLOCKED", false,
            "REQUIRES_RELEASE_ARTIFACT_RAW_RECEIPTS_SERIES_ENVIRONMENT_DISCLOSURE_AND_REVIEW"));

        out.put("gates", gates);
        out.put("claim_allowed_runtime_execution", executionPass);
        out.put("claim_allowed_timing_measurement", timingPass);
        out.put("claim_allowed_distribution_summary", seriesGate.claimAllowed);
        out.put("claim_allowed_environment_stability", false);
        out.put("claim_allowed_cross_device_comparison", false);
        out.put("claim_allowed_energy_measurement", false);
        out.put("claim_allowed_public_benchmark_ranking", false);
        out.put("overall_state", overallState(gates));
        out.put("overall_reason", "NARROW_GATES_MAY_PASS_BROAD_CLAIMS_REMAIN_FAIL_CLOSED");
        return out;
    }

    public static File evaluateAndWrite(Context context) throws Exception {
        JSONObject matrix = evaluate(context);
        File directory = new File(context.getFilesDir(), PaBenchmarkReceipt.DIRECTORY);
        if (!directory.exists() && !directory.mkdirs() && !directory.exists()) {
            throw new IllegalStateException("Unable to create benchmark claim matrix directory");
        }
        File out = new File(directory, FILE_NAME);
        AtomicFile atomic = new AtomicFile(out);
        FileOutputStream stream = null;
        try {
            stream = atomic.startWrite();
            byte[] bytes = (matrix.toString() + "\n").getBytes(StandardCharsets.UTF_8);
            stream.write(bytes);
            stream.flush();
            stream.getFD().sync();
            atomic.finishWrite(stream);
            return out;
        } catch (Exception error) {
            if (stream != null) atomic.failWrite(stream);
            throw error;
        }
    }

    private static JSONObject gate(String id, String state, boolean claimAllowed, String reason) throws Exception {
        JSONObject gate = new JSONObject();
        gate.put("id", id);
        gate.put("state", state);
        gate.put("claim_allowed", claimAllowed);
        gate.put("reason", reason);
        return gate;
    }

    private static SeriesGate evaluateSeries(JSONObject report) {
        if (report == null) return new SeriesGate("NOT_MEASURED", false, "NO_SERIES_REPORT");
        JSONArray series = report.optJSONArray("series");
        if (series == null || series.length() == 0) {
            return new SeriesGate(report.optString("state", "NOT_MEASURED"), false,
                report.optString("reason", "NO_GOVERNED_SERIES"));
        }
        boolean ready = false;
        boolean invalidated = false;
        for (int i = 0; i < series.length(); i++) {
            JSONObject item = series.optJSONObject(i);
            if (item == null) continue;
            ready |= item.optBoolean("claim_allowed_distribution_summary", false);
            invalidated |= "INVALIDATED".equals(item.optString("state"));
        }
        if (invalidated) return new SeriesGate("INVALIDATED", false, "AT_LEAST_ONE_GOVERNED_SERIES_INVALIDATED");
        if (ready) return new SeriesGate("PASS", true, "GOVERNED_HOMOGENEOUS_SERIES_TARGET_REACHED");
        return new SeriesGate("OBSERVED_LIMITED", false, "GOVERNED_SERIES_PRESENT_TARGET_NOT_REACHED");
    }

    private static EnvironmentGate evaluateEnvironment(JSONObject report) {
        if (report == null) return new EnvironmentGate("NOT_MEASURED", "NO_SERIES_REPORT");
        JSONArray series = report.optJSONArray("series");
        if (series == null || series.length() == 0) {
            return new EnvironmentGate("NOT_MEASURED", "NO_GOVERNED_SERIES_ENVIRONMENT_WINDOW");
        }
        boolean anyFull = false;
        boolean anyThermal = false;
        for (int i = 0; i < series.length(); i++) {
            JSONObject item = series.optJSONObject(i);
            if (item == null) continue;
            anyFull |= item.optBoolean("full_environment_coverage", false);
            anyThermal |= item.optInt("thermal_interference_samples", 0) > 0;
        }
        if (anyThermal) return new EnvironmentGate("OBSERVED_LIMITED", "SEVERE_THERMAL_INTERFERENCE_RETAINED_IN_SERIES");
        if (anyFull) return new EnvironmentGate("OBSERVED_LIMITED", "FULL_PRE_POST_COVERAGE_OBSERVED_STABILITY_POLICY_NOT_YET_VALIDATED");
        return new EnvironmentGate("OBSERVED_LIMITED", "ENVIRONMENT_OBSERVED_BUT_COVERAGE_INCOMPLETE");
    }

    private static String overallState(JSONArray gates) {
        boolean anyInvalidated = false;
        boolean anyPass = false;
        for (int i = 0; i < gates.length(); i++) {
            JSONObject gate = gates.optJSONObject(i);
            if (gate == null) continue;
            String state = gate.optString("state", "INVALIDATED");
            anyInvalidated |= "INVALIDATED".equals(state);
            anyPass |= "PASS".equals(state);
        }
        if (anyInvalidated) return "INVALIDATED";
        return anyPass ? "OBSERVED_LIMITED" : "BLOCKED";
    }

    private static final class SeriesGate {
        final String state;
        final boolean claimAllowed;
        final String reason;
        SeriesGate(String state, boolean claimAllowed, String reason) {
            this.state = state;
            this.claimAllowed = claimAllowed;
            this.reason = reason;
        }
    }

    private static final class EnvironmentGate {
        final String state;
        final String reason;
        EnvironmentGate(String state, String reason) {
            this.state = state;
            this.reason = reason;
        }
    }
}
