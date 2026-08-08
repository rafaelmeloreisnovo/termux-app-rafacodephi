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


def test_receipt_v3_preserves_legacy_execution_but_gates_timing_claim() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/PaBenchmarkReceipt.java")
    assert "rafcodephi.pa-elf-runtime-receipt/v3" in src
    assert "pa_freestanding_elf_runtime_v2.json" in src
    assert "LEGACY_V1_EXECUTION_ONLY" in src
    assert "CALIBRATED_V2" in src
    assert "claim_allowed_runtime_execution" in src
    assert "claim_allowed_timing_measurement" in src
    assert "claim_allowed_cross_device_comparison" in src
    assert "timing_contract_complete" in src
    assert "parseWorkloads" in src


def test_series_analyzer_never_pools_heterogeneous_workloads() -> None:
    src = read("app/src/main/java/com/termux/app/benchmark/PaBenchmarkSeriesAnalyzer.java")
    assert "MIN_DISTRIBUTION_N = 30" in src
    assert "same_artifact_abi_linker_protocol_workload_ops_flags_only" in src
    assert "heterogeneous_workload_pooling" in src
    assert 'report.put("heterogeneous_workload_pooling", false)' in src
    assert "DETERMINISTIC_SCORE_OR_CHECKSUM_DRIFT" in src
    assert "claim_allowed_distribution_summary" in src
    assert 'report.put("claim_allowed_reproducibility", false)' in src
    assert 'report.put("claim_allowed_cross_device_comparison", false)' in src
