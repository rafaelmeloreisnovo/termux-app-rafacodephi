from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_bootstrap_readiness_gate_is_single_fail_closed_read_only_runtime_and_profile_contract() -> None:
    gate = read("app/src/main/java/com/termux/app/BootstrapReadinessGate.java")

    for token in [
        'SCHEMA = "rafcodephi.bootstrap-readiness/v1"',
        'STATE_BLOCKED = "BLOCKED"',
        'TOKEN_VAZIO = "TOKEN_VAZIO"',
        'PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1"',
        'PROFILE_FILE = "BOOTSTRAP_PROFILE.json"',
        'PROFILE_READ_LIMIT = 64 * 1024',
        '"sh"',
        '"pkg"',
        '"apkmanager"',
        '"shellbash"',
        '"busybox-safe"',
        '"proot-safe"',
        'TermuxRuntimePaths.storageHomeDir()',
        'context.getPackageName().equals(profile.optString("package_name", ""))',
        'prefix.getAbsolutePath().equals(profile.optString("prefix", ""))',
        'expectedBootstrapArch().equals(profile.optString("arch", ""))',
        '!profile.optBoolean("claim_allowed", true)',
        '!profile.optBoolean("release_allowed", true)',
        'TOKEN_VAZIO.equals(profile.optString("device_validation", ""))',
        'profile.optJSONArray("required_entries")',
        'canonicalTarget.startsWith(canonicalPrefix)',
        'real_pkg_relocation_claim_allowed=',
        'claim_allowed_release=false',
    ]:
        assert token in gate

    assert 'observeRealExecutable(checks, "$PREFIX/bin/busybox"' in gate
    assert 'observeRealExecutable(checks, "$PREFIX/bin/proot"' in gate

    # Readiness must remain observation-only. Mutation belongs to installer/repair paths.
    assert ".mkdirs()" not in gate
    assert "Os.chmod" not in gate
    assert ".delete()" not in gate
    assert "setupBootstrapIfNeeded" not in gate


def test_wizard_and_benchmark_compatibility_entries_delegate_to_unified_implementations() -> None:
    wizard_entry = read("app/src/main/java/com/termux/app/activities/Android15WizardActivity.java")
    wizard_impl = read("app/src/main/java/com/termux/app/activities/BetaBootstrapWizardActivity.java")
    benchmark_entry = read("app/src/main/java/com/termux/app/benchmark/BenchmarkMenuActivity.java")

    assert "extends BetaBootstrapWizardActivity" in wizard_entry
    assert "BootstrapReadinessGate.evaluate(this).isPass()" in wizard_impl
    assert "Install / Repair Real Bootstrap" in wizard_impl
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


def test_control_center_is_single_operator_surface_with_run_all_runtime_vectra_and_zip_export() -> None:
    ui = read("app/src/main/java/com/termux/app/activities/BetaOrchestratorActivity.java")
    prefs = read("app/src/main/res/xml/root_preferences.xml")

    for token in [
        "CheckBox",
        "RAFCODEΦ · Control Center",
        "Bootstrap / Package Readiness",
        "Package Runtime + Vectra Snapshot",
        "Require bootstrap readiness",
        "Capture package + Vectra runtime snapshot",
        "Execute one PA observation",
        "Run governed 30-trial series",
        "Analyze governed receipt history",
        "Export industrial V3 methods/gap artifact",
        "RUN SELECTED",
        "RUN EVERYTHING",
        "STOP AFTER CURRENT ATOMIC STAGE",
        "EXPORT ALL EVIDENCE (.ZIP)…",
        "REFRESH ALL STATUS",
        "OPEN BOOTSTRAP / PERMISSIONS WIZARD",
        "OPEN FULL VECTRA DETAILS",
        "ControlCenterSnapshot.render(this)",
        "ControlCenterEvidenceBundle.write(this, stream, bootstrap, runtime, latest)",
        "Intent.ACTION_CREATE_DOCUMENT",
        'intent.setType("application/zip")',
        "Intent.EXTRA_TITLE",
        'getContentResolver().openFileDescriptor(target, "rwt")',
        "stream.getFD().sync()",
        "EXPORT_ALL=PASS",
        "EXPORT_ALL_AUTHORITY=copy_only canonical_receipts_remain_authoritative",
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
        "exportAll.setEnabled(!running)",
    ]:
        assert token in ui

    assert 'app:key="rafcodephi_control_center"' in prefs
    assert 'app:title="RAFCODEΦ · Control Center"' in prefs
    assert 'android:targetClass="com.termux.app.activities.BetaOrchestratorActivity"' in prefs

    # The old operational screen fan-out is intentionally removed from the primary settings surface.
    for obsolete_key in [
        'app:key="android15_wizard"',
        'app:key="system_audit"',
        'app:key="industrial_diagnostics"',
        'app:key="pa_freestanding_elf"',
        'app:key="vectra_runtime"',
    ]:
        assert obsolete_key not in prefs


def test_control_center_export_is_evidence_scoped_not_an_arbitrary_home_backup() -> None:
    bundle = read("app/src/main/java/com/termux/app/orchestration/ControlCenterEvidenceBundle.java")
    snapshot = read("app/src/main/java/com/termux/app/orchestration/ControlCenterSnapshot.java")

    for token in [
        "rafcodephi.control-center-export/v1",
        "bootstrap-readiness.txt",
        "runtime-vectra-snapshot.txt",
        "latest-orchestrator-receipt.json",
        'new File(context.getFilesDir(), "rafcodephi-beta-orchestrator")',
        'context.getExternalFilesDir("beta-evidence")',
        "unrelated_termux_home_files=NOT_EXPORTED",
        "evidence_path_escape",
        "unsafe_zip_entry",
        "export_authority=copy_only",
        "claim_allowed_release=false",
    ]:
        assert token in bundle

    for token in [
        'String[] tools = {"sh", "pkg", "apt", "apt-get", "dpkg", "bash", "busybox", "proot"}',
        '"var/lib/dpkg/status"',
        '"etc/apt/sources.list.d"',
        '"BOOTSTRAP_PROFILE.json"',
        '"package_repo_runtime_state"',
        '"apt_repository_url"',
        '"package_runtime_gate="',
        '[VECTRA_RUNTIME]',
        'Sensor.TYPE_ACCELEROMETER',
        'Sensor.TYPE_GYROSCOPE',
        'Sensor.TYPE_MAGNETIC_FIELD',
        'Sensor.TYPE_LIGHT',
        'Sensor.TYPE_PROXIMITY',
    ]:
        assert token in snapshot


def test_full_beta_does_not_self_promote_to_certification_or_release() -> None:
    ui = read("app/src/main/java/com/termux/app/activities/BetaOrchestratorActivity.java")
    engine = read("app/src/main/java/com/termux/app/orchestration/BetaEvidenceOrchestrator.java")

    assert "claim_allowed_release=false" in ui
    assert "Fail-closed: TOKEN_VAZIO/UNAVAILABLE/BLOCKED never become PASS" in ui
    assert "Device/release claims remain independent" in ui
    assert 'finish(context, receipt, "OBSERVED_LIMITED"' in engine
    assert 'receipt.put("claim_allowed_release", false)' in engine
    assert 'receipt.put("claim_allowed_certification", false)' in engine
    assert "certified=true" not in engine.lower()
    assert "release_allowed=true" not in engine.lower()
