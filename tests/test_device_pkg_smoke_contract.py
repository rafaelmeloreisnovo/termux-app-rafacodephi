from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/device_pkg_smoke.sh"
RUNBOOK = ROOT / "docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md"
TRUTH_TABLE = ROOT / "docs/RUNTIME_TRUTH_TABLE.md"


def test_device_pkg_smoke_separates_minimal_layer_from_real_pkg() -> None:
    text = SCRIPT.read_text(encoding="utf-8")

    for token in (
        "MINIMAL_PKG_LAYER=PASS",
        "DEVICE_LOCAL_PKG_CANDIDATE_VALIDATED",
        "REQUIRE_REAL_PKG",
        "DEVICE_REAL_PKG_VALIDATED",
        "pkg update -y",
        "pkg install -y nano",
        "pkg install -y python",
        "pkg install -y git",
        "reports/device_pkg_smoke.json",
        "reports/device_pkg_smoke.md",
        "reports/device_pkg_smoke.log",
    ):
        assert token in text


def test_device_pkg_smoke_keeps_minimal_commands_as_gate() -> None:
    text = SCRIPT.read_text(encoding="utf-8")

    for token in (
        "check cat_help cat --help",
        'check ls_home ls "$HOME"',
        "check clear clear",
        "grep x /dev/null",
        "check pkg_help pkg help",
        "check apt_help apt help",
        'check proot_real "$PREFIX/bin/proot.real" --version',
        '"$PREFIX/var/lib/rafcodephi/repo/dists/stable/Release"',
    ):
        assert token in text


def test_runbook_mentions_pkg_promotion_gate() -> None:
    text = RUNBOOK.read_text(encoding="utf-8")

    for token in (
        "scripts/device_pkg_smoke.sh",
        "REQUIRE_REAL_PKG=true",
        "DEVICE_REAL_PKG_VALIDATED",
        "pkg update -y",
        "pkg install -y nano",
        "pkg install -y python",
        "pkg install -y git",
    ):
        assert token in text


def test_truth_table_keeps_real_pkg_unproved_until_device_smoke() -> None:
    text = TRUTH_TABLE.read_text(encoding="utf-8")

    assert "device pkg smoke" in text
    assert "DEVICE_REAL_PKG_VALIDATED" in text

    rows: dict[str, str] = {}
    for line in text.splitlines():
        if not line.startswith("|") or line.startswith("|---"):
            continue
        columns = [column.strip() for column in line.strip("|").split("|")]
        if len(columns) < 2 or columns[0] == "Recurso":
            continue
        rows[columns[0]] = columns[1]

    expected_states = {
        "`pkg update`": "FUTURO",
        "`pkg install`": "FUTURO",
        "`apt`": "PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE",
        "`dpkg`": "PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE",
    }
    for resource, expected_state in expected_states.items():
        assert rows.get(resource) == expected_state
