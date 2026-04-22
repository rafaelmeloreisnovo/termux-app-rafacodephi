#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LOCK_FILE="$ROOT_DIR/external/linuxkernel.lock"
TARGET_DIR="${1:-$ROOT_DIR/external/linuxkernel-src}"

if [[ ! -f "$LOCK_FILE" ]]; then
  echo "❌ lock file not found: $LOCK_FILE"
  exit 1
fi

# shellcheck disable=SC1090
source "$LOCK_FILE"

if [[ -z "${REPO_URL:-}" || -z "${COMMIT:-}" ]]; then
  echo "❌ lock file must define REPO_URL and COMMIT"
  exit 1
fi

mkdir -p "$(dirname "$TARGET_DIR")"

if [[ -d "$TARGET_DIR/.git" ]]; then
  echo "Updating existing checkout: $TARGET_DIR"
  git -C "$TARGET_DIR" remote set-url origin "$REPO_URL"
  git -C "$TARGET_DIR" fetch --depth 1 origin "$COMMIT"
else
  echo "Cloning sparse checkout from $REPO_URL"
  git clone --filter=blob:none --no-checkout "$REPO_URL" "$TARGET_DIR"
fi

if [[ -n "${SPARSE_PATHS:-}" ]]; then
  git -C "$TARGET_DIR" sparse-checkout init --cone
  # shellcheck disable=SC2206
  paths=( $SPARSE_PATHS )
  git -C "$TARGET_DIR" sparse-checkout set "${paths[@]}"
fi

git -C "$TARGET_DIR" checkout --force "$COMMIT"

checked_out_commit="$(git -C "$TARGET_DIR" rev-parse HEAD)"
echo "✅ linuxkernel synced at $checked_out_commit"
