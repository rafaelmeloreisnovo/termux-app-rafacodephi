from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = "com.termux.rafacodephi"
PREFIX = f"/data/data/{PACKAGE}/files/usr"
API_RECEIVER_COMPONENT = f"{PACKAGE}.api/com.termux.api.TermuxApiReceiver"


def elf_for_arch(arch: str, extra: bytes = b"") -> bytes:
    data = bytearray(64)
    data[:4] = b"\x7fELF"
    data[4] = 1 if arch == "arm" else 2
    data[5] = 1  # little endian
    data[6] = 1  # ELF version
    struct.pack_into("<H", data, 16, 2)  # ET_EXEC
    struct.pack_into("<H", data, 18, 40 if arch == "arm" else 183)
    data.extend(extra)
    return bytes(data)


def bootstrap_info(arch: str, overrides: dict[str, str] | None = None) -> bytes:
    values = {
        "TERMUX_PACKAGE_NAME": PACKAGE,
        "TERMUX_ARCH": arch,
        "RAFCODEPHI_BOOTSTRAP_PROFILE": "real-pkg",
        "RAFCODEPHI_PACKAGE_LAYER": "real-pkg",
        "RAFCODEPHI_DEVICE_VALIDATION": "TOKEN_VAZIO",
        "RAFCODEPHI_CLAIM_ALLOWED": "0",
        "BOOTSTRAP_FULLENGINE_READY": "0",
        "BOOTSTRAP_PKG_REAL": "1",
        "BOOTSTRAP_APT_REAL": "1",
        "BOOTSTRAP_DPKG_REAL": "1",
        "BOOTSTRAP_TERMUX_API_CLI": "1",
        "RAFCODEPHI_API_PACKAGE": f"{PACKAGE}.api",
        "RAFCODEPHI_API_RECEIVER_COMPONENT": API_RECEIVER_COMPONENT,
        "RAFCODEPHI_API_ACCESS_CONTROL": "SIGNATURE_PERMISSION_NO_SHARED_UID",
        "RAFCODEPHI_APT_UPDATE_GUARD": "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED",
    }
    values.update(overrides or {})
    return "".join(f"{key}={values[key]}\n" for key in sorted(values)).encode()


def make_zip(
    path: Path,
    *,
    arch: str = "arm",
    bridge: bool = False,
    legacy: bool = False,
    unsafe_repo: bool = False,
    info_overrides: dict[str, str] | None = None,
) -> None:
    required = [
        "BOOTSTRAP_INFO",
        "SYMLINKS.txt",
        "bin/sh",
        "bin/pkg",
        "bin/apt",
        "bin/apt-get",
        "bin/dpkg",
        "bin/bash",
        "bin/busybox",
        "bin/proot",
        "bin/termux-battery-status",
        "bin/termux-sensor",
        "libexec/termux-api",
        "libexec/termux-api-broadcast",
        "etc/apt/sources.list.d/termux.sources",
        "etc/apt/apt.conf.d/00rafcodephi-repository-block",
        "var/lib/dpkg/status",
    ]
    profile = {
        "schema": "rafcodephi-bootstrap-profile/v1",
        "profile": "real-pkg",
        "package_layer": "real-pkg",
        "package_name": PACKAGE,
        "prefix": PREFIX,
        "arch": arch,
        "api_package": f"{PACKAGE}.api",
        "api_receiver_component": API_RECEIVER_COMPONENT,
        "api_access_control": "SIGNATURE_PERMISSION_NO_SHARED_UID",
        "required_entries": required,
        "claim_allowed": False,
        "release_allowed": False,
        "device_validation": "TOKEN_VAZIO",
        "real_pkg_relocation_claim_allowed": False,
        "package_repo_runtime_state": "BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED",
        "apt_update_guard": "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED",
    }
    pkg = f"#!{PREFIX}/bin/bash\nexec {PREFIX}/bin/apt \"$@\"\n".encode()
    if bridge:
        pkg += b"# RAFCODEPHI pkg bridge\n"
    legacy_extra = b"/data/data/com.termux/files/usr" if legacy else b""

    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("BOOTSTRAP_INFO", bootstrap_info(arch, info_overrides))
        zf.writestr("BOOTSTRAP_PROFILE.json", json.dumps(profile, sort_keys=True))
        zf.writestr(
            "SYMLINKS.txt",
            "dash←bin/sh\ntermux-api-broadcast←libexec/termux-api\n",
        )
        zf.writestr("bin/dash", elf_for_arch(arch))
        zf.writestr("bin/pkg", pkg)
        zf.writestr("bin/apt", elf_for_arch(arch, legacy_extra))
        zf.writestr("bin/apt-get", elf_for_arch(arch))
        zf.writestr("bin/dpkg", elf_for_arch(arch))
        zf.writestr("bin/bash", elf_for_arch(arch))
        zf.writestr("bin/busybox", elf_for_arch(arch))
        zf.writestr("bin/proot", elf_for_arch(arch))
        zf.writestr("bin/termux-battery-status", b"#!/bin/sh\n")
        zf.writestr("bin/termux-sensor", b"#!/bin/sh\n")
        zf.writestr(
            "libexec/termux-api-broadcast",
            elf_for_arch(arch, API_RECEIVER_COMPONENT.encode()),
        )
        zf.writestr("lib/libapt-pkg.so.7.0", b"synthetic-test-lib")
        if unsafe_repo:
            sources = (
                b"Enabled: yes\nTypes: deb\nURIs: https://termux.net\n"
                b"Suites: stable\nComponents: main\n"
            )
        else:
            sources = (
                b"# RAFCODEPHI_PACKAGE_REPOSITORY=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED\n"
                b"Enabled: no\nTypes: deb\n"
                b"URIs: https://packages.rafcodephi.invalid/termux\n"
                b"Suites: stable\nComponents: main\n"
            )
        zf.writestr("etc/apt/sources.list.d/termux.sources", sources)
        zf.writestr(
            "etc/apt/apt.conf.d/00rafcodephi-repository-block",
            b'APT::Update::Pre-Invoke { "echo RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED >&2; exit 100"; };\n',
        )
        zf.writestr("var/lib/dpkg/status", b"Package: termux-api\nStatus: install ok installed\n")


def make_manifest(path: Path, zip_path: Path, *, arch: str = "arm", wrong_hash: bool = False) -> None:
    digest = hashlib.sha256(zip_path.read_bytes()).hexdigest()
    if wrong_hash:
        digest = "0" * 64
    path.write_text(
        "\n".join(
            [
                "schema=rafcodephi.real-bootstrap-sourcebuild/v1",
                f"package_name={PACKAGE}",
                f"prefix={PREFIX}",
                f"api_package={PACKAGE}.api",
                f"api_receiver_component={API_RECEIVER_COMPONENT}",
                "api_access_control=SIGNATURE_PERMISSION_NO_SHARED_UID",
                "bridge_allowed=false",
                "legacy_prefix_allowed=false",
                "termux_api_cli=EMBEDDED",
                "package_repo_runtime_state=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED",
                "apt_update_guard=RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED",
                f"sha256_{arch}={digest}",
                f"bytes_{arch}={zip_path.stat().st_size}",
                "claim_allowed_device_runtime=false",
                "device_runtime_proof=TOKEN_VAZIO",
                "",
            ]
        ),
        encoding="utf-8",
    )


def run_import(
    zip_path: Path,
    manifest: Path,
    dest: Path,
    receipt: Path,
    *,
    arch: str = "arm",
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts/import_rafcodephi_real_bootstrap.py"),
            "--arch",
            arch,
            "--zip",
            str(zip_path),
            "--manifest",
            str(manifest),
            "--dest",
            str(dest),
            "--receipt",
            str(receipt),
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def test_source_built_real_import_accepts_canonical_sh_symlink(tmp_path: Path) -> None:
    archive = tmp_path / "rafcodephi-bootstrap-arm.zip"
    manifest = tmp_path / "manifest.txt"
    dest = tmp_path / "rewritten-bootstrap-arm.zip"
    receipt = tmp_path / "receipt.json"
    make_zip(archive)
    make_manifest(manifest, archive)

    result = run_import(archive, manifest, dest, receipt)
    assert result.returncode == 0, result.stderr
    assert "rafcodephi_real_bootstrap_import=PASS" in result.stdout
    assert dest.read_bytes() == archive.read_bytes()
    doc = json.loads(receipt.read_text(encoding="utf-8"))
    assert doc["profile"] == "real-pkg"
    assert doc["bridge_allowed"] is False
    assert doc["device_runtime_proof"] == "TOKEN_VAZIO"
    assert doc["package_repo_runtime_state"] == "BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED"
    assert doc["apt_update_guard"] == "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED"

    profile_validation = subprocess.run(
        [
            sys.executable,
            str(ROOT / "tools/raf_bootstrap_profile.py"),
            "validate",
            "--zip",
            str(dest),
            "--expected-profile",
            "real-pkg",
            "--expected-arch",
            "arm",
            "--package-name",
            PACKAGE,
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert profile_validation.returncode == 0, profile_validation.stderr
    report = json.loads(profile_validation.stdout)
    assert report["structural_state"] == "PASS"
    assert report["device_validation"] == "TOKEN_VAZIO"


def test_source_built_real_import_accepts_aarch64_pair_member(tmp_path: Path) -> None:
    archive = tmp_path / "rafcodephi-bootstrap-aarch64.zip"
    manifest = tmp_path / "manifest.txt"
    dest = tmp_path / "rewritten-bootstrap-aarch64.zip"
    receipt = tmp_path / "receipt-aarch64.json"
    make_zip(archive, arch="aarch64")
    make_manifest(manifest, archive, arch="aarch64")

    result = run_import(archive, manifest, dest, receipt, arch="aarch64")
    assert result.returncode == 0, result.stderr
    doc = json.loads(receipt.read_text(encoding="utf-8"))
    assert doc["arch"] == "aarch64"
    assert doc["api_package"] == f"{PACKAGE}.api"
    assert doc["termux_api_cli"] == "EMBEDDED"
    assert doc["device_runtime_proof"] == "TOKEN_VAZIO"


def test_source_built_real_import_rejects_wrong_manifest_hash(tmp_path: Path) -> None:
    archive = tmp_path / "bootstrap.zip"
    manifest = tmp_path / "manifest.txt"
    make_zip(archive)
    make_manifest(manifest, archive, wrong_hash=True)
    result = run_import(archive, manifest, tmp_path / "dest.zip", tmp_path / "receipt.json")
    assert result.returncode != 0
    assert "SHA256 mismatch" in result.stderr


def test_source_built_real_import_rejects_bridge_pkg(tmp_path: Path) -> None:
    archive = tmp_path / "bootstrap.zip"
    manifest = tmp_path / "manifest.txt"
    make_zip(archive, bridge=True)
    make_manifest(manifest, archive)
    result = run_import(archive, manifest, tmp_path / "dest.zip", tmp_path / "receipt.json")
    assert result.returncode != 0
    assert "bridge marker" in result.stderr


def test_source_built_real_import_rejects_legacy_prefix_in_elf(tmp_path: Path) -> None:
    archive = tmp_path / "bootstrap.zip"
    manifest = tmp_path / "manifest.txt"
    make_zip(archive, legacy=True)
    make_manifest(manifest, archive)
    result = run_import(archive, manifest, tmp_path / "dest.zip", tmp_path / "receipt.json")
    assert result.returncode != 0
    assert "forbidden legacy prefix" in result.stderr


def test_source_built_real_import_rejects_active_upstream_repository(tmp_path: Path) -> None:
    archive = tmp_path / "bootstrap.zip"
    manifest = tmp_path / "manifest.txt"
    make_zip(archive, unsafe_repo=True)
    make_manifest(manifest, archive)
    result = run_import(archive, manifest, tmp_path / "dest.zip", tmp_path / "receipt.json")
    assert result.returncode != 0
    assert "custom-prefix apt repository is not deterministically disabled" in result.stderr


def test_source_built_real_import_rejects_bootstrap_info_identity_drift(tmp_path: Path) -> None:
    archive = tmp_path / "bootstrap.zip"
    manifest = tmp_path / "manifest.txt"
    make_zip(
        archive,
        info_overrides={"RAFCODEPHI_API_RECEIVER_COMPONENT": "wrong/.Receiver"},
    )
    make_manifest(manifest, archive)
    result = run_import(archive, manifest, tmp_path / "dest.zip", tmp_path / "receipt.json")
    assert result.returncode != 0
    assert "BOOTSTRAP_INFO contract mismatch RAFCODEPHI_API_RECEIVER_COMPONENT" in result.stderr


def test_build_and_wizard_contracts_expose_source_built_real_route() -> None:
    prepare = (ROOT / "scripts/prepare_bootstrap_env.sh").read_text(encoding="utf-8")
    wizard = (ROOT / "app/src/main/java/com/termux/app/BootstrapWizardSource.java").read_text(encoding="utf-8")
    importer = (ROOT / "scripts/import_rafcodephi_real_bootstrap.py").read_text(encoding="utf-8")

    for token in [
        "source-built-real",
        "RAF_REAL_BOOTSTRAP_ZIP_ARM",
        "RAF_REAL_BOOTSTRAP_ZIP_AARCH64",
        "RAF_REAL_BOOTSTRAP_MANIFEST",
        "import_rafcodephi_real_bootstrap.py",
        "bash scripts/build_bootstrap_profile.sh",
        "build/python-deps",
        "TERMUX_BOOTSTRAP_BLAKE3_ARM",
    ]:
        assert token in prepare

    for token in [
        "MAX_SYMLINKS_BYTES",
        "parseSymlinkDestinations",
        'symlinkDestinations.contains("bin/sh")',
        "BOOTSTRAP_REQUIRED_INSTALLED_ENTRIES_MISSING",
    ]:
        assert token in wizard

    assert '"machine_name": "EM_ARM"' in importer
    assert '"machine_name": "EM_AARCH64"' in importer
    assert "RAFCODEPHI pkg bridge" in importer
    assert "termux-api client does not target the RAFCODEPHI API receiver" in importer
    assert "RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED" in importer
    assert "custom-prefix apt repository is not deterministically disabled" in importer
    assert "device_runtime_proof" in importer
