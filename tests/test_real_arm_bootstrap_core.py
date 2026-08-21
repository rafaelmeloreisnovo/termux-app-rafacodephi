from pathlib import Path
import json
import subprocess
import sys
import zipfile
ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / 'scripts/build_real_arm_bootstrap_core.py'
VALIDATOR = ROOT / 'scripts/validate_real_arm_bootstrap_core.py'
BRIDGE = ROOT / 'scripts/rafcodephi_packages_bridge.sh'
PIN = ROOT / 'data/contracts/termux-packages-rafcodephi-pin.v1.json'
BOOTSTRAP_BUILD = ROOT / 'scripts/build_rafaelia_bootstraps.sh'
RUNBOOK = ROOT / 'docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md'
TRUTH_TABLE = ROOT / 'docs/RUNTIME_TRUTH_TABLE.md'
DEPRECATED_MAGIC_PIN = '7a26629938452c6d6fd80cf3fccce8c2056aabac'


def test_real_arm_core_builder_declares_required_payload_and_arches():
    text = BUILDER.read_text(encoding='utf-8')
    for token in [
        'CORE_PACKAGES', '"apt"', '"bash"', '"busybox"', '"ca-certificates"',
        '"coreutils"', '"dpkg"', '"findutils"', '"gawk"', '"grep"', '"gzip"',
        '"ncurses-utils"', '"proot"', '"sed"', '"tar"', '"termux-tools"',
        'ARCHES = ("aarch64", "arm")', 'BOOTSTRAP_REAL_APT_READY=1',
        'BOOTSTRAP_REAL_DPKG_READY=1', 'BOOTSTRAP_REAL_PROOT_READY=1',
        'BOOTSTRAP_REAL_COREUTILS_READY=1', 'BOOTSTRAP_CA_CERTIFICATES_READY=1',
        'BOOTSTRAP_DNS_RESOLVER_READY=1', 'BOOTSTRAP_MINIMUM_COMMANDS_READY=1',
        'proot.real', 'sources.list', 'resolv.conf'
    ]:
        assert token in text


def test_real_arm_core_builder_verifies_hashes_rewrites_prefix_and_fills_commands():
    text = BUILDER.read_text(encoding='utf-8')
    for token in [
        'verify_sha256', 'hashlib.sha256', 'dependency_closure', 'parse_depends',
        'rewrite_text_file(path, "/data/data/com.termux/files/usr", current_prefix)',
        'SYMLINKS.txt', 'MINIMUM_COMMANDS', 'ensure_minimum_commands',
        'write_command_wrapper', '$PREFIX/bin/busybox', 'exec /system/bin/toybox',
    ]:
        assert token in text


def test_main_bootstrap_build_keeps_real_pkg_opt_in_and_validates_when_enabled():
    text = BOOTSTRAP_BUILD.read_text(encoding='utf-8')
    for token in [
        'RAFCODEPHI_REAL_PKG_BOOTSTRAP:=false',
        'RAFCODEPHI_REAL_PKG_ARCH:=all',
        'python3 scripts/build_real_arm_bootstrap_core.py',
        'python3 scripts/validate_real_arm_bootstrap_core.py',
        'rewritten-bootstrap-aarch64.zip',
        'rewritten-bootstrap-arm.zip',
        'bridge-only ARM bootstraps. This is not a real pkg build.',
    ]:
        assert token in text


def test_real_arm_core_validator_blocks_missing_runtime_contract():
    text = VALIDATOR.read_text(encoding='utf-8')
    for token in [
        'bin/apt', 'bin/apt-get', 'bin/dpkg', 'bin/pkg', 'bin/proot.real',
        'bin/cat', 'bin/ls', 'bin/clear', 'bin/grep', 'etc/apt/sources.list',
        'etc/resolv.conf', 'BOOTSTRAP_REAL_COREUTILS_READY=1',
        'BOOTSTRAP_MINIMUM_COMMANDS_READY=1', 'unsafe zip entry', 'legacy prefix'
    ]:
        assert token in text


def test_real_arm_core_validator_audits_binary_legacy_prefix_risk():
    text = VALIDATOR.read_text(encoding='utf-8')
    assert 'LEGACY_PREFIX_BINARY_RISK' in text
    # Test the contract strings, not Python quote style.
    assert '/data/data/com.termux/files/usr' in text
    assert '/data/data/com.termux/' in text
    assert 'prefix in data' in text
    assert "replace(" not in text
    assert 'no binary replacement was performed' in text


def test_runbook_blocks_promotion_on_binary_prefix_risk():
    text = RUNBOOK.read_text(encoding='utf-8')
    assert 'python3 scripts/validate_real_arm_bootstrap_core.py' in text
    assert 'LEGACY_PREFIX_BINARY_RISK' in text
    assert 'bloqueia promoção' in text


def test_truth_table_keeps_real_package_stack_unproved():
    text = TRUTH_TABLE.read_text(encoding='utf-8')
    rows = {}
    for line in text.splitlines():
        if not line.startswith('|') or line.startswith('|---'):
            continue
        columns = [column.strip() for column in line.strip('|').split('|')]
        if len(columns) < 2 or columns[0] == 'Recurso':
            continue
        rows[columns[0]] = columns[1]

    payload_only = 'PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE'
    expected_states = {
        '`pkg` real': payload_only,
        '`apt`': payload_only,
        '`apt-get`': payload_only,
        '`dpkg`': payload_only,
        '`libapt`': 'TOKEN_VAZIO',
        '`proot`': payload_only,
        'certificados': payload_only,
        'DNS/network básico': 'TOKEN_VAZIO',
        'repositório binário custom': 'BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED',
    }
    for resource, expected_state in expected_states.items():
        assert rows.get(resource) == expected_state


def test_validator_rejects_binary_zip_entry_with_legacy_prefix(tmp_path):
    zip_path = tmp_path / 'bootstrap.zip'
    info = '\n'.join([
        'BOOTSTRAP_REAL_APT_READY=1',
        'BOOTSTRAP_REAL_DPKG_READY=1',
        'BOOTSTRAP_REAL_PROOT_READY=1',
        'BOOTSTRAP_REAL_COREUTILS_READY=1',
        'BOOTSTRAP_CA_CERTIFICATES_READY=1',
        'BOOTSTRAP_DNS_RESOLVER_READY=1',
        'BOOTSTRAP_MINIMUM_COMMANDS_READY=1',
    ]) + '\n'
    entries = {
        'bin/sh': b'#!/bin/sh\n',
        'bin/bash': b'#!/bin/sh\n',
        'bin/apt': b'#!/bin/sh\n',
        'bin/apt-get': b'#!/bin/sh\n',
        'bin/dpkg': b'#!/bin/sh\n',
        'bin/pkg': b'#!/bin/sh\n',
        'bin/cat': b'#!/data/data/com.termux.rafacodephi/files/usr/bin/sh\n',
        'bin/ls': b'#!/data/data/com.termux.rafacodephi/files/usr/bin/sh\n',
        'bin/clear': b'#!/data/data/com.termux.rafacodephi/files/usr/bin/sh\n',
        'bin/grep': b'#!/data/data/com.termux.rafacodephi/files/usr/bin/sh\n',
        'bin/proot': b'#!/data/data/com.termux.rafacodephi/files/usr/bin/sh\n',
        'bin/proot.real': b'\x7fELF\x00/data/data/com.termux/files/usr\x00',
        'etc/apt/sources.list': b'deb https://packages.termux.dev/apt/termux-main stable main\n',
        'etc/resolv.conf': b'nameserver 1.1.1.1\n',
        'etc/rafcodephi-core.env': b'TERMUX_PREFIX=/data/data/com.termux.rafacodephi/files/usr\n',
        'BOOTSTRAP_INFO': info.encode(),
        'SYMLINKS.txt': b'',
    }
    with zipfile.ZipFile(zip_path, 'w') as zf:
        for name, data in entries.items():
            zf.writestr(name, data)
    result = subprocess.run([sys.executable, str(VALIDATOR), str(zip_path)], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert result.returncode == 1
    assert 'LEGACY_PREFIX_BINARY_RISK' in result.stderr
    assert str(zip_path) in result.stderr
    assert 'entry=bin/proot.real' in result.stderr
    assert 'legacy_prefix=/data/data/com.termux/files/usr' in result.stderr


def test_bridge_targets_semantically_pinned_arm_core_package_set_including_termux_api():
    text = BRIDGE.read_text(encoding='utf-8')
    contract = json.loads(PIN.read_text(encoding='utf-8'))
    assert 'rafaelmeloreisnovo/termux-packages.git' in text
    assert 'resolve_termux_packages_pin.py' in text
    assert 'RAFCODEPHI_PACKAGES_CHANNEL:-canonical' in text
    assert DEPRECATED_MAGIC_PIN not in text
    assert contract['schema'] == 'rafcodephi.termux-packages-pin/v1'
    assert contract['channels']['canonical']['state'] == 'MERGED_BASELINE'
    assert contract['channels']['candidate']['claim_allowed'] is False
    assert 'REQUIRED_PACKAGES=(apt bash busybox proot dpkg ca-certificates coreutils termux-tools termux-api)' in text
    assert 'REQUIRED_ARCHES=(aarch64 arm)' in text


def test_runbook_requires_pkg_install_promotion_sequence():
    text = RUNBOOK.read_text(encoding='utf-8')
    for token in [
        'build_real_arm_bootstrap_core.py --arch all',
        'validate_real_arm_bootstrap_core.py',
        'DEVICE_SMOKE_REQUIRED=true',
        'pkg update -y',
        'pkg install -y nano',
        'pkg install -y python',
        'pkg install -y git',
    ]:
        assert token in text
