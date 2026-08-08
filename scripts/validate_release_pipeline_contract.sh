#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

fail() { echo "❌ $*" >&2; exit 1; }

[[ -x scripts/prepare_bootstrap_env.sh ]] || fail "scripts/prepare_bootstrap_env.sh missing or not executable"
[[ -f .github/actions/rafaelia-android-setup/action.yml ]] || fail "central RAFAELIA Android setup action is missing"

grep -Eq 'prepare_bootstrap_env\.sh' scripts/build_release_artifacts.sh || fail "build_release_artifacts.sh must call prepare_bootstrap_env.sh"
grep -Eq 'prepare_bootstrap_env\.sh' scripts/build_apk_matrix.sh || fail "build_apk_matrix.sh must call prepare_bootstrap_env.sh"

grep -Eq 'rafaelia-android-setup' .github/workflows/rafaelia_pipeline.yml || fail "workflow must use centralized RAFAELIA Android setup action"
grep -Eq 'prepare_bootstrap_env\.sh --github-env --skip-android-preflight' .github/actions/rafaelia-android-setup/action.yml || fail "central setup action must prepare bootstrap exactly once after Android preflight"

# Final-gate invariants: the orchestrator's version task must exist, and any
# GitHub Actions path that still uses setup_android_toolchain.sh must receive
# the same rewritten bootstrap preparation before Gradle/NDK can reach .incbin.
grep -Eq 'tasks\.register\("printVersionName"\)' build.gradle || fail "root build.gradle must register :app:printVersionName"
grep -Eq ':app:printVersionName' .github/workflows/rafaelia_pipeline.yml || fail "RAFAELIA workflow must resolve version through :app:printVersionName"
grep -Eq 'GITHUB_ACTIONS.*true' scripts/setup_android_toolchain.sh || fail "setup_android_toolchain.sh must gate CI bootstrap preparation on GITHUB_ACTIONS"
grep -Eq 'prepare_bootstrap_env\.sh' scripts/setup_android_toolchain.sh || fail "setup_android_toolchain.sh must materialize rewritten bootstraps for CI builds"
grep -Eq -- '--github-env --skip-android-preflight' scripts/setup_android_toolchain.sh || fail "setup_android_toolchain.sh must avoid recursive Android preflight during bootstrap preparation"

if grep -En 'LOCAL_CFLAGS.*(fno-rtti|fno-exceptions)|fno-rtti|fno-exceptions' app/src/main/cpp/Android.mk | grep -q 'LOCAL_CFLAGS'; then
  fail "Android.mk LOCAL_CFLAGS must not include -fno-rtti or -fno-exceptions"
fi

grep -Eq 'TARGET_ARCH_ABI\),armeabi-v7a\)' app/src/main/cpp/Android.mk || fail "Android.mk must contain armeabi-v7a branch"
if ! python3 - <<'PYCHK'
from pathlib import Path
text = Path('app/src/main/cpp/Android.mk').read_text()
arm32_idx = text.find('ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)')
if arm32_idx < 0:
    raise SystemExit(1)
block = text[arm32_idx:text.find('endif', arm32_idx)]
if 'lowlevel/baremetal_asm.S' not in block or 'HAS_BM_NEON_ASM=1' not in block:
    raise SystemExit(2)

pa_idx = text.find('LOCAL_MODULE := raf_pa_core')
if pa_idx < 0:
    raise SystemExit(3)
pa_end = text.find('include $(BUILD_SHARED_LIBRARY)', pa_idx)
pa_block = text[pa_idx:pa_end]
pa_arm_idx = pa_block.find('ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)')
if pa_arm_idx < 0:
    raise SystemExit(4)
pa_arm_end = pa_block.find('endif', pa_arm_idx)
pa_arm_block = pa_block[pa_arm_idx:pa_arm_end]
if 'LOCAL_ARM_MODE := arm' not in pa_arm_block:
    raise SystemExit(5)
if 'freestanding/raf_pa_entry_arm32.S' not in pa_arm_block:
    raise SystemExit(6)
PYCHK
then
  fail "ARM32 native contracts drifted: baremetal NEON and raf_pa_core ARM syscall mode are mandatory"
fi
grep -Eq 'verifyBootstrapZipsPresent' app/build.gradle || fail "app/build.gradle must contain verifyBootstrapZipsPresent"
for v in AARCH64 ARM I686 X86_64; do
  grep -Eq "TERMUX_BOOTSTRAP_BLAKE3_${v}" app/build.gradle || fail "Missing TERMUX_BOOTSTRAP_BLAKE3_${v} reference in app/build.gradle"
done

echo "✅ validate_release_pipeline_contract.sh passed"
