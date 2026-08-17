#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="${RAF_LKG_ARTIFACT_DEST:-$ROOT/.ci-artifact-lkg/rafcodephi-bootstrap}"
OUTPUT_FILE="${GITHUB_OUTPUT:-/dev/stdout}"

emit_empty() {
  printf 'state=TOKEN_VAZIO\n' >> "$OUTPUT_FILE"
  printf 'source=NONE\n' >> "$OUTPUT_FILE"
}

if [[ -z "${GITHUB_REPOSITORY:-}" || -z "${GH_TOKEN:-${GITHUB_TOKEN:-}}" ]]; then
  echo "LKG artifact restore unavailable: GitHub repository/token not present."
  emit_empty
  exit 0
fi

if ! command -v gh >/dev/null 2>&1; then
  echo "LKG artifact restore unavailable: gh CLI not present."
  emit_empty
  exit 0
fi

export GH_TOKEN="${GH_TOKEN:-${GITHUB_TOKEN}}"
rm -rf "$DEST"
mkdir -p "$DEST"

runs="$(
  gh api \
    "/repos/${GITHUB_REPOSITORY}/actions/workflows/beta-build.yml/runs?status=success&per_page=30" \
    --jq '.workflow_runs[].id' 2>/dev/null || true
)"

if [[ -z "$runs" ]]; then
  echo "No successful Beta run is currently available as an artifact LKG source."
  emit_empty
  exit 0
fi

for run_id in $runs; do
  [[ "${GITHUB_RUN_ID:-}" == "$run_id" ]] && continue

  artifact_name="$(
    gh api \
      "/repos/${GITHUB_REPOSITORY}/actions/runs/${run_id}/artifacts?per_page=100" \
      --jq '.artifacts[] | select(.expired == false) | select(.name | startswith("rafcodephi-beta-real-artifacts-")) | .name' \
      2>/dev/null | head -n1 || true
  )"
  [[ -n "$artifact_name" ]] || continue

  tmp="$(mktemp -d "${TMPDIR:-/tmp}/rafcodephi-lkg-artifact.XXXXXX")"
  if ! gh run download "$run_id" \
        --repo "$GITHUB_REPOSITORY" \
        --name "$artifact_name" \
        --dir "$tmp" >/dev/null 2>&1; then
    rm -rf "$tmp"
    continue
  fi

  arm="$(find "$tmp" -type f -name 'rafcodephi-bootstrap-arm.zip' -print -quit)"
  arm64="$(find "$tmp" -type f -name 'rafcodephi-bootstrap-aarch64.zip' -print -quit)"
  manifest="$(find "$tmp" -type f -name 'RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt' -print -quit)"
  if [[ -z "$arm" || -z "$arm64" || -z "$manifest" ]]; then
    rm -rf "$tmp"
    continue
  fi

  cp "$arm" "$DEST/rafcodephi-bootstrap-arm.zip"
  cp "$arm64" "$DEST/rafcodephi-bootstrap-aarch64.zip"
  cp "$manifest" "$DEST/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt"

  if python3 "$ROOT/scripts/import_rafcodephi_real_bootstrap.py" \
        --arch arm \
        --zip "$DEST/rafcodephi-bootstrap-arm.zip" \
        --manifest "$DEST/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt" \
        --validate-only \
    && python3 "$ROOT/scripts/import_rafcodephi_real_bootstrap.py" \
        --arch aarch64 \
        --zip "$DEST/rafcodephi-bootstrap-aarch64.zip" \
        --manifest "$DEST/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt" \
        --validate-only; then
    sha256sum \
      "$DEST/rafcodephi-bootstrap-arm.zip" \
      "$DEST/rafcodephi-bootstrap-aarch64.zip" \
      "$DEST/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt" \
      > "$DEST/SHA256SUMS"
    head_sha="$(
      gh api "/repos/${GITHUB_REPOSITORY}/actions/runs/${run_id}" --jq '.head_sha' 2>/dev/null || true
    )"
    printf 'state=READY\n' >> "$OUTPUT_FILE"
    printf 'source=ACTIONS_ARTIFACT\n' >> "$OUTPUT_FILE"
    printf 'run_id=%s\n' "$run_id" >> "$OUTPUT_FILE"
    printf 'head_sha=%s\n' "${head_sha:-TOKEN_VAZIO}" >> "$OUTPUT_FILE"
    printf 'arm=%s\n' "$DEST/rafcodephi-bootstrap-arm.zip" >> "$OUTPUT_FILE"
    printf 'aarch64=%s\n' "$DEST/rafcodephi-bootstrap-aarch64.zip" >> "$OUTPUT_FILE"
    printf 'manifest=%s\n' "$DEST/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt" >> "$OUTPUT_FILE"
    echo "Validated LKG restored from successful Beta artifact run_id=$run_id head_sha=${head_sha:-TOKEN_VAZIO}"
    rm -rf "$tmp"
    exit 0
  fi

  rm -rf "$DEST"
  mkdir -p "$DEST"
  rm -rf "$tmp"
done

echo "No historical Beta artifact contained a consumable strictly validated ARM32+AArch64 pair."
emit_empty
exit 0
