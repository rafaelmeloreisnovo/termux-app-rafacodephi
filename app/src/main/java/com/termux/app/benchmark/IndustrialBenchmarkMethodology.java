package com.termux.app.benchmark;

import android.content.Context;
import android.os.Build;
import android.util.AtomicFile;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/** Generates a device-local, auditable benchmark methodology and gap document. */
public final class IndustrialBenchmarkMethodology {

    public static final String FILE_NAME = "RAFCODEPHI_INTERNAL_VECTRA_BENCHMARK_METHODS_V3.md";

    private IndustrialBenchmarkMethodology() {}

    public static File write(Context context) throws IOException {
        File directory = context.getExternalFilesDir("benchmark-methods");
        if (directory == null) directory = new File(context.getFilesDir(), "benchmark-methods");
        if (!directory.exists() && !directory.mkdirs() && !directory.exists()) {
            throw new IOException("Unable to create benchmark methodology directory: " + directory);
        }
        File file = new File(directory, FILE_NAME);
        AtomicFile atomicFile = new AtomicFile(file);
        FileOutputStream output = null;
        try {
            output = atomicFile.startWrite();
            byte[] bytes = buildMarkdown(context).getBytes(StandardCharsets.UTF_8);
            output.write(bytes);
            output.flush();
            output.getFD().sync();
            atomicFile.finishWrite(output);
            return file;
        } catch (IOException error) {
            if (output != null) atomicFile.failWrite(output);
            throw error;
        }
    }

    public static String buildMarkdown(Context context) {
        StringBuilder sb = new StringBuilder(24_576);
        sb.append("# Termux RAFCODEΦ · Internal Vectra Benchmark Methods V3\n\n");
        sb.append("Generated: ").append(utcNow()).append("\n\n");

        sb.append("## 0. Deployment truth\n\n");
        sb.append("- Component: `INTERNAL_TERMUX_RAFCODEPHI_SCREEN`.\n");
        sb.append("- Android package: `").append(context.getPackageName()).append("`.\n");
        sb.append("- Separate Vectras/Vectras-VM-Android app required: **NO**.\n");
        sb.append("- Separate Vectras repository or CI required for this screen: **NO**.\n");
        sb.append("- Vectra here is the diagnostics/benchmark surface bundled inside this Termux RAFCODEΦ APK.\n\n");

        sb.append("## 1. Measurement architecture\n\n");
        sb.append("A benchmark number is not independently promotable. V3 evaluates seven gates: ")
            .append("`EXECUTION_PROOF`, `MEASUREMENT_VALIDITY`, `SERIES_VALIDITY`, `ENVIRONMENT_VALIDITY`, ")
            .append("`COMPARABILITY_VALIDITY`, `ENERGY_VALIDITY`, and `PUBLICATION_VALIDITY`. ")
            .append("No gate inherits PASS from another gate.\n\n");
        sb.append("User-space timing observes a combined system: silicon + firmware + kernel + scheduler + Android policy + linker + compiler artifact + memory/thermal/power state. ")
            .append("The PA payload removes JNI/libc/malloc from its measured native core, but this does not isolate silicon.\n\n");

        sb.append("## 2. Device under test\n\n");
        sb.append("- Manufacturer: ").append(Build.MANUFACTURER).append("\n");
        sb.append("- Model: ").append(Build.MODEL).append("\n");
        sb.append("- Android: ").append(Build.VERSION.RELEASE).append(" (API ").append(Build.VERSION.SDK_INT).append(")\n");
        sb.append("- ABI set: ").append(Build.SUPPORTED_ABIS == null ? "UNAVAILABLE" : join(Build.SUPPORTED_ABIS)).append("\n");
        sb.append("- CPUs visible to app: ").append(Runtime.getRuntime().availableProcessors()).append("\n\n");

        JSONObject receipt = PaBenchmarkReceipt.read(context);
        String readState = PaBenchmarkReceipt.getReadState(context);
        sb.append("## 3. Current PA execution / measurement evidence\n\n");
        if ("NOT_MEASURED".equals(readState)) {
            sb.append("- State: `NOT_MEASURED`.\n");
            sb.append("- TOKEN_VAZIO: this build/install has not produced a PA receipt.\n");
            sb.append("- Next action: run one PA observation or a governed n=30 series.\n\n");
        } else if (receipt == null) {
            sb.append("- State: `INVALIDATED`.\n");
            sb.append("- Reason: latest receipt exists but is unreadable under the receipt contract.\n");
            sb.append("- Claim promotion: false.\n\n");
        } else {
            sb.append("- Evidence state: `").append(receipt.optString("evidence_state", "INVALIDATED")).append("`.\n");
            sb.append("- Reason: `").append(receipt.optString("evidence_reason", "UNKNOWN")).append("`.\n");
            sb.append("- PA protocol: ").append(receipt.optInt("pa_protocol_version", 0)).append(" / ")
                .append(receipt.optString("pa_protocol_state", "UNKNOWN")).append(".\n");
            sb.append("- Runtime execution claim: ").append(receipt.optBoolean("claim_allowed_runtime_execution", false)).append(".\n");
            sb.append("- Timing measurement claim: ").append(receipt.optBoolean("claim_allowed_timing_measurement", false)).append(".\n");
            sb.append("- Timer: ").append(receipt.optString("timer_clock", "UNAVAILABLE")).append(" / ")
                .append(receipt.optString("timer_unit", "UNAVAILABLE")).append(".\n");
            sb.append("- ELF SHA-256: ").append(emptyToUnavailable(receipt.optString("elf_sha256", ""))).append(".\n");
            sb.append("- Linker: ").append(emptyToUnavailable(receipt.optString("linker", ""))).append(".\n");
            sb.append("- Exit code: ").append(receipt.optInt("exit_code", -1)).append(".\n");
            sb.append("- Stdout truncated: ").append(receipt.optBoolean("stdout_truncated", false)).append(".\n");
            sb.append("- Environment: ").append(receipt.optString("environment_state", "NOT_MEASURED")).append(".\n");
            sb.append("- Severe thermal interference observed: ").append(receipt.optBoolean("thermal_interference_observed", false)).append(".\n");
            sb.append("- Series id: ").append(emptyToUnavailable(receipt.optString("series_id", ""))).append(".\n");
            sb.append("- Isolated silicon / reproducibility / cross-device / energy claims: **false**.\n\n");
        }

        sb.append("## 4. Governed series state\n\n");
        try {
            JSONObject series = PaBenchmarkSeriesAnalyzer.analyze(context);
            sb.append("- Analysis schema: ").append(series.optString("schema", "UNKNOWN")).append(".\n");
            sb.append("- State: `").append(series.optString("state", "INVALIDATED")).append("`.\n");
            sb.append("- Reason: `").append(series.optString("reason", "UNKNOWN")).append("`.\n");
            sb.append("- Eligible governed receipts: ").append(series.optInt("eligible_governed_receipts", 0)).append(".\n");
            sb.append("- Ad-hoc timing receipts not promoted: ").append(series.optInt("ad_hoc_timing_receipts_not_promoted", 0)).append(".\n");
            sb.append("- Series count: ").append(series.optInt("series_count", 0)).append(".\n");
            sb.append("- Minimum governed distribution target: n>=").append(PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N).append(".\n");
            sb.append("- Heterogeneous workload pooling: forbidden.\n");
            sb.append("- Cross-series pooling: forbidden.\n\n");
        } catch (Throwable error) {
            sb.append("- State: `INVALIDATED`.\n");
            sb.append("- Series analyzer error: ").append(error.getClass().getSimpleName()).append(".\n\n");
        }

        sb.append("## 5. Evidence gap ledger\n\n");
        sb.append("| Evidence | State unless directly observed | Closure |\n");
        sb.append("|---|---|---|\n");
        sb.append("| PA physical execution | receipt-governed | execute packaged ELF and retain receipt |\n");
        sb.append("| PA protocol-v2 timer semantics | source-fixed / physical proof required | execute V2 payload and observe timer markers |\n");
        sb.append("| Explicit governed n>=30 series | TOKEN_VAZIO until completed | use `Run Governed 30-Trial Series` |\n");
        sb.append("| Timer read overhead | per V2 receipt | observe `TIMER CLOCK_MONOTONIC_NS OVERHEAD_MIN` |\n");
        sb.append("| DVFS pre/post | AVAILABLE/PARTIAL/UNAVAILABLE | capture per-trial sysfs visibility |\n");
        sb.append("| Thermal pre/post | OBSERVED_LIMITED/UNAVAILABLE | Android thermal status snapshots |\n");
        sb.append("| PMU cycles/instructions/cache/branch | TOKEN_VAZIO or UNAVAILABLE | capability probe + Simpleperf/perf route if permitted |\n");
        sb.append("| Sensor callback timing | TOKEN_VAZIO | callback timestamp receipt, not inventory metadata |\n");
        sb.append("| Calibrated energy | BLOCKED | calibrated instrument/API with accuracy contract |\n");
        sb.append("| Cross-device baseline | BLOCKED | same workload/version/input + baseline + uncertainty |\n");
        sb.append("| Composite score | BLOCKED_BY_DESIGN | versioned normalization + weights + uncertainty |\n\n");

        sb.append("## 6. Seven antiderivative directions\n\n");
        appendDirection(sb, "D1 Metrology", "clock source → unit → overhead → monotonicity → uncertainty");
        appendDirection(sb, "D2 Statistics", "raw trials → homogeneous series → robust/classical dispersion → interval → drift");
        appendDirection(sb, "D3 Environment", "duration → thermal/DVFS/battery/memory/scheduler context");
        appendDirection(sb, "D4 Provenance", "source/build → ELF hash → linker → process → stdout hash → receipt → claim");
        appendDirection(sb, "D5 Workload realism", "microkernel → subsystem stress → representative application workload");
        appendDirection(sb, "D6 Comparability", "same version/input/route/environment disclosure → baseline → uncertainty-aware comparison");
        appendDirection(sb, "D7 Publication", "schema + raw receipts + CI + physical proof + review + release artifact");

        sb.append("## 7. Statistical contract\n\n");
        sb.append("- Each R0…R5 workload is a separate metric family.\n");
        sb.append("- A governed series requires explicit `series_id`, `series_index` and declared target n>=30.\n");
        sb.append("- Ad-hoc history cannot become a governed n=30 series by accumulation.\n");
        sb.append("- Preserve all raw receipts; no silent warm-up deletion and no arbitrary outlier deletion.\n");
        sb.append("- Report n, mean, median, sample SD, CV, MAD, Q1, Q3, IQR, min/max and a declared mean interval.\n");
        sb.append("- Deterministic score/checksum drift invalidates series identity.\n");
        sb.append("- n>=30 enables only a distribution summary; it does not prove reproducibility, environmental stability or cross-device comparability.\n");
        sb.append("- Composite scores remain forbidden until baseline, normalization, weights and uncertainty are versioned.\n\n");

        sb.append("## 8. Environment contract\n\n");
        sb.append("- Thermal status is Android/framework evidence when available.\n");
        sb.append("- Battery temperature is explicitly `BATTERY_NOT_CPU_SOC`.\n");
        sb.append("- CPU frequency values are best-effort sysfs observations, not frequency locks.\n");
        sb.append("- Missing cpufreq/thermal fields remain UNAVAILABLE, never zero.\n");
        sb.append("- Severe thermal state is retained as interference evidence; samples are not silently deleted.\n");
        sb.append("- Framework sensor power values and battery metadata are not measured energy.\n\n");

        sb.append("## 9. Evidence states\n\n");
        sb.append("`PASS`, `FAIL`, `NOT_MEASURED`, `UNAVAILABLE`, `BLOCKED`, `INVALIDATED`, `OBSERVED_LIMITED`, `TOKEN_VAZIO`. ")
            .append("TOKEN_VAZIO is an auditable missing-evidence marker and is never converted to zero or PASS.\n\n");

        sb.append("## 10. Industrial release boundary\n\n");
        sb.append("A numerical result is eligible for an industrial-quality report only when artifact provenance, execution route, unit semantics, homogeneous repeated trials, environmental disclosure, raw receipts, uncertainty and the interpretation boundary are all present. ")
            .append("This V3 method borrows good-practice patterns from SPEC-style run rules, Android Micro/Macrobenchmark, Perfetto/Simpleperf, Google Benchmark, EEMBC/MLPerf-style workload disclosure and metrology/verification practice, but it does not claim certification or conformance to those programs.\n");
        return sb.toString();
    }

    private static void appendDirection(StringBuilder sb, String title, String chain) {
        sb.append("### ").append(title).append("\n\n");
        sb.append("- Reconstruction chain: `").append(chain).append("`.\n");
        sb.append("- Rule: missing evidence blocks only the claim that depends on it; it does not erase narrower direct evidence.\n\n");
    }

    private static String join(String[] values) {
        if (values == null || values.length == 0) return "UNAVAILABLE";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < values.length; i++) {
            if (i > 0) sb.append(", ");
            sb.append(values[i] == null ? "UNAVAILABLE" : values[i]);
        }
        return sb.toString();
    }

    private static String emptyToUnavailable(String value) {
        return value == null || value.trim().isEmpty() ? "UNAVAILABLE" : value;
    }

    private static String utcNow() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(new Date());
    }
}
