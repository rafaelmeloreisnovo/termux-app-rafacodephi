#!/usr/bin/env bash
# Run on user-authorized Android/Termux; no download, root, network or install.
set -euo pipefail
jar=${1:?usage: run_assistant_arena_art.sh assistant-arena-device.jar}
test -s "$jar" || { echo 'DEVICE_JAR_MISSING'; exit 20; }
command -v dalvikvm >/dev/null 2>&1 || { echo 'ART_NOT_RUN_DALVIKVM_MISSING'; exit 21; }
command -v sha256sum >/dev/null 2>&1 || { echo 'RECEIPT_NOT_RUN_SHA256SUM_MISSING'; exit 22; }
printf 'scope=ANDROID_ART_DEVICE_EXECUTION\nclaim_allowed=false\n'
if command -v getprop >/dev/null 2>&1; then
  printf 'abi=%s\n' "$(getprop ro.product.cpu.abi)"
  printf 'sdk=%s\n' "$(getprop ro.build.version.sdk)"
fi
sha256sum "$jar"
dalvikvm -cp "$jar" com.termux.rafacodephi.assistant.RafAssistantArenaV1Adversarial
dalvikvm -cp "$jar" com.termux.rafacodephi.assistant.RafAssistantArenaBenchmark
printf 'ANDROID_ART_DEVICE_VECTORS_PASS_SCOPED\n'
