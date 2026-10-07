from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_rafaelia_dependency_profile_defaults_to_no_native_or_work_runtime():
    props = (ROOT / "gradle.properties").read_text(encoding="utf-8")

    assert "rafaelia.nativeBridge.enabled=false" in props
    assert "rafaelia.workRuntime.enabled=false" in props


def test_rafaelia_build_gates_native_and_external_work_dependencies():
    build = (ROOT / "rafaelia" / "build.gradle").read_text(encoding="utf-8")

    assert 'providers.gradleProperty("rafaelia.nativeBridge.enabled")' in build
    assert 'providers.gradleProperty("rafaelia.workRuntime.enabled")' in build
    assert "if (rafaeliaNativeBridgeEnabled)" in build
    assert "if (rafaeliaWorkRuntimeEnabled)" in build
    assert 'exclude "**/RafaeliaBatchScheduler.java"' in build
    assert 'exclude "**/RafaeliaBatchWorker.java"' in build
    assert "implementation 'androidx.work:work-runtime:2.9.1'" in build
    assert "implementation 'androidx.annotation:annotation:1.8.2'" in build


def test_rafaelia_utils_public_api_has_no_required_jni_surface():
    utils = (
        ROOT
        / "rafaelia"
        / "src"
        / "main"
        / "java"
        / "com"
        / "termux"
        / "rafaelia"
        / "RafaeliaUtils.java"
    ).read_text(encoding="utf-8")

    assert "public static native" not in utils
    assert "private static native float sqrtNative" in utils


if __name__ == "__main__":
    test_rafaelia_dependency_profile_defaults_to_no_native_or_work_runtime()
    test_rafaelia_build_gates_native_and_external_work_dependencies()
    test_rafaelia_utils_public_api_has_no_required_jni_surface()
