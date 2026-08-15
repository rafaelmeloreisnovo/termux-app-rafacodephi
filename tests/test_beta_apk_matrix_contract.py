from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_MATRIX = ROOT / 'scripts/build_apk_matrix.sh'
BETA_WORKFLOW = ROOT / '.github/workflows/beta-build.yml'


def test_build_apk_matrix_uses_valid_bootstrap_payload_source_default():
    text = BUILD_MATRIX.read_text(encoding='utf-8')
    assert 'BOOTSTRAP_SOURCE_REQUESTED="${RAF_BOOTSTRAP_SOURCE:-local}"' in text
    assert 'RAF_BOOTSTRAP_SOURCE must be local, upstream, or source-built-real' in text
    assert 'termux-packages-source-contract' in text
    assert 'using local bootstrap payload generation' in text


def test_build_apk_matrix_validates_hashes_for_required_abi_policy_only():
    text = BUILD_MATRIX.read_text(encoding='utf-8')
    assert 'required_abis=( $(abi_policy_required_array) )' in text
    assert 'bootstrap_hash_keys=()' in text
    assert 'arm64-v8a) bootstrap_hash_keys+=(AARCH64)' in text
    assert 'armeabi-v7a) bootstrap_hash_keys+=(ARM)' in text
    assert 'for v in "${bootstrap_hash_keys[@]}"' in text
    assert 'for v in AARCH64 ARM I686 X86_64' not in text


def test_build_apk_matrix_records_diagnostics_but_keeps_strict_default():
    text = BUILD_MATRIX.read_text(encoding='utf-8')
    assert 'APK_MATRIX_UNIT_TESTS_REQUIRED="${APK_MATRIX_UNIT_TESTS_REQUIRED:-true}"' in text
    assert ':app:testDebugUnitTest failed and APK_MATRIX_UNIT_TESTS_REQUIRED=true' in text
    assert 'APK_MATRIX_DIAGNOSTIC.md' in text


def test_beta_workflow_keeps_apk_matrix_strict():
    text = BETA_WORKFLOW.read_text(encoding='utf-8')
    assert 'id: apk_matrix' in text
    assert 'RAF_BOOTSTRAP_SOURCE: source-built-real' in text
    assert 'RELEASE_TRACK: internal' in text
    assert 'APK_MATRIX_UNIT_TESTS_REQUIRED: "true"' in text
    assert 'APK_MATRIX_UNIT_TESTS_REQUIRED: "false"' not in text
