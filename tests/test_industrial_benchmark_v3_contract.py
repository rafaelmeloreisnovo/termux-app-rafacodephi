from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_pa_protocol_v2_uses_monotonic_nanoseconds_on_arm32_and_aarch64() -> None:
    src = read("app/src/main/cpp/freestanding/raf_pa_core.c")
    assert "RAFCODEPHI-PA-ELF 00000002" in src
    assert "CLOCK_MONOTONIC_NS" in src
    assert "1000000000ull" in src
    assert 'register sl x8 __asm__("x8") = 113' in src
    assert 'register sl r7 __asm__("r7") = 263' in src
    assert "return ns_from_ts(&x);" in src
    assert "((u64)(u32)x.s << 32) | (u32)x.n" not in src


def test_pa_protocol_v2_separates_deterministic_identity_from_timing() -> None:
    src = read("app/src/main/cpp/freestanding/raf_pa_core.c")
    assert "SCORE DETERMINISTIC_WORKLOAD_ID TIMING_EXCLUDED" in src
    assert "timer_overhead_min_ns" in src
    assert "score = ((u64)cs << 32)" in src
    assert "^ ~elapsed_ns" not in src
    assert "^ ~cy" not in src


def test_arm32_hwcap_namespace_cannot_promote_sve_or_sve2() -> None:
    header = read("app/src/main/cpp/lowlevel/baremetal.h")
    native = read("app/src/main/cpp/lowlevel/baremetal.c")
    assert "#if defined(ARCH_ARM32)" in header
    assert "#define HWCAP_SVE 0UL" in header
    assert "#define HWCAP2_SVE2 0UL" in header
    assert "if (hwcap & HWCAP_SVE)" in native
    assert "if (hwcap2 & HWCAP2_SVE2)" in native


def test_receipt_v3_preserves_legacy_execution_but_gates_timing_and_series_claims() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/PaBenchmarkReceipt.java")
    assert "rafcodephi.pa-elf-runtime-receipt/v3" in src
    assert "pa_freestanding_elf_runtime_v2.json" in src
    assert "LEGACY_V1_EXECUTION_ONLY" in src
    assert "CALIBRATED_V2" in src
    assert "claim_allowed_runtime_execution" in src
    assert "claim_allowed_timing_measurement" in src
    assert "claim_allowed_series_membership" in src
    assert "claim_allowed_environment_stability" in src
    assert "claim_allowed_cross_device_comparison" in src
    assert "timing_contract_complete" in src
    assert "environment_before" in src
    assert "environment_after" in src
    assert "series_governed" in src
    assert "parseWorkloads" in src


def test_series_analyzer_requires_explicit_series_id_and_never_pools_heterogeneous_workloads() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/PaBenchmarkSeriesAnalyzer.java")
    assert "MIN_DISTRIBUTION_N = 30" in src
    assert "same_series_artifact_abi_linker_protocol_workload_ops_flags_only" in src
    assert 'report.put("heterogeneous_workload_pooling", false)' in src
    assert 'report.put("cross_series_pooling", false)' in src
    assert 'report.put("ad_hoc_promotion_to_series", false)' in src
    assert 'receipt.optString("series_id", "")' in src
    assert "DETERMINISTIC_SCORE_OR_CHECKSUM_DRIFT" in src
    assert "environment_complete_samples" in src
    assert "thermal_interference_samples" in src
    assert "claim_allowed_distribution_summary" in src
    assert 'report.put("claim_allowed_reproducibility", false)' in src
    assert 'report.put("claim_allowed_cross_device_comparison", false)' in src


def test_environment_snapshot_is_observation_not_energy_or_cpu_temperature_claim() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/BenchmarkEnvironmentSnapshot.java")
    assert "getCurrentThermalStatus" in src
    assert "scaling_cur_freq" in src
    assert "memory_avail_bytes" in src
    assert "BATTERY_NOT_CPU_SOC" in src
    assert 'out.put("claim_allowed_cpu_temperature", false)' in src
    assert 'out.put("claim_allowed_energy_measurement", false)' in src
    assert 'out.put("claim_allowed_pmu", false)' in src


def test_central_runner_binds_pre_post_environment_and_series_metadata() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/PaBenchmarkRunner.java")
    assert "BenchmarkEnvironmentSnapshot.capture(context)" in src
    assert "PaBenchmarkReceipt.recordExecution(" in src
    assert "seriesId, seriesIndex, seriesTargetN" in src
    assert "PROCESS_TIMEOUT_MS = 60_000L" in src
    assert "STDOUT_CAPTURE_LIMIT = 64 * 1024" in src


def test_activity_exposes_governed_30_trial_series_without_silent_warmup_or_outlier_deletion() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/BenchmarkMenuActivity.java")
    assert "Run Governed 30-Trial Series" in src
    assert "PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N" in src
    assert "NO_SILENT_WARMUP_NO_OUTLIER_DELETION" in src
    assert "Cancel Series After Current Trial" in src
    assert "PaBenchmarkRunner.runOnce(this, seriesId, index, target)" in src
    assert "PaBenchmarkSeriesAnalyzer.analyzeAndWrite(this)" in src
    assert "claim_allowed_reproducibility=false" in src
    assert "claim_allowed_cross_device_comparison=false" in src


def test_claim_matrix_keeps_seven_gates_independent_and_broad_claims_closed() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/BenchmarkClaimMatrix.java")
    for gate in [
        "EXECUTION_PROOF",
        "MEASUREMENT_VALIDITY",
        "SERIES_VALIDITY",
        "ENVIRONMENT_VALIDITY",
        "COMPARABILITY_VALIDITY",
        "ENERGY_VALIDITY",
        "PUBLICATION_VALIDITY",
    ]:
        assert gate in src
    assert 'out.put("gate_inheritance", false)' in src
    assert 'out.put("composite_score_allowed", false)' in src
    assert 'out.put("claim_allowed_cross_device_comparison", false)' in src
    assert 'out.put("claim_allowed_energy_measurement", false)' in src
    assert 'out.put("claim_allowed_public_benchmark_ranking", false)' in src
