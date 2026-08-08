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

/** Generates a device-local, auditable benchmark methodology document. */
public final class IndustrialBenchmarkMethodology {

    public static final String FILE_NAME = "RAFCODEPHI_INDUSTRIAL_BENCHMARK_METHODS_V1.md";

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
        StringBuilder sb = new StringBuilder(16_384);
        sb.append("# RAFCODEΦ Industrial Benchmark Methods V1\n\n");
        sb.append("Generated: ").append(utcNow()).append("\n\n");
        sb.append("## 1. Scope and invariant\n\n");
        sb.append("This document defines a measurement procedure, not a certification. The invariant is: ")
            .append("a claim is permitted only when workload, input, unit, execution route, artifact identity and environmental evidence are bound to the same observation. ")
            .append("Different workloads are never pooled into a single reproducibility statistic merely because they share a timer unit.\n\n");
        sb.append("User-space benchmarking observes the combined behavior of silicon, firmware, kernel, scheduler, Android runtime, linker, compiler output and thermal/power policy. ")
            .append("It does not isolate silicon performance unless the necessary hardware counters and controls are directly available and recorded.\n\n");

        sb.append("## 2. Device under test (DUT)\n\n");
        sb.append("- Manufacturer: ").append(Build.MANUFACTURER).append("\n");
        sb.append("- Model: ").append(Build.MODEL).append("\n");
        sb.append("- Android: ").append(Build.VERSION.RELEASE).append(" (API ").append(Build.VERSION.SDK_INT).append(")\n");
        sb.append("- ABI set: ").append(Build.SUPPORTED_ABIS == null ? "UNAVAILABLE" : join(Build.SUPPORTED_ABIS)).append("\n");
        sb.append("- CPU cores visible to app: ").append(Runtime.getRuntime().availableProcessors()).append("\n\n");

        JSONObject receipt = PaBenchmarkReceipt.read(context);
        sb.append("## 3. Current PA execution evidence\n\n");
        if (receipt == null) {
            sb.append("State: NOT_MEASURED — no persisted PA runtime receipt exists for this installation.\n\n");
        } else {
            sb.append("- Runtime execution pass: ").append(receipt.optBoolean("runtime_exec_pass", false)).append("\n");
            sb.append("- Receipt timestamp: ").append(receipt.optString("generated_at_utc", "UNAVAILABLE")).append("\n");
            sb.append("- Linker: ").append(receipt.optString("linker", "UNAVAILABLE")).append("\n");
            sb.append("- ELF SHA-256: ").append(receipt.optString("elf_sha256", "UNAVAILABLE")).append("\n");
            sb.append("- Exit code: ").append(receipt.optInt("exit_code", -1)).append("\n");
            sb.append("- Evidence scope: ").append(receipt.optString("evidence_scope", "UNAVAILABLE")).append("\n\n");
        }

        sb.append("## 4. Seven production domains × seven controls\n\n");
        appendDomain(sb, "A. CPU / instruction execution",
            "Integer and fixed-width ALU throughput with declared operation count",
            "FP32/FP64 throughput with compiler flags and operation definition",
            "SIMD/NEON path versus scalar path using identical data",
            "Branch/control-flow workload with fixed branch distribution",
            "Single-thread versus multi-thread scaling",
            "Syscall transition cost separated from user-space compute",
            "Instruction-path artifact hash and ABI proof");
        appendDomain(sb, "B. Memory hierarchy",
            "Sequential read/write bandwidth by declared bytes touched",
            "Random access latency with fixed index sequence",
            "Copy/fill route identity: scalar, JNI arena, NEON or fallback",
            "Working-set sweep across multiple footprints",
            "Stride sweep to expose locality sensitivity",
            "Warm versus first-touch state reported separately",
            "Cache-line size recorded only when detector returns a positive value");
        appendDomain(sb, "C. Storage and durability",
            "Sequential read/write with exact fixture size and filesystem path",
            "Random 4 KiB I/O with operation count and seed",
            "Buffered versus explicitly synchronized write separated",
            "fsync/sync latency measured as its own metric",
            "Warm-cache and cache-unknown results labelled; no false cold-cache claim",
            "Free-space and storage-pressure preflight",
            "Fixture hash/size and cleanup outcome preserved");
        appendDomain(sb, "D. Kernel / scheduler / concurrency",
            "Clock-source identity and timer overhead calibration",
            "Scheduler interference snapshot before and after run",
            "Thread count, affinity availability and priority recorded",
            "Context-switch and synchronization workloads isolated",
            "GC/runtime interference recorded for Java-mediated tests",
            "Background-process or load warnings invalidate comparison when severe",
            "Execution-governance limits and queue pressure preserved");
        appendDomain(sb, "E. Virtualization / emulation",
            "Native host baseline separated from QEMU/emulated path",
            "Guest architecture and accelerator mode explicitly recorded",
            "Syscall/emulation transition cost isolated",
            "Memory-map/copy overhead isolated",
            "Timer precision and event-dispatch behavior measured separately",
            "Serialization/state-transition workload defined by bytes/operations",
            "Host/guest artifact and configuration hashes bound to result");
        appendDomain(sb, "F. Sensors / timing / edge runtime",
            "Sensor inventory distinguished from actual sample acquisition",
            "Sampling period requested versus timestamps observed",
            "Latency distribution measured from callbacks rather than declared preset",
            "Cancellation and timeout paths tested",
            "Unavailable hardware remains UNAVAILABLE, not zero",
            "Power metadata is framework-reported metadata, not measured energy",
            "Clock domain and timestamp provenance recorded");
        appendDomain(sb, "G. Integrity / build / provenance",
            "ELF/APK SHA-256 before result promotion",
            "Compiler, optimization flags and source commit recorded",
            "CRC/hash throughput measured with declared byte count",
            "Expected digest/result checked to prevent dead-code elimination",
            "Linker route and dynamic dependency contract inspected",
            "Receipt write is atomic and failure preserves prior evidence",
            "Claim state remains narrower than the strongest direct evidence");

        sb.append("## 5. Experimental procedure\n\n");
        sb.append("1. Define one falsifiable metric claim and its unit.\n");
        sb.append("2. Freeze the DUT identity, source commit, build flags and artifact hashes.\n");
        sb.append("3. Run preflight: ABI, page size, free storage, battery/thermal state, available memory, timer availability and execution path.\n");
        sb.append("4. Perform declared warm-up; do not silently mix warm-up samples with measured trials.\n");
        sb.append("5. Repeat the same workload with the same input. Preserve every raw sample before summarization.\n");
        sb.append("6. Capture post-run environment and invalidate a series when a declared interference gate is exceeded.\n");
        sb.append("7. Generate a receipt binding samples, artifact hashes, environment, exit status and interpretation boundary.\n\n");

        sb.append("## 6. Statistical contract\n\n");
        sb.append("- Statistics are computed per homogeneous metric/workload only.\n");
        sb.append("- Report n, median, mean, sample standard deviation (N-1 when n>1), MAD and IQR where useful.\n");
        sb.append("- Use Student-t confidence intervals for a small approximately normal repeated series; use a declared bootstrap procedure when distributional assumptions are not justified.\n");
        sb.append("- Coefficient of variation is allowed only for the same positive ratio-scale metric; never across unrelated workloads.\n");
        sb.append("- Outlier policy must be declared before interpretation; robust summaries are preferred to arbitrary deletion.\n");
        sb.append("- Cross-device comparison requires identical workload definition, software route and normalization contract.\n");
        sb.append("- A dimensionless composite score is forbidden unless its baseline, weights, uncertainty and aggregation rule are explicitly versioned.\n\n");

        sb.append("## 7. Evidence states\n\n");
        sb.append("- PASS: required observation and acceptance predicate both satisfied.\n");
        sb.append("- FAIL: observation exists and violates the predicate.\n");
        sb.append("- NOT_MEASURED: procedure has not run or no receipt exists.\n");
        sb.append("- UNAVAILABLE: platform does not expose the required capability.\n");
        sb.append("- BLOCKED: prerequisite prevents execution.\n");
        sb.append("- INVALIDATED: run occurred but interference/provenance makes interpretation unsafe.\n");
        sb.append("- OBSERVED_LIMITED: direct observation exists but supports only a narrower claim.\n\n");

        sb.append("## 8. Industrial release gate\n\n");
        sb.append("A benchmark result is eligible for an industrial-quality report only when: artifact provenance is complete; the execution route is observed; the metric definition is dimensionally coherent; repeated samples are homogeneous; environmental interference is bounded or disclosed; statistics match the data-generating process; raw evidence is retained; and every conclusion stays inside the documented claim boundary.\n\n");
        sb.append("References such as ISO/IEC 25010, IEEE verification/test documentation practices, SPEC-style workload disclosure, NIST measurement principles and MLPerf-style reproducibility can guide procedure design, but this file does not claim certification or conformance to any external program.\n");
        return sb.toString();
    }

    private static void appendDomain(StringBuilder sb, String title, String... controls) {
        sb.append("### ").append(title).append("\n\n");
        for (int i = 0; i < controls.length; i++) {
            sb.append(i + 1).append(". ").append(controls[i]).append(".\n");
        }
        sb.append("\n");
    }

    private static String join(String[] values) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < values.length; i++) {
            if (i > 0) sb.append(", ");
            sb.append(values[i]);
        }
        return sb.toString();
    }

    private static String utcNow() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("UTC"));
        return format.format(new Date());
    }
}
