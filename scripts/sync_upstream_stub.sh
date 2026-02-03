#!/usr/bin/env bash
# Stub: how to keep fork parallel to upstream (run on your machine with git remotes).
set -euo pipefail

echo "Add upstream remote (example):"
echo "  git remote add upstream <upstream-url>"
echo "Fetch + rebase your branch onto upstream/master:"
echo "  git fetch upstream"
echo "  git checkout restructure-draft"
echo "  git rebase upstream/master"
echo
echo "Then re-apply your 3 deltas (docs in rafaelia_env/docs/UPSTREAM_PARALLEL.md)"
