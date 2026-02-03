#!/usr/bin/env bash
# Lightweight repo smoke checks for CI/local use
set -euo pipefail

echo "[1] doctor (host tools)"
./rafaelia_env/tools/doctor.sh | head -n 80 || true

echo "[2] upstream parallel check"
./scripts/rafaelia_upstream_parallel_check.sh

echo "OK: smoke";
