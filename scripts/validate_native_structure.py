#!/usr/bin/env python3
"""Valida consistência estrutural dos módulos nativos Android (NDK/JNI)."""

from __future__ import annotations

import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

EXPECTED_ABIS = ("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
EXPECTED_ABI_SET = set(EXPECTED_ABIS)
REQUIRES_16KB_ABIS = {"arm64-v8a", "x86_64"}
PAGE_SIZE_FLAG = "max-page-size=16384"


@dataclass
class NativeModule:
    name: str
    build_gradle: Path
    android_mk: Path
    application_mk: Path | None


@dataclass
class ValidationResult:
    errors: list[str]
    warnings: list[str]


def parse_settings_modules(settings_gradle: Path) -> list[str]:
    text = settings_gradle.read_text(encoding="utf-8")
    modules: list[str] = []
    for match in re.finditer(r"['\"]:([^'\"]+)['\"]", text):
        modules.append(match.group(1))
    return modules


def find_native_modules(repo_root: Path) -> list[NativeModule]:
    modules = parse_settings_modules(repo_root / "settings.gradle")
    native_modules: list[NativeModule] = []

    for module in modules:
        module_dir = repo_root / module
        build_gradle = module_dir / "build.gradle"
        if not build_gradle.exists():
            continue

        android_mk_candidates = [
            module_dir / "src/main/cpp/Android.mk",
            module_dir / "src/main/jni/Android.mk",
        ]
        android_mk = next((p for p in android_mk_candidates if p.exists()), None)
        if android_mk is None:
            continue

        app_mk_candidates = [
            android_mk.parent / "Application.mk",
            module_dir / "src/main/cpp/Application.mk",
            module_dir / "src/main/jni/Application.mk",
        ]
        application_mk = next((p for p in app_mk_candidates if p.exists()), None)

        native_modules.append(
            NativeModule(
                name=module,
                build_gradle=build_gradle,
                android_mk=android_mk,
                application_mk=application_mk,
            )
        )

    return native_modules


def _parse_abis_tokens(text: str) -> set[str]:
    quoted = re.findall(r"['\"]([A-Za-z0-9_\-]+)['\"]", text)
    return {abi for abi in quoted if abi in EXPECTED_ABI_SET}


def parse_gradle_abis(build_gradle_text: str) -> set[str]:
    abi_set: set[str] = set()

    for line in build_gradle_text.splitlines():
        if "abiFilters" in line:
            abi_set.update(_parse_abis_tokens(line))
        if re.search(r"\binclude\b", line) and "splits" in build_gradle_text:
            if any(abi in line for abi in EXPECTED_ABIS):
                abi_set.update(_parse_abis_tokens(line))

    return abi_set


def parse_app_mk_abis(application_mk_text: str) -> set[str]:
    for line in application_mk_text.splitlines():
        if "APP_ABI" not in line:
            continue
        value = re.split(r":=|=", line, maxsplit=1)
        if len(value) < 2:
            continue
        rhs = value[1].strip()
        if rhs == "all":
            return set(EXPECTED_ABIS)
        tokens = [tok.strip() for tok in re.split(r"\s+", rhs) if tok.strip()]
        return {tok for tok in tokens if tok in EXPECTED_ABI_SET}
    return set()


def parse_ndk_config_line(build_gradle_text: str) -> str | None:
    for line in build_gradle_text.splitlines():
        if "ndkVersion" in line:
            return line.strip()
    return None


def resolve_effective_ndk(repo_root: Path) -> tuple[str | None, str]:
    gradle_props = (repo_root / "gradle.properties").read_text(encoding="utf-8")
    match = re.search(r"^ndkVersion\s*=\s*([^\n\r]+)", gradle_props, flags=re.M)
    props_ndk = match.group(1).strip() if match else None
    env_ndk = os.environ.get("JITPACK_NDK_VERSION", "").strip()

    if env_ndk:
        return props_ndk, env_ndk
    return props_ndk, props_ndk or ""


def validate_module(module: NativeModule, effective_ndk: str) -> ValidationResult:
    errors: list[str] = []
    warnings: list[str] = []

    build_gradle_text = module.build_gradle.read_text(encoding="utf-8")
    android_mk_text = module.android_mk.read_text(encoding="utf-8")
    app_mk_text = module.application_mk.read_text(encoding="utf-8") if module.application_mk else ""

    gradle_abis = parse_gradle_abis(build_gradle_text)
    app_mk_abis = parse_app_mk_abis(app_mk_text) if module.application_mk else set()

    if gradle_abis and app_mk_abis and gradle_abis != app_mk_abis:
        errors.append(
            f"{module.name}: ABI mismatch entre build.gradle {sorted(gradle_abis)} e Application.mk {sorted(app_mk_abis)}"
        )

    resolved_abis = gradle_abis or app_mk_abis or set(EXPECTED_ABIS)

    unsupported = resolved_abis - EXPECTED_ABI_SET
    if unsupported:
        errors.append(f"{module.name}: ABIs não permitidas: {sorted(unsupported)}")

    if not resolved_abis:
        errors.append(f"{module.name}: não foi possível resolver ABIs permitidas")

    if REQUIRES_16KB_ABIS & resolved_abis and PAGE_SIZE_FLAG not in android_mk_text and PAGE_SIZE_FLAG not in build_gradle_text:
        errors.append(
            f"{module.name}: faltando flag crítica '{PAGE_SIZE_FLAG}' para ABIs 64-bit"
        )

    ndk_line = parse_ndk_config_line(build_gradle_text)
    if not ndk_line:
        errors.append(f"{module.name}: build.gradle sem configuração de ndkVersion")
    else:
        has_env_fallback = "JITPACK_NDK_VERSION" in ndk_line
        uses_project_ndk = "project.properties.ndkVersion" in ndk_line
        literal = re.findall(r"ndkVersion\s*=?\s*['\"]([^'\"]+)['\"]", ndk_line)

        if literal and literal[0] != effective_ndk:
            errors.append(
                f"{module.name}: ndkVersion literal '{literal[0]}' diverge da versão efetiva '{effective_ndk}'"
            )

        if not (has_env_fallback or uses_project_ndk or literal):
            warnings.append(f"{module.name}: ndkVersion usa expressão não reconhecida: {ndk_line}")

    print(f"[module:{module.name}] ABIs permitidas: {', '.join(sorted(resolved_abis))}")
    print(f"[module:{module.name}] Android.mk: {module.android_mk.relative_to(module.build_gradle.parents[1])}")
    if module.application_mk:
        print(f"[module:{module.name}] Application.mk: {module.application_mk.relative_to(module.build_gradle.parents[1])}")
    else:
        print(f"[module:{module.name}] Application.mk: (não encontrado)")

    return ValidationResult(errors=errors, warnings=warnings)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    native_modules = find_native_modules(repo_root)

    if not native_modules:
        print("❌ Nenhum módulo nativo encontrado (Android.mk)")
        return 1

    props_ndk, effective_ndk = resolve_effective_ndk(repo_root)
    if not effective_ndk:
        print("❌ Não foi possível determinar versão NDK efetiva")
        return 1

    print("=" * 72)
    print("Validação estrutural NDK/JNI")
    print("=" * 72)
    print(f"Módulos nativos detectados: {', '.join(module.name for module in native_modules)}")
    print(f"NDK gradle.properties: {props_ndk or '(ausente)'}")
    if os.environ.get("JITPACK_NDK_VERSION"):
        print(f"NDK efetiva (JITPACK_NDK_VERSION): {effective_ndk}")
    else:
        print(f"NDK efetiva (fallback project.properties.ndkVersion): {effective_ndk}")
    print("")

    errors: list[str] = []
    warnings: list[str] = []

    for module in native_modules:
        result = validate_module(module, effective_ndk=effective_ndk)
        errors.extend(result.errors)
        warnings.extend(result.warnings)
        print("")

    if warnings:
        print("⚠ Avisos:")
        for warning in warnings:
            print(f"  - {warning}")
        print("")

    if errors:
        print("❌ Erros de consistência estrutural:")
        for err in errors:
            print(f"  - {err}")
        return 1

    print("✅ Estrutura nativa consistente: ABIs, flags críticas e NDK efetiva validadas.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
