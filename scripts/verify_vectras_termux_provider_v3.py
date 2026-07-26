#!/usr/bin/env python3
"""Verify the Termux RAFCODE-Phi side of the Vectras IPC v3 contract."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = "raf.termux-run-command-provider-verification.v3"
FILES = {
    "app_build": ROOT / "app/build.gradle",
    "manifest": ROOT / "app/src/main/AndroidManifest.xml",
    "constants": ROOT / "termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java",
    "execution_command": ROOT / "termux-shared/src/main/java/com/termux/shared/shell/command/ExecutionCommand.java",
    "run_command_service": ROOT / "app/src/main/java/com/termux/app/RunCommandService.java",
    "plugin_utils": ROOT / "termux-shared/src/main/java/com/termux/shared/termux/plugins/TermuxPluginUtils.java",
    "result_sender": ROOT / "termux-shared/src/main/java/com/termux/shared/shell/command/result/ResultSender.java",
    "provider_contract": ROOT / "docs/contracts/VECTRAS_TERMUX_PROVIDER_V3.json",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    return parser.parse_args()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(text: str, snippets: list[str], label: str, errors: list[str]) -> None:
    for snippet in snippets:
        if snippet not in text:
            errors.append(f"{label}: missing {snippet!r}")


def parse_default_package(build: str) -> str | None:
    match = re.search(r'def\s+appPackageName\s*=\s*"([^"]+)"', build)
    return match.group(1) if match else None


def main() -> int:
    args = parse_args()
    errors: list[str] = []
    texts: dict[str, str] = {}
    inputs: dict[str, Any] = {}

    for label, path in FILES.items():
        if not path.is_file():
            errors.append(f"missing {label}: {path}")
            continue
        texts[label] = path.read_text(encoding="utf-8")
        inputs[label] = {"path": str(path.relative_to(ROOT)), "sha256": sha256_file(path)}

    contract: dict[str, Any] = {}
    if "provider_contract" in texts:
        try:
            contract = json.loads(texts["provider_contract"])
        except json.JSONDecodeError as error:
            errors.append(f"provider contract invalid JSON: {error}")

    app_build = texts.get("app_build", "")
    package_name = parse_default_package(app_build)
    if package_name != "com.termux.rafacodephi":
        errors.append(f"default package mismatch: {package_name}")

    manifest = texts.get("manifest", "")
    require(
        manifest,
        [
            'android:name="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND"',
            'android:protectionLevel="dangerous"',
            'android:name="com.termux.app.RunCommandService"',
            'android:exported="true"',
            'android:permission="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND"',
            'android:name="${TERMUX_PACKAGE_NAME}.RUN_COMMAND"',
        ],
        "manifest",
        errors,
    )

    constants = texts.get("constants", "")
    require(
        constants,
        [
            'RUN_COMMAND_SERVICE_NAME = TERMUX_PACKAGE_NAME + ".app.RunCommandService"',
            'RUN_COMMAND_PERMISSION = TERMUX_PACKAGE_NAME + ".permission.RUN_COMMAND"',
            'RUN_COMMAND_ACTION = TERMUX_PACKAGE_NAME + ".RUN_COMMAND"',
            'EXTRA_COMMAND_PATH = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_PATH"',
            'EXTRA_ARGUMENTS = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_ARGUMENTS"',
            'EXTRA_WORKDIR = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_WORKDIR"',
            'EXTRA_RUNNER = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RUNNER"',
            'EXTRA_PENDING_INTENT = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_PENDING_INTENT"',
            'EXTRA_PLUGIN_RESULT_BUNDLE = "result"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT = "stdout"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH = "stdout_original_length"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_STDERR = "stderr"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH = "stderr_original_length"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE = "exitCode"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_ERR = "err"',
            'EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG = "errmsg"',
        ],
        "constants",
        errors,
    )

    execution = texts.get("execution_command", "")
    require(
        execution,
        [
            'APP_SHELL("app-shell")',
            'TERMINAL_SESSION("terminal-session")',
            'public static Runner runnerOf(String runnerName)',
        ],
        "execution_command",
        errors,
    )

    service = texts.get("run_command_service", "")
    require(
        service,
        [
            'TermuxConstants.TERMUX_SERVICE.EXTRA_COMMAND_PATH',
            'TermuxConstants.TERMUX_SERVICE.EXTRA_ARGUMENTS',
            'TermuxConstants.TERMUX_SERVICE.EXTRA_WORKDIR',
            'TermuxConstants.TERMUX_SERVICE.EXTRA_RUNNER',
            'TermuxConstants.TERMUX_SERVICE.EXTRA_PENDING_INTENT',
            'ExecutionCommand.Runner.runnerOf',
            'TermuxFileUtils.getCanonicalPath',
            'new ExecutionCommand(',
            'ResultSender.sendCommandResultData',
        ],
        "run_command_service",
        errors,
    )

    plugin_utils = texts.get("plugin_utils", "")
    require(
        plugin_utils,
        [
            'resultConfig.resultBundleKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE',
            'resultConfig.resultStdoutKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT',
            'resultConfig.resultStdoutOriginalLengthKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH',
            'resultConfig.resultStderrKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR',
            'resultConfig.resultStderrOriginalLengthKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH',
            'resultConfig.resultExitCodeKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE',
            'resultConfig.resultErrCodeKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERR',
            'resultConfig.resultErrmsgKey = TermuxConstants.TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG',
        ],
        "plugin_utils",
        errors,
    )

    sender = texts.get("result_sender", "")
    require(
        sender,
        [
            'resultBundle.putString(resultConfig.resultStdoutKey',
            'resultBundle.putInt(resultConfig.resultStdoutOriginalLengthKey',
            'resultBundle.putString(resultConfig.resultStderrKey',
            'resultBundle.putInt(resultConfig.resultStderrOriginalLengthKey',
            'resultBundle.putInt(resultConfig.resultExitCodeKey',
            'resultBundle.putInt(resultConfig.resultErrCodeKey',
            'resultBundle.putString(resultConfig.resultErrmsgKey',
            'resultIntent.putExtra(resultConfig.resultBundleKey, resultBundle)',
        ],
        "result_sender",
        errors,
    )

    if contract:
        if contract.get("schema") != "raf.termux-run-command-provider.v3":
            errors.append("provider contract schema mismatch")
        if contract.get("claim_allowed") is not False:
            errors.append("provider contract claim_allowed must be false")
        provider = contract.get("provider", {})
        if provider.get("package_default") != package_name:
            errors.append("provider contract package does not match app build")
        expected_full = {
            "service_class": "com.termux.app.RunCommandService",
            "permission": f"{package_name}.permission.RUN_COMMAND",
            "action": f"{package_name}.RUN_COMMAND",
        }
        for key, expected in expected_full.items():
            if provider.get(key) != expected:
                errors.append(f"provider.{key} mismatch")
        result_keys = contract.get("result_keys", {})
        exact_result_keys = {
            "bundle": "result",
            "stdout": "stdout",
            "stdout_original_length": "stdout_original_length",
            "stderr": "stderr",
            "stderr_original_length": "stderr_original_length",
            "exit_code": "exitCode",
            "error_code": "err",
            "error_message": "errmsg",
        }
        for key, expected in exact_result_keys.items():
            if result_keys.get(key) != expected:
                errors.append(f"result_keys.{key} mismatch")

    state = "FAIL" if errors else "PASS_STATIC_PROVIDER_CONTRACT"
    report = {
        "schema": SCHEMA,
        "cycle_id": "C07",
        "state": state,
        "claim_allowed": False,
        "provider_package": package_name or "TOKEN_VAZIO",
        "inputs": inputs,
        "checks": {
            "dangerous_permission_declared": 'android:protectionLevel="dangerous"' in manifest,
            "service_exported_with_permission": (
                'android:name="com.termux.app.RunCommandService"' in manifest
                and 'android:permission="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND"' in manifest
            ),
            "pending_intent_result_supported": 'EXTRA_PENDING_INTENT' in service,
            "app_shell_runner_supported": 'APP_SHELL("app-shell")' in execution,
            "internal_error_separate_from_exit_code": (
                'resultConfig.resultExitCodeKey' in sender
                and 'resultConfig.resultErrCodeKey' in sender
            ),
            "truncation_metadata_supported": (
                'resultStdoutOriginalLengthKey' in sender
                and 'resultStderrOriginalLengthKey' in sender
            ),
        },
        "runtime_boundary": {
            "android_build": "TOKEN_VAZIO",
            "provider_installed": "TOKEN_VAZIO",
            "permission_granted": "TOKEN_VAZIO",
            "real_result_bundle": "TOKEN_VAZIO",
            "qemu_execution": "TOKEN_VAZIO",
            "guest_boot": "TOKEN_VAZIO",
        },
        "errors": errors,
        "falsifiers": [
            "application_id_changed",
            "run_command_permission_or_action_changed",
            "result_key_changed",
            "pending_intent_support_removed",
            "stdout_or_stderr_original_length_removed",
            "internal_error_collapsed_into_process_exit_code",
            "provider_static_pass_promoted_to_device_execution",
        ],
    }

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"state": state, "error_count": len(errors)}, sort_keys=True))
    if errors:
        for error in errors:
            print(f"FAIL: {error}")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
