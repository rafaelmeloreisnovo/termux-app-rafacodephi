#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_PATH="${1:-${ROOT_DIR}/build/reports/toolchain-startup.json}"
EXPECTED_NODE_MAJOR="${RAFCODEPHI_EXPECT_NODE_MAJOR:-24}"
EXPECTED_JAVA_MAJOR="${RAFCODEPHI_EXPECT_JAVA_MAJOR:-21}"

fail() {
  local reason="$1"
  if command -v python3 >/dev/null 2>&1; then
    mkdir -p "$(dirname "$OUTPUT_PATH")"
    python3 - "$OUTPUT_PATH" "$reason" "${GITHUB_SHA:-TOKEN_VAZIO_PROVENANCE_UNBOUND}" <<'PY'
import datetime as dt
import json
import sys
from pathlib import Path

output, reason, source_commit = sys.argv[1:]
receipt = {
    "schema": "rafcodephi.toolchain-startup/v1",
    "state": "FAIL",
    "source_commit": source_commit,
    "observed_at": dt.datetime.now(dt.timezone.utc).isoformat(),
    "failure_reason": reason,
    "runtime_boundaries": {
        "termux_bootstrap_ninja_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
        "termux_session_keyboard_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
        "fastboot_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
        "physical_android": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
    },
    "claim_allowed": False,
}
Path(output).write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
  fi
  printf 'RAFCODEPHI_TOOLCHAIN=FAIL reason=%s\n' "$reason" >&2
  exit 1
}

read_gradle_property() {
  local key="$1"
  sed -n "s/^${key}=//p" "$ROOT_DIR/gradle.properties" | sed -n '1p' | tr -d '[:space:]'
}

# setup_android_toolchain.sh persists these values through GITHUB_ENV for CI. For a local
# two-command invocation, reconstruct the same deterministic paths from local.properties and
# gradle.properties because exports made by a child shell cannot modify its parent shell.
sdk_root="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
if [[ -z "$sdk_root" && -f "$ROOT_DIR/local.properties" ]]; then
  sdk_root="$(sed -n 's/^sdk.dir=//p' "$ROOT_DIR/local.properties" | sed -n '1p')"
fi
if [[ -z "$sdk_root" ]]; then
  for candidate in "$HOME/Android/Sdk" /usr/local/lib/android/sdk /opt/android-sdk /opt/android-sdk-linux; do
    if [[ -d "$candidate" ]]; then
      sdk_root="$candidate"
      break
    fi
  done
fi
[[ -n "$sdk_root" ]] || fail "android_sdk_root_missing"

compile_sdk="$(read_gradle_property compileSdkVersion)"
build_tools_version="$(read_gradle_property buildToolsVersion)"
[[ -n "$build_tools_version" ]] || build_tools_version="${compile_sdk}.0.0"
canonical_ndk_version="$(read_gradle_property ndkVersion)"
effective_ndk_version="${JITPACK_NDK_VERSION:-$canonical_ndk_version}"

export ANDROID_HOME="$sdk_root"
export ANDROID_SDK_ROOT="$sdk_root"
export ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-${sdk_root}/ndk/${effective_ndk_version}}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
export ANDROID_APKSIGNER="${ANDROID_APKSIGNER:-${sdk_root}/build-tools/${build_tools_version}/apksigner}"
export PATH="${sdk_root}/cmdline-tools/latest/bin:${sdk_root}/platform-tools:${sdk_root}/build-tools/${build_tools_version}:${sdk_root}/cmake/3.22.1/bin:${ANDROID_NDK_HOME}:$PATH"

for name in bash node java javac python3 sdkmanager adb fastboot ndk-build ninja; do
  command -v "$name" >/dev/null 2>&1 || fail "missing_command_${name}"
done

[[ -x "$ANDROID_APKSIGNER" ]] || fail "apksigner_not_executable"

node_version="$(node --version)"
node_major="${node_version#v}"
node_major="${node_major%%.*}"
[[ "$node_major" == "$EXPECTED_NODE_MAJOR" ]] || fail "node_major_${node_major}_expected_${EXPECTED_NODE_MAJOR}"

java_version="$(java -version 2>&1 | sed -n '1p')"
javac_version="$(javac -version 2>&1 | sed -n '1p')"
java_major="$(printf '%s\n' "$javac_version" | sed -E 's/^[^0-9]*([0-9]+).*/\1/')"
[[ "$java_major" == "$EXPECTED_JAVA_MAJOR" ]] || fail "java_major_${java_major}_expected_${EXPECTED_JAVA_MAJOR}"

bash_version="$(bash --version | sed -n '1p')"
sdkmanager_version="$(sdkmanager --version 2>&1 | sed -n '1p')"
adb_version="$(adb version 2>&1 | sed -n '1p')"
fastboot_version="$(fastboot --version 2>&1 | sed -n '1p')"
ndk_build_version="$(ndk-build --version 2>&1 | sed -n '1p')"
ninja_version="$(ninja --version 2>&1 | sed -n '1p')"
apksigner_version="$($ANDROID_APKSIGNER version 2>&1 | sed -n '1p')"

js_smoke="$(node -e "process.stdout.write('rafcodephi-js-ok')")"
[[ "$js_smoke" == "rafcodephi-js-ok" ]] || fail "javascript_smoke"

probe_dir="$(mktemp -d)"
cleanup() {
  rm -rf "$probe_dir"
}
trap cleanup EXIT

mkdir -p "$probe_dir/java" "$probe_dir/ninja"
cat > "$probe_dir/java/RafcodephiJdkSmoke.java" <<'JAVA'
public final class RafcodephiJdkSmoke {
    public static void main(String[] args) {
        System.out.print("rafcodephi-jdk-ok");
    }
}
JAVA
javac -d "$probe_dir/java" "$probe_dir/java/RafcodephiJdkSmoke.java"
jdk_smoke="$(java -cp "$probe_dir/java" RafcodephiJdkSmoke)"
[[ "$jdk_smoke" == "rafcodephi-jdk-ok" ]] || fail "jdk_compile_run_smoke"

cat > "$probe_dir/ninja/build.ninja" <<'NINJA'
rule emit
  command = printf rafcodephi-ninja-ok > $out
build proof.txt: emit
NINJA
ninja -C "$probe_dir/ninja" -f build.ninja >/dev/null
[[ "$(cat "$probe_dir/ninja/proof.txt")" == "rafcodephi-ninja-ok" ]] || fail "ninja_graph_smoke"

cat > "$probe_dir/ndk_probe.c" <<'C'
int rafcodephi_ndk_probe(void) { return 0x524146; }
C

ndk_llvm_bin=""
for candidate in "$ANDROID_NDK_HOME"/toolchains/llvm/prebuilt/*/bin; do
  if [[ -d "$candidate" ]]; then
    ndk_llvm_bin="$candidate"
    break
  fi
done
[[ -n "$ndk_llvm_bin" ]] || fail "ndk_llvm_bin_missing"

arm_clang="$ndk_llvm_bin/armv7a-linux-androideabi21-clang"
aarch64_clang="$ndk_llvm_bin/aarch64-linux-android21-clang"
llvm_readelf="$ndk_llvm_bin/llvm-readelf"
for executable in "$arm_clang" "$aarch64_clang" "$llvm_readelf"; do
  [[ -x "$executable" ]] || fail "ndk_probe_tool_missing"
done

"$arm_clang" -c "$probe_dir/ndk_probe.c" -o "$probe_dir/ndk-arm.o"
"$aarch64_clang" -c "$probe_dir/ndk_probe.c" -o "$probe_dir/ndk-aarch64.o"
"$llvm_readelf" -h "$probe_dir/ndk-arm.o" | grep -Eq 'Machine:[[:space:]]+ARM$' || fail "ndk_arm_object"
"$llvm_readelf" -h "$probe_dir/ndk-aarch64.o" | grep -Eq 'Machine:[[:space:]]+AArch64$' || fail "ndk_aarch64_object"

ndk_revision="TOKEN_VAZIO_NDK_REVISION"
if [[ -f "$ANDROID_NDK_HOME/source.properties" ]]; then
  ndk_revision="$(sed -n 's/^Pkg.Revision[[:space:]]*=[[:space:]]*//p' "$ANDROID_NDK_HOME/source.properties" | sed -n '1p')"
fi
[[ "$ndk_revision" != "TOKEN_VAZIO_NDK_REVISION" ]] || fail "ndk_revision_missing"
[[ "$ndk_revision" == "$effective_ndk_version" ]] || fail "ndk_revision_${ndk_revision}_expected_${effective_ndk_version}"

mkdir -p "$(dirname "$OUTPUT_PATH")"
script_sha256="$(sha256sum "$0" | awk '{print $1}')"
setup_sha256="$(sha256sum "$ROOT_DIR/scripts/setup_android_toolchain.sh" | awk '{print $1}')"
action_sha256="$(sha256sum "$ROOT_DIR/.github/actions/rafaelia-android-setup/action.yml" | awk '{print $1}')"
source_commit="${GITHUB_SHA:-TOKEN_VAZIO_PROVENANCE_UNBOUND}"

python3 - "$OUTPUT_PATH" "$source_commit" "$script_sha256" "$setup_sha256" "$action_sha256" \
  "$bash_version" "$node_version" "$java_version" "$javac_version" \
  "$sdkmanager_version" "$adb_version" "$fastboot_version" \
  "$ndk_revision" "$ndk_build_version" "$ninja_version" "$apksigner_version" <<'PY'
from __future__ import annotations

import datetime as dt
import json
import sys
from pathlib import Path

(
    output,
    source_commit,
    script_sha256,
    setup_sha256,
    action_sha256,
    bash_version,
    node_version,
    java_version,
    javac_version,
    sdkmanager_version,
    adb_version,
    fastboot_version,
    ndk_revision,
    ndk_build_version,
    ninja_version,
    apksigner_version,
) = sys.argv[1:]

receipt = {
    "schema": "rafcodephi.toolchain-startup/v1",
    "state": "PASS",
    "source_commit": source_commit,
    "verifier_sha256": script_sha256,
    "setup_sha256": setup_sha256,
    "composite_action_sha256": action_sha256,
    "observed_at": dt.datetime.now(dt.timezone.utc).isoformat(),
    "host_tools": {
        "bash": {"state": "PASS", "version": bash_version},
        "javascript_node_lts": {"state": "PASS", "version": node_version, "smoke": "rafcodephi-js-ok"},
        "jdk": {"state": "PASS", "java": java_version, "javac": javac_version, "compile_run": "PASS"},
        "android_sdk": {"state": "PASS", "sdkmanager": sdkmanager_version, "adb": adb_version},
        "fastboot_cli": {"state": "PASS", "version": fastboot_version, "device_execution": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE"},
        "ndk": {
            "state": "PASS",
            "revision": ndk_revision,
            "ndk_build": ndk_build_version,
            "arm_object": "PASS",
            "aarch64_object": "PASS",
        },
        "ninja": {"state": "PASS", "version": ninja_version, "graph_execution": "PASS"},
        "apksigner": {"state": "PASS", "version": apksigner_version},
    },
    "runtime_boundaries": {
        "termux_bootstrap_ninja_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
        "termux_session_keyboard_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
        "fastboot_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
        "physical_android": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE",
    },
    "claim_allowed": False,
}

Path(output).write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
print(f"RAFCODEPHI_TOOLCHAIN=PASS receipt={output}")
PY
