from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_setup_uses_effective_ndk_and_exports_every_followup_path() -> None:
    setup = read("scripts/setup_android_toolchain.sh")

    assert 'EFFECTIVE_NDK_VERSION="${JITPACK_NDK_VERSION:-$CANONICAL_NDK_VERSION}"' in setup
    assert '"ndk;${EFFECTIVE_NDK_VERSION}"' in setup
    for token in (
        "ANDROID_HOME",
        "ANDROID_SDK_ROOT",
        "ANDROID_NDK_HOME",
        "ANDROID_NDK_ROOT",
        "ANDROID_APKSIGNER",
        "RAFCODEPHI_EFFECTIVE_NDK_VERSION",
        "ANDROID_CMDLINE_TOOLS_PATH",
        "ANDROID_PLATFORM_TOOLS_PATH",
        "NDK_PREBUILT_BIN",
    ):
        assert token in setup
    for tool in ("/adb", "/fastboot", "/ndk-build", "/bin/ninja", "/apksigner"):
        assert tool in setup


def test_toolchain_verifier_executes_real_language_and_cross_abi_smokes() -> None:
    verifier = read("scripts/verify_toolchain_startup.sh")

    for command in ("bash", "node", "java", "javac", "sdkmanager", "adb", "fastboot", "ndk-build", "ninja"):
        assert command in verifier
    assert "EXPECTED_NODE_MAJOR" in verifier and "24" in verifier
    assert "EXPECTED_JAVA_MAJOR" in verifier and "21" in verifier
    assert "rafcodephi-js-ok" in verifier
    assert "rafcodephi-jdk-ok" in verifier
    assert "build.ninja" in verifier and "rafcodephi-ninja-ok" in verifier
    assert "armv7a-linux-androideabi21-clang" in verifier
    assert "aarch64-linux-android21-clang" in verifier
    assert "NDK_ELF_ABI_MISMATCH" in verifier
    assert 'int.from_bytes(data[18:20], "little")' in verifier
    assert '(sys.argv[1], 1, 40)' in verifier
    assert '(sys.argv[2], 2, 183)' in verifier
    assert '"schema": "rafcodephi.toolchain-startup/v1"' in verifier
    assert '[[ "$ndk_revision" == "$effective_ndk_version" ]]' in verifier
    assert '"termux_session_keyboard_device": "TOKEN_VAZIO_ENVIRONMENT_UNAVAILABLE"' in verifier
    assert '"claim_allowed": False' in verifier
    assert '"state": "FAIL"' in verifier
    assert '"setup_sha256": setup_sha256' in verifier
    assert '"composite_action_sha256": action_sha256' in verifier
    assert "exports made by a child shell cannot modify its parent shell" in verifier
    assert 'sdk_root="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"' in verifier



def test_ndk_elf_header_probe_accepts_real_arm_bytes_and_rejects_corruption() -> None:
    """Run the real embedded NDK verifier on valid and adversarial ELF headers."""
    import subprocess
    import sys
    import tempfile

    verifier = read("scripts/verify_toolchain_startup.sh")
    marker = "if ! python3 - \"$probe_dir/ndk-arm.o\" \"$probe_dir/ndk-aarch64.o\" <<'PY'\n"
    assert verifier.count(marker) == 1
    probe = verifier.split(marker, 1)[1].split("\nPY\n", 1)[0]

    def elf_header(elf_class: int, machine: int) -> bytes:
        header = bytearray(20)
        header[:4] = b"\x7fELF"
        header[4] = elf_class
        header[5] = 1  # little endian
        header[18:20] = machine.to_bytes(2, "little")
        return bytes(header)

    with tempfile.TemporaryDirectory(prefix="raf-ndk-elf-") as folder:
        arm = Path(folder) / "arm.o"
        arm64 = Path(folder) / "aarch64.o"

        def run_probe() -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [sys.executable, "-c", probe, str(arm), str(arm64)],
                capture_output=True, text=True, timeout=5, check=False,
            )

        arm.write_bytes(elf_header(1, 40))
        arm64.write_bytes(elf_header(2, 183))
        positive = run_probe()
        assert positive.returncode == 0, positive.stderr
        assert "NDK_ELF_ABI=PASS_SCOPED" in positive.stdout

        for corrupted in (
            b"NOT_ELF",
            elf_header(1, 183),   # ARM32 class with AArch64 machine
            elf_header(2, 40),    # ARM64 class with ARM machine
            b"\x00" + elf_header(1, 40)[1:],  # invalid magic
        ):
            arm.write_bytes(corrupted)
            negative = run_probe()
            assert negative.returncode != 0, (
                f"invalid ARM object unexpectedly passed: {corrupted!r}"
            )
            arm.write_bytes(elf_header(1, 40))

        arm64.write_bytes(elf_header(2, 40))
        assert run_probe().returncode != 0, "ARM64 machine mismatch accepted"


def test_composite_action_is_immutable_and_prepares_bootstrap_once() -> None:
    action = read(".github/actions/rafaelia-android-setup/action.yml")

    assert "actions/setup-java@de7274f081f381c8f8158605e0321c36c376e2e6" in action
    assert "actions/setup-node@2028fbc5c25fe9cf00d9f06a71cc4710d4507903" in action
    assert "android-actions/setup-android@" not in action
    assert "default: '21'" in action
    assert "default: '24'" in action
    assert "TERMUX_SKIP_BOOTSTRAP_PREPARE: '1'" in action
    assert "RAFCODEPHI_EXPECT_JAVA_MAJOR: ${{ inputs.java-version }}" in action
    assert "RAFCODEPHI_EXPECT_NODE_MAJOR: ${{ inputs.node-version }}" in action
    assert action.count("./scripts/prepare_bootstrap_env.sh") == 1
    assert "./scripts/verify_toolchain_startup.sh" in action


def test_workflows_exercise_clean_build_and_expose_real_pkg_route() -> None:
    real_pkg = read(".github/workflows/real-pkg-apk-ci.yml")
    arm32 = read(".github/workflows/_reusable-arm32-compat.yml")
    start_here = read(".github/workflows/00_START_HERE.yml")
    gradle = read("app/build.gradle")

    assert "workflow_call:" in real_pkg
    assert "./.github/actions/rafaelia-android-setup" in real_pkg
    assert "./gradlew clean :app:assembleDebug" in real_pkg
    assert "build/reports/toolchain-startup.json" in real_pkg
    assert "JITPACK_NDK_VERSION: \'29.0.14206865\'" in real_pkg
    assert "steps.startup.outcome != \'skipped\'" in real_pkg
    assert "./.github/actions/rafaelia-android-setup" in arm32
    assert "11_REAL_PKG_TOOLCHAIN" in start_here
    assert "./.github/workflows/real-pkg-apk-ci.yml" in start_here
    assert '"11": ["route", "inventory", "policy_contracts", "real_pkg_toolchain"]' in start_here
    assert 'mustRunAfter(tasks.named("clean"))' in gradle
    assert '!it.name.startsWith("externalNativeBuildClean")' in gradle


if __name__ == "__main__":
    tests = [value for name, value in sorted(globals().items()) if name.startswith("test_") and callable(value)]
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
