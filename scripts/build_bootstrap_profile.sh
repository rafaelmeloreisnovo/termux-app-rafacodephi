#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

PROFILE="${RAF_BOOTSTRAP_PROFILE:-bridge}"
PACKAGE_NAME="${TERMUX_BOOTSTRAP_PACKAGE_NAME:-com.termux.rafacodephi}"
SOURCE_REPO="${RAFCODEPHI_REAL_PKG_REPO:-https://packages.termux.dev/apt/termux-main}"
REAL_PKG_ARCH="${RAFCODEPHI_REAL_PKG_ARCH:-all}"
SKIP_BUILD="${RAF_BOOTSTRAP_SKIP_BUILD:-false}"

case "$PROFILE" in
  bridge|real-pkg) ;;
  *)
    echo "Unsupported RAF_BOOTSTRAP_PROFILE=$PROFILE (allowed: bridge, real-pkg)" >&2
    exit 2
    ;;
esac
case "$REAL_PKG_ARCH" in
  all|aarch64|arm) ;;
  *)
    echo "Unsupported RAFCODEPHI_REAL_PKG_ARCH=$REAL_PKG_ARCH (allowed: all, aarch64, arm)" >&2
    exit 2
    ;;
esac
case "$SKIP_BUILD" in
  true|1|yes|on)
    SKIP_BUILD=true
    ;;
  false|0|no|off)
    SKIP_BUILD=false
    ;;
  *)
    echo "Unsupported RAF_BOOTSTRAP_SKIP_BUILD=$SKIP_BUILD (allowed: true/false)" >&2
    exit 2
    ;;
esac

embedded_archives=(
  app/src/main/cpp/rewritten-bootstrap-aarch64.zip
  app/src/main/cpp/rewritten-bootstrap-arm.zip
  app/src/main/cpp/rewritten-bootstrap-i686.zip
  app/src/main/cpp/rewritten-bootstrap-x86_64.zip
)

if [[ "$SKIP_BUILD" == "false" ]]; then
  case "$PROFILE" in
    bridge)
      RAFCODEPHI_REAL_PKG_BOOTSTRAP=false \
        bash scripts/build_rafaelia_bootstraps.sh
      ;;
    real-pkg)
      RAFCODEPHI_REAL_PKG_BOOTSTRAP=true \
        RAFCODEPHI_REAL_PKG_ARCH="$REAL_PKG_ARCH" \
        bash scripts/build_rafaelia_bootstraps.sh
      ;;
  esac
else
  # Same-observation path: a promotion gate may have already built and audited
  # these exact raw archives. Do not download/rebuild them again before adding
  # BOOTSTRAP_PROFILE.json; doing so would sever the audit -> candidate lineage.
  for archive in "${embedded_archives[@]}"; do
    [[ -s "$archive" ]] || {
      echo "RAF_BOOTSTRAP_SKIP_BUILD=true but audited embedded archive is missing/empty: $archive" >&2
      exit 1
    }
  done
  echo "BOOTSTRAP_PROFILE_REUSE_AUDITED_BYTES=true profile=$PROFILE real_pkg_arch=$REAL_PKG_ARCH"
fi

profile_for_arch() {
  local arch="$1"
  if [[ "$PROFILE" != "real-pkg" ]]; then
    printf 'bridge'
    return 0
  fi
  case "$REAL_PKG_ARCH:$arch" in
    all:aarch64|all:arm|aarch64:aarch64|arm:arm) printf 'real-pkg' ;;
    *) printf 'bridge' ;;
  esac
}

materialize_one() {
  local zip="$1"
  local arch="$2"
  local profile="$3"
  python3 tools/raf_bootstrap_profile.py materialize \
    --zip "$zip" \
    --profile "$profile" \
    --arch "$arch" \
    --package-name "$PACKAGE_NAME" \
    --source-repo "$SOURCE_REPO"
}

# The APK embeds rewritten-bootstrap-*.zip through termux-bootstrap-zip.S.
# Profile the exact embedded archive per architecture. An ARM-only real-pkg
# candidate must not mislabel the untouched AArch64 archive as real-pkg.
materialize_one app/src/main/cpp/rewritten-bootstrap-aarch64.zip aarch64 "$(profile_for_arch aarch64)"
materialize_one app/src/main/cpp/rewritten-bootstrap-arm.zip arm "$(profile_for_arch arm)"
materialize_one app/src/main/cpp/rewritten-bootstrap-i686.zip i686 bridge
materialize_one app/src/main/cpp/rewritten-bootstrap-x86_64.zip x86_64 bridge

mkdir -p build/reports
export RAF_BOOTSTRAP_PROFILE_MATRIX_REQUESTED_PROFILE="$PROFILE"
export RAF_BOOTSTRAP_PROFILE_MATRIX_REAL_PKG_ARCH="$REAL_PKG_ARCH"
export RAF_BOOTSTRAP_PROFILE_MATRIX_REUSED_AUDITED_BYTES="$SKIP_BUILD"
python3 - <<'PY'
from __future__ import annotations
import json
import os
from pathlib import Path
import subprocess

items = [
    ("aarch64", Path("app/src/main/cpp/rewritten-bootstrap-aarch64.zip")),
    ("arm", Path("app/src/main/cpp/rewritten-bootstrap-arm.zip")),
    ("i686", Path("app/src/main/cpp/rewritten-bootstrap-i686.zip")),
    ("x86_64", Path("app/src/main/cpp/rewritten-bootstrap-x86_64.zip")),
]
reports = []
for arch, path in items:
    output = subprocess.check_output(
        ["python3", "tools/raf_bootstrap_profile.py", "inspect", "--zip", str(path)],
        text=True,
    )
    report = json.loads(output)
    manifest = report.get("profile_manifest") or {}
    reports.append(report)
    if manifest.get("arch") != arch:
        raise SystemExit(f"embedded profile arch mismatch: expected={arch} actual={manifest.get('arch')}")

profile_by_arch = {
    report["profile_manifest"]["arch"]: report["profile_manifest"]["profile"]
    for report in reports
}
payload = {
    "schema": "rafcodephi-bootstrap-profile-matrix/v2",
    "requested_profile": os.environ.get("RAF_BOOTSTRAP_PROFILE_MATRIX_REQUESTED_PROFILE", "bridge"),
    "real_pkg_arch_request": os.environ.get("RAF_BOOTSTRAP_PROFILE_MATRIX_REAL_PKG_ARCH", "all"),
    "reused_audited_bytes": os.environ.get("RAF_BOOTSTRAP_PROFILE_MATRIX_REUSED_AUDITED_BYTES", "false") == "true",
    "embedded_profile_by_arch": profile_by_arch,
    "structural_state": "PASS",
    "device_validation": "TOKEN_VAZIO",
    "claim_allowed": False,
    "release_allowed": False,
    "artifacts": reports,
    "token_vazio": [
        {
            "id": "TV_BOOTSTRAP_DEVICE_INSTALL",
            "priority": "P0",
            "state": "TOKEN_VAZIO",
            "blocks": ["device_validation", "release_allowed"],
            "closure": "Install the exact hash-bound candidate APK on the target physical device and persist install/runtime provenance."
        },
        {
            "id": "TV_PKG_UPDATE",
            "priority": "P0",
            "state": "TOKEN_VAZIO",
            "blocks": ["claim_allowed_real_pkg_runtime"],
            "closure": "Run pkg update from the installed RAFCODEPhi prefix with exit=0 and retain stdout/stderr plus repository/TLS evidence."
        },
        {
            "id": "TV_PKG_INSTALL_SMOKE",
            "priority": "P0",
            "state": "TOKEN_VAZIO",
            "blocks": ["claim_allowed_real_pkg_runtime", "release_allowed"],
            "closure": "Install nano, python and git and execute version probes from the same installed prefix."
        },
        {
            "id": "TV_DNS_TLS_REPOSITORY",
            "priority": "P1",
            "state": "TOKEN_VAZIO",
            "blocks": ["repository_connectivity_claim"],
            "closure": "Persist DNS resolution, TLS/repository access and apt source identity from the physical runtime."
        },
        {
            "id": "TV_DUAL_ARM_PHYSICAL",
            "priority": "P2",
            "state": "TOKEN_VAZIO",
            "blocks": ["dual_arm_runtime_claim"],
            "closure": "Repeat the real-pkg runtime receipt on AArch64; ARMv7 evidence alone must not promote dual-ARM support."
        }
    ],
}
Path("build/reports/bootstrap-profile-matrix.json").write_text(
    json.dumps(payload, sort_keys=True, indent=2) + "\n",
    encoding="utf-8",
)
print(json.dumps(payload, sort_keys=True, indent=2))
PY

echo "BOOTSTRAP_PROFILE_BUILD_PASS profile=$PROFILE real_pkg_arch=$REAL_PKG_ARCH reused_audited_bytes=$SKIP_BUILD package=$PACKAGE_NAME"
