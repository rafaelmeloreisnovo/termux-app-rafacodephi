package com.termux.app.benchmark;

import android.content.Context;
import android.util.AtomicFile;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Statistical layer for PA protocol-v2 receipts.
 *
 * Invariant: unrelated workloads are never pooled. A series is homogeneous only
 * when artifact hash, linker, ABI set, protocol, workload id, operation count
 * and flags are identical. Deterministic score/checksum drift invalidates the
 * series instead of silently splitting it into a more favorable subgroup.
 */
public final class PaBenchmarkSeriesAnalyzer {

    public static final String SCHEMA = "rafcodephi.pa-series-analysis/v1";
    public static final String FILE_NAME = "pa_series_analysis_v1.json";
    public static final int MIN_DISTRIBUTION_N = 30;

    private PaBenchmarkSeriesAnalyzer() {}

    public static File analyzeAndWrite(Context context) throws Exception {
        JSONObject report = analyze(context);
        File out = new File(new File(context.getFilesDir(), PaBenchmarkReceipt.DIRECTORY), FILE_NAME);
        File parent = out.getParentFile();
        if (parent == null || (!parent.exists() && !parent.mkdirs() && !parent.exists())) {
            throw new IllegalStateException("Unable to create PA analysis directory: " + parent);
        }
        AtomicFile atomic = new AtomicFile(out);
        FileOutputStream stream = null;
        try {
            stream = atomic.startWrite();
            byte[] bytes = (report.toString() + "\n").getBytes(StandardCharsets.UTF_8);
            stream.write(bytes);
            stream.flush();
            stream.getFD().sync();
            atomic.finishWrite(stream);
        } catch (Exception error) {
            if (stream != null) atomic.failWrite(stream);
            throw error;
        }
        return out;
    }

    public static JSONObject analyze(Context context) throws Exception {
        JSONObject report = new JSONObject();
        report.put("schema", SCHEMA);
        report.put("minimum_distribution_n", MIN_DISTRIBUTION_N);
        report.put("aggregation_invariant", "same_artifact_abi_linker_protocol_workload_ops_flags_only");
        report.put("heterogeneous_workload_pooling", false);
        report.put("claim_allowed_reproducibility", false);
        report.put("claim_allowed_cross_device_comparison", false);

        File history = PaBenchmarkReceipt.getHistoryDirectory(context);
        File[] files = history.listFiles((dir, name) -> name.endsWith(".json"));
        if (files == null || files.length == 0) {
            report.put("state", "NOT_MEASURED");
            report.put("reason", "NO_HISTORY_RECEIPTS");
            report.put("series", new JSONArray());
            return report;
        }
        Arrays.sort(files, Comparator.comparing(File::getName));

        Map<String, SeriesAccumulator> groups = new LinkedHashMap<>();
        int eligibleReceipts = 0;
        int ignoredReceipts = 0;

        for (File file : files) {
            JSONObject receipt = PaBenchmarkReceipt.readHistoryFile(file);
            if (!eligibleReceipt(receipt)) {
                ignoredReceipts++;
                continue;
            }
            eligibleReceipts++;
            JSONArray workloads = receipt.optJSONArray("workloads");
            if (workloads == null) {
                ignoredReceipts++;
                continue;
            }
            String elf = receipt.optString("elf_sha256", "");
            String linker = receipt.optString("linker", "");
            String abi = receipt.optJSONArray("supported_abis") == null
                ? "[]" : receipt.optJSONArray("supported_abis").toString();
            int protocol = receipt.optInt("pa_protocol_version", 0);

            for (int i = 0; i < workloads.length(); i++) {
                JSONObject row = workloads.optJSONObject(i);
                if (row == null || row.isNull("elapsed_ns")) continue;
                long elapsed = row.optLong("elapsed_ns", -1L);
                if (elapsed <= 0L) continue;
                String id = row.optString("id", "");
                String ops = row.optString("operations_hex", "");
                String flags = row.optString("flags_hex", "");
                String keyMaterial = elf + "|" + linker + "|" + abi + "|p" + protocol + "|" + id + "|" + ops + "|" + flags;
                String key = sha256(keyMaterial);
                SeriesAccumulator acc = groups.get(key);
                if (acc == null) {
                    acc = new SeriesAccumulator(key, keyMaterial, id, elf, linker, abi, protocol, ops, flags);
                    groups.put(key, acc);
                }
                acc.add(row, elapsed, file.getName());
            }
        }

        JSONArray series = new JSONArray();
        boolean anyReady = false;
        boolean anyInvalidated = false;
        for (SeriesAccumulator acc : groups.values()) {
            JSONObject item = acc.toJson();
            series.put(item);
            anyReady |= item.optBoolean("claim_allowed_distribution_summary", false);
            anyInvalidated |= "INVALIDATED".equals(item.optString("state"));
        }

        report.put("eligible_receipts", eligibleReceipts);
        report.put("ignored_receipts", ignoredReceipts);
        report.put("series_count", series.length());
        report.put("series", series);
        if (series.length() == 0) {
            report.put("state", "NOT_MEASURED");
            report.put("reason", "NO_PROTOCOL_V2_PASS_RECEIPTS");
        } else if (anyInvalidated) {
            report.put("state", "INVALIDATED");
            report.put("reason", "DETERMINISTIC_IDENTITY_DRIFT_IN_AT_LEAST_ONE_SERIES");
        } else if (anyReady) {
            report.put("state", "OBSERVED_LIMITED");
            report.put("reason", "DISTRIBUTION_SUMMARY_READY_ENVIRONMENTAL_COMPARABILITY_NOT_YET_PROVEN");
        } else {
            report.put("state", "OBSERVED_LIMITED");
            report.put("reason", "HOMOGENEOUS_SERIES_BELOW_N30");
        }
        return report;
    }

    private static boolean eligibleReceipt(JSONObject receipt) {
        return receipt != null
            && PaBenchmarkReceipt.SCHEMA.equals(receipt.optString("schema"))
            && PaBenchmarkReceipt.STATE_PASS.equals(receipt.optString("evidence_state"))
            && receipt.optInt("pa_protocol_version", 0) >= 2
            && receipt.optBoolean("claim_allowed_timing_measurement", false)
            && !receipt.optString("elf_sha256", "").isEmpty();
    }

    private static String sha256(String value) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] raw = digest.digest(value.getBytes(StandardCharsets.UTF_8));
        StringBuilder out = new StringBuilder(raw.length * 2);
        for (byte b : raw) out.append(String.format(Locale.US, "%02x", b & 0xff));
        return out.toString();
    }

    private static final class SeriesAccumulator {
        final String key;
        final String keyMaterial;
        final String workloadId;
        final String elfSha256;
        final String linker;
        final String abiSet;
        final int protocol;
        final String operationsHex;
        final String flagsHex;
        final List<Long> elapsedNs = new ArrayList<>();
        final List<String> sourceReceipts = new ArrayList<>();
        String deterministicScoreHex;
        String checksumHex;
        boolean identityDrift;

        SeriesAccumulator(String key, String keyMaterial, String workloadId, String elfSha256,
                          String linker, String abiSet, int protocol, String operationsHex, String flagsHex) {
            this.key = key;
            this.keyMaterial = keyMaterial;
            this.workloadId = workloadId;
            this.elfSha256 = elfSha256;
            this.linker = linker;
            this.abiSet = abiSet;
            this.protocol = protocol;
            this.operationsHex = operationsHex;
            this.flagsHex = flagsHex;
        }

        void add(JSONObject row, long elapsed, String receiptName) {
            String score = row.optString("deterministic_score_hex", "");
            String checksum = row.optString("checksum_hex", "");
            if (deterministicScoreHex == null) deterministicScoreHex = score;
            else if (!deterministicScoreHex.equals(score)) identityDrift = true;
            if (checksumHex == null) checksumHex = checksum;
            else if (!checksumHex.equals(checksum)) identityDrift = true;
            elapsedNs.add(elapsed);
            sourceReceipts.add(receiptName);
        }

        JSONObject toJson() throws Exception {
            JSONObject out = new JSONObject();
            out.put("series_key_sha256", key);
            out.put("series_key_material", keyMaterial);
            out.put("workload_id", workloadId);
            out.put("elf_sha256", elfSha256);
            out.put("linker", linker);
            out.put("abi_set", abiSet);
            out.put("protocol", protocol);
            out.put("operations_hex", operationsHex);
            out.put("flags_hex", flagsHex);
            out.put("deterministic_score_hex", deterministicScoreHex == null ? "" : deterministicScoreHex);
            out.put("checksum_hex", checksumHex == null ? "" : checksumHex);
            out.put("n", elapsedNs.size());
            out.put("identity_drift", identityDrift);
            JSONArray sources = new JSONArray();
            for (String source : sourceReceipts) sources.put(source);
            out.put("source_receipts", sources);

            if (identityDrift) {
                out.put("state", "INVALIDATED");
                out.put("reason", "DETERMINISTIC_SCORE_OR_CHECKSUM_DRIFT");
                out.put("claim_allowed_distribution_summary", false);
                return out;
            }
            if (elapsedNs.isEmpty()) {
                out.put("state", "NOT_MEASURED");
                out.put("reason", "NO_POSITIVE_ELAPSED_NS");
                out.put("claim_allowed_distribution_summary", false);
                return out;
            }

            long[] values = new long[elapsedNs.size()];
            for (int i = 0; i < values.length; i++) values[i] = elapsedNs.get(i);
            Arrays.sort(values);
            Stats stats = Stats.of(values);
            out.put("min_ns", stats.min);
            out.put("max_ns", stats.max);
            out.put("median_ns", stats.median);
            out.put("mean_ns", stats.mean);
            out.put("sample_sd_ns", stats.sampleSd);
            out.put("cv", stats.cv);
            out.put("mad_ns", stats.mad);
            out.put("q1_ns", stats.q1);
            out.put("q3_ns", stats.q3);
            out.put("iqr_ns", stats.q3 - stats.q1);
            out.put("ci95_mean_low_ns", stats.ciLow);
            out.put("ci95_mean_high_ns", stats.ciHigh);
            out.put("ci_method", stats.ciMethod);
            boolean distributionReady = values.length >= MIN_DISTRIBUTION_N;
            out.put("claim_allowed_distribution_summary", distributionReady);
            out.put("claim_allowed_reproducibility", false);
            out.put("claim_allowed_cross_device_comparison", false);
            out.put("state", distributionReady ? "OBSERVED_LIMITED" : "OBSERVED_LIMITED");
            out.put("reason", distributionReady
                ? "N30_DISTRIBUTION_READY_ENVIRONMENTAL_GATES_STILL_OPEN"
                : "HOMOGENEOUS_SERIES_BELOW_N30");
            return out;
        }
    }

    static final class Stats {
        final long min;
        final long max;
        final double median;
        final double mean;
        final double sampleSd;
        final double cv;
        final double mad;
        final double q1;
        final double q3;
        final double ciLow;
        final double ciHigh;
        final String ciMethod;

        private Stats(long min, long max, double median, double mean, double sampleSd,
                      double cv, double mad, double q1, double q3,
                      double ciLow, double ciHigh, String ciMethod) {
            this.min = min;
            this.max = max;
            this.median = median;
            this.mean = mean;
            this.sampleSd = sampleSd;
            this.cv = cv;
            this.mad = mad;
            this.q1 = q1;
            this.q3 = q3;
            this.ciLow = ciLow;
            this.ciHigh = ciHigh;
            this.ciMethod = ciMethod;
        }

        static Stats of(long[] sorted) {
            int n = sorted.length;
            double mean = 0.0;
            for (long value : sorted) mean += value;
            mean /= n;
            double sumSq = 0.0;
            for (long value : sorted) {
                double d = value - mean;
                sumSq += d * d;
            }
            double sd = n > 1 ? Math.sqrt(sumSq / (n - 1)) : 0.0;
            double median = percentile(sorted, 0.50);
            double q1 = percentile(sorted, 0.25);
            double q3 = percentile(sorted, 0.75);
            double[] deviations = new double[n];
            for (int i = 0; i < n; i++) deviations[i] = Math.abs(sorted[i] - median);
            Arrays.sort(deviations);
            double mad = percentile(deviations, 0.50);
            double critical = t95TwoSided(n - 1);
            double margin = n > 1 ? critical * sd / Math.sqrt(n) : 0.0;
            return new Stats(sorted[0], sorted[n - 1], median, mean, sd,
                mean > 0.0 ? sd / mean : Double.NaN,
                mad, q1, q3, mean - margin, mean + margin,
                n > 1 ? "student_t_approx_two_sided_95" : "single_observation_no_interval");
        }

        static double percentile(long[] sorted, double p) {
            if (sorted.length == 1) return sorted[0];
            double pos = p * (sorted.length - 1);
            int lo = (int) Math.floor(pos);
            int hi = (int) Math.ceil(pos);
            if (lo == hi) return sorted[lo];
            double fraction = pos - lo;
            return sorted[lo] + fraction * (sorted[hi] - sorted[lo]);
        }

        static double percentile(double[] sorted, double p) {
            if (sorted.length == 1) return sorted[0];
            double pos = p * (sorted.length - 1);
            int lo = (int) Math.floor(pos);
            int hi = (int) Math.ceil(pos);
            if (lo == hi) return sorted[lo];
            double fraction = pos - lo;
            return sorted[lo] + fraction * (sorted[hi] - sorted[lo]);
        }

        // Conservative table/approximation for a two-sided 95% mean interval.
        static double t95TwoSided(int df) {
            if (df <= 0) return 0.0;
            if (df == 1) return 12.706;
            if (df == 2) return 4.303;
            if (df == 3) return 3.182;
            if (df == 4) return 2.776;
            if (df == 5) return 2.571;
            if (df == 6) return 2.447;
            if (df == 7) return 2.365;
            if (df == 8) return 2.306;
            if (df == 9) return 2.262;
            if (df == 10) return 2.228;
            if (df <= 12) return 2.201;
            if (df <= 15) return 2.145;
            if (df <= 20) return 2.086;
            if (df <= 25) return 2.060;
            if (df <= 30) return 2.042;
            if (df <= 60) return 2.000;
            if (df <= 120) return 1.980;
            return 1.960;
        }
    }
}
