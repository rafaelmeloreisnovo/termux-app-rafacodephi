#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

REPO_URL="${TERMUX_PACKAGES_RAF_REPO:-https://github.com/rafaelmeloreisnovo/termux-packages.git}"
REPO_REF="${TERMUX_PACKAGES_RAF_REF:-7a26629938452c6d6fd80cf3fccce8c2056aabac}"
OUT_DIR="${TERMUX_PACKAGES_RAF_MANIFEST_DIR:-dist/source-contract}"
REQUIRE_PINNED="${TERMUX_PACKAGES_RAF_REQUIRE_PINNED:-true}"

fail() { printf '[raf-packages-source] ERROR: %s\n' "$*" >&2; exit 1; }
info() { printf '[raf-packages-source] %s\n' "$*"; }

case "$REPO_URL" in
  https://github.com/rafaelmeloreisnovo/termux-packages|https://github.com/rafaelmeloreisnovo/termux-packages.git) ;;
  *) fail "TERMUX_PACKAGES_RAF_REPO must point to rafaelmeloreisnovo/termux-packages, got: ${REPO_URL}" ;;
esac

if [[ "$REQUIRE_PINNED" == "true" && ! "$REPO_REF" =~ ^[0-9a-f]{40}$ ]]; then
  fail "TERMUX_PACKAGES_RAF_REF must be a pinned 40-char commit for reproducible CI; got: ${REPO_REF}"
fi

github_commit_exists() {
  local repo_url="$1"
  local commit_sha="$2"

  python3 - "$repo_url" "$commit_sha" <<'PYAPI'
import json
import sys
import urllib.request

repo_url = sys.argv[1]
commit_sha = sys.argv[2]

repo = repo_url
repo = repo.removeprefix("https://github.com/")
repo = repo.removesuffix(".git")

api_url = f"https://api.github.com/repos/{repo}/commits/{commit_sha}"

req = urllib.request.Request(
    api_url,
    headers={
        "Accept": "application/vnd.github+json",
        "User-Agent": "rafcodephi-source-contract",
    },
)

try:
    with urllib.request.urlopen(req, timeout=30) as response:
        payload = json.loads(response.read().decode("utf-8"))
except Exception:
    sys.exit(1)

sys.exit(0 if payload.get("sha") == commit_sha else 1)
PYAPI
}

resolved=""
if [[ "$REPO_REF" =~ ^[0-9a-f]{40}$ ]]; then
  if git ls-remote --exit-code "$REPO_URL" "$REPO_REF" >/dev/null 2>&1; then
    resolved="$REPO_REF"
  elif git ls-remote "$REPO_URL" | awk '{print $1}' | grep -Fxq "$REPO_REF"; then
    resolved="$REPO_REF"
  elif github_commit_exists "$REPO_URL" "$REPO_REF"; then
    info "pinned commit exists in GitHub commit API but is not advertised by ls-remote"
    resolved="$REPO_REF"
  else
    fail "pinned commit not found in ${REPO_URL}: ${REPO_REF}"
  fi
else
  resolved="$(git ls-remote --exit-code "$REPO_URL" "$REPO_REF" | awk 'NR==1{print $1}')"
  [[ -n "$resolved" ]] || fail "unable to resolve ref ${REPO_REF} from ${REPO_URL}"
fi

mkdir -p "$OUT_DIR"
cat > "${OUT_DIR}/TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env" <<MANIFEST
TERMUX_PACKAGES_RAF_REPO=${REPO_URL}
TERMUX_PACKAGES_RAF_REF=${REPO_REF}
TERMUX_PACKAGES_RAF_RESOLVED_COMMIT=${resolved}
TERMUX_PACKAGES_RAF_ABIS=armeabi-v7a,arm64-v8a
TERMUX_PACKAGES_RAF_ROLE=source-built-packages-bootstrap-and-apt
TERMUX_PACKAGES_RAF_SOURCE_KIND=rafaelmeloreisnovo-termux-packages
TERMUX_PACKAGES_RAF_BINARIES_CREATED_BY_CI_ONLY=1
MANIFEST

info "repo=${REPO_URL}"
info "ref=${REPO_REF}"
info "resolved_commit=${resolved}"
info "manifest=${OUT_DIR}/TERMUX_PACKAGES_RAFCODEPHI_SOURCE.env"
