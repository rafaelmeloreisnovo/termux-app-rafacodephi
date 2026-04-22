#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

current_branch="$(git rev-parse --abbrev-ref HEAD)"
echo "Branch: $current_branch"

if git remote get-url upstream >/dev/null 2>&1; then
  echo "Upstream remote: $(git remote get-url upstream)"
else
  echo "⚠️ upstream remote not configured"
  exit 0
fi

if git fetch upstream --quiet; then
  echo "Fetched upstream"
else
  echo "⚠️ unable to fetch upstream"
  exit 0
fi

if git show-ref --verify --quiet refs/remotes/upstream/master; then
  target="upstream/master"
elif git show-ref --verify --quiet refs/remotes/upstream/main; then
  target="upstream/main"
else
  echo "⚠️ upstream main/master not found"
  exit 0
fi

read -r ahead behind < <(git rev-list --left-right --count "$target...HEAD")
echo "Divergence vs $target: ahead=$behind behind=$ahead"
