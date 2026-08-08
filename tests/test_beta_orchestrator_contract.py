from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_bootstrap_readiness_gate_is_single_fail_closed_runtime_contract() -> None:
    gate = read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")

    for token in [
        'SCHEMA = "rafcodephi.bootstrap-readiness/v1"',
        'STATE_BLOCKED = "BLOCKED"',
        'TOKEN_VAZIO = "TOKEN_VAZIO"',
        '"sh"',
        '"pkg"',
        '"apkmanager"',
        '"shellbash"',
        '"busybox-safe"',
        '"proot-safe"',
        'TermuxRuntimePaths.storageHomeDir()',
        'real_pkg_relocation_claim_allowed=',
        'claim_allowed_release=false',
    ]:
        assert token in gate

    assert 'observeOptionalExecutable(checks, "$PREFIX/bin/busybox"' in gate
    assert 'observeOptionalExecutable(checks, "$PREFIX/bin/proot"' in gate


def test_wizard_and_benchmark_compatibility_entries_delegate_to_unified_implementations() -> None:
    wizard_entry = read("app/src/main/java/com/termux/app/activities/Android15WizardActivity.java")
    wizard_impl = read("app/src/main/java/com/termux/app/activities/BetaBootstrapWizardActivity.java")
    benchmark_entry = read("app/src/main/java/com/termux/app/benchmark/BenchmarkMenuActivity.java")

    assert "extends BetaBootstrapWizardActivity" in wizard_entry
    assert "BootstrapReadinessGate.evaluate(this).isPass()" in wizard_impl
    assert "Install / Repair Bootstrap" in wizard_impl
    assert "Open Unified Beta Evidence Pipeline" in wizard_impl
    assert "extends BetaOrchestratorActivity" in benchmark_entry


def test_orchestrator_has_process_wide_single_flight_nonempty_plan_watchdog_cancel_atomic_receipt_and_export_semantics() -> None:
    engine = read("app/src/main/java/com/termux/app/orchestration/BetaEvidenceOrchestrator.java")

    required = [
        'SCHEMA = "rafcodephi.beta-evidence-orchestrator/v1"',
        'EXPORT_DIRECTORY = "beta-evidence"',
        "private static final AtomicBoolean PROCESS_RUNNING",
        "private static final AtomicBoolean PROCESS_CANCEL_REQUESTED",
        "!plan.hasSelectedAction()",
        "PROCESS_RUNNING.compareAndSet(false, true)",
        "PROCESS_CANCEL_REQUESTED.get()",
        "BootstrapReadinessGate.evaluate(context)",
        "PaBenchmarkRunner.runOnce(context)",
        "PaBenchmarkSeriesAnalyzer.MIN_DISTRIBUTION_N",
        "PaBenchmarkSeriesAnalyzer.analyzeAndWrite(context)",
        "IndustrialBenchmarkMethodology.write(context)",
        "cancelAfterCurrentAtomicStage",
        "AtomicFile",
        "context.getExternalFilesDir(EXPORT_DIRECTORY)",
        'receipt.put("external_export_state", "NOT_MEASURED")',
        'receipt.put("external_export_state", "UNAVAILABLE")',
        'receipt.put("external_export_state", "PASS")',
        'receipt.put("external_export_state", "FAIL")',
        'receipt.put("local_pipeline_completed", true)',
        'finish(context, receipt, "OBSERVED_LIMITED"',
        '"orchestration_execution_state"',
        '"empty_plan_allowed", false',
        '"single_flight_scope", "ANDROID_APP_PROCESS"',
        '"claim_allowed_release", false',
        '"claim_allowed_certification", false',
        '"claim_allowed_cross_device_comparison", false',
        '"publication_gate_state", "BLOCKED"',
        '"PaBenchmarkRunner_per_trial_timeout_60000ms"',
        '"no_gate_bypass_open_wizard_when_bootstrap_blocked"',
    ]
    for token in required:
        assert token in engine

    # A surviving canonical receipt defaults to NOT_MEASURED before any external mirror
    # can be promoted, so a crash between writes fails closed rather than inventing export success.
    conservative = engine.index('receipt.put("external_export_state", "NOT_MEASURED")')
    canonical_write = engine.index("atomicWrite(historyFile", conservative)
    external_probe = engine.index("context.getExternalFilesDir(EXPORT_DIRECTORY)", canonical_write)
    assert conservative < canonical_write < external_probe

    bootstrap = engine.index('emit(listener, "BOOTSTRAP_PREFLIGHT"')
    single = engine.index('if (plan.runSingleObservation)')
    series = engine.index('if (plan.runGovernedSeries)')
    analysis = engine.index('if (plan.analyzeHistory)')
    export = engine.index('if (plan.exportIndustrialMethods)')
    assert bootstrap < single < series < analysis < export


def test_operator_ui_has_comprehensible_checkboxes_export_paths_lifecycle_and_process_recovery() -> None:
    ui = read("app/src/main/java/com/termux/app/activities/BetaOrchestratorActivity.java")
    prefs = read("app/src/main/res/xml/root_preferences.xml")

    for token in [
        "CheckBox",
        "Require Bootstrap/Wizard readiness gate",
        "Execute one PA observation",
        "Run governed 30-trial series",
        "Analyze governed receipt history",
        "Export industrial V3 methods/gap artifact",
        "RUN SELECTED PIPELINE",
        "RUN FULL BETA EVIDENCE PIPELINE",
        "STOP AFTER CURRENT ATOMIC STAGE",
        "OPEN BOOTSTRAP / PERMISSIONS WIZARD",
        "OPEN VECTRA EXPERT DIAGNOSTICS",
        "REFRESH READINESS + PROCESS STATE + LATEST RECEIPT",
        "PROCESS_WIDE_PIPELINE=RUNNING",
        "SINGLE_FLIGHT_INVARIANT",
        "FINAL_EVIDENCE_STATE=",
        "ORCHESTRATION_EXECUTION_STATE=",
        "CANONICAL_RECEIPT=",
        "EXTERNAL_EXPORT_STATE=",
        "EXTERNAL_EXPORT_PATH=",
        "BetaEvidenceOrchestrator.readLatest(this)",
        "isFinishing() || isDestroyed()",
        "protected void onDestroy()",
        "orchestrator.cancelAfterCurrentAtomicStage()",
        "refresh.setEnabled(true)",
    ]:
        assert token in ui

    assert "RAFCODEΦ · First Beta Orchestrator" in prefs
    assert "optional governed n=30" in prefs


def test_full_beta_does_not_self_promote_to_certification_or_release() -> None:
    ui = read("app/src/main/java/com/termux/app/activities/BetaOrchestratorActivity.java")
    engine = read("app/src/main/java/com/termux/app/orchestration/BetaEvidenceOrchestrator.java")

    assert "claim_allowed_release=false" in ui
    assert "Certification/release/cross-device claims remain blocked" in ui
    assert "Local execution PASS is reported separately from evidence state" in ui
    assert 'finish(context, receipt, "OBSERVED_LIMITED"' in engine
    assert 'receipt.put("claim_allowed_release", false)' in engine
    assert 'receipt.put("claim_allowed_certification", false)' in engine
    assert "certified=true" not in engine.lower()
    assert "release_allowed=true" not in engine.lower()
