#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

PROFILE="${RAF_BOOTSTRAP_PROFILE:-bridge}"
PACKAGE_NAME="${TERMUX_BOOTSTRAP_PACKAGE_NAME:-com.termux.rafacodephi}"
SOURCE_REPO="${RAFCODEPHI_REAL_PKG_REPO:-https://packages.termux.dev/apt/termux-main}"

case "$PROFILE" in
  bridge)
    RAFCODEPHI_REAL_PKG_BOOTSTRAP=false \
      bash scripts/build_rafaelia_bootstraps.sh
    ;;
  real-pkg)
    RAFCODEPHI_REAL_PKG_BOOTSTRAP=true \
      RAFCODEPHI_REAL_PKG_ARCH="${RAFCODEPHI_REAL_PKG_ARCH:-all}" \
      bash scripts/build_rafaelia_bootstraps.sh
    ;;
  *)
    echo "Unsupported RAF_BOOTSTRAP_PROFILE=$PROFILE (allowed: bridge, real-pkg)" >&2
    exit 2
    ;;
esac

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
materialize_one app/src/main/cpp/rewritten-bootstrap-aarch64.zip aarch64 "$PROFILE"
materialize_one app/src/main/cpp/rewritten-bootstrap-arm.zip arm "$PROFILE"

# The current real package builder is ARM-only. Non-ARM artifacts stay honest bridges.
materialize_one app/src/main/cpp/rewritten-bootstrap-i686.zip i686 bridge
materialize_one app/src/main/cpp/rewritten-bootstrap-x86_64.zip x86_64 bridge

mkdir -p build/reports
python3 - <<'PY'
from __future__ import annotations
import json
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
    reports.append(json.loads(output))
payload = {
    "schema": "rafcodephi-bootstrap-profile-matrix/v1",
    "claim_allowed": False,
    "release_allowed": False,
    "artifacts": reports,
}
Path("build/reports/bootstrap-profile-matrix.json").write_text(
    json.dumps(payload, sort_keys=True, indent=2) + "\n",
    encoding="utf-8",
)
print(json.dumps(payload, sort_keys=True, indent=2))
PY

echo "BOOTSTRAP_PROFILE_BUILD_PASS profile=$PROFILE package=$PACKAGE_NAME"
