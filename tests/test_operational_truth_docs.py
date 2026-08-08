from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

def test_truth_table_separates_source_built_payload_from_runtime_proof():
    text = (ROOT / 'docs/RUNTIME_TRUTH_TABLE.md').read_text(encoding='utf-8')
    for token in [
        '`pkg` real | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE',
        '`apt` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE',
        '`apt-get` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE',
        '`proot` | PROVADO ESTRUTURAL NO PAYLOAD, TOKEN_VAZIO NO DEVICE',
        'BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED',
    ]:
        assert token in text
    assert '`pkg update` | FUTURO' in text
    assert '`pkg install` | FUTURO' in text

def test_status_documents_canonical_sdk_abi_and_boundaries():
    text = (ROOT / 'docs/STATUS.md').read_text(encoding='utf-8')
    for token in ['targetSdkVersion=28', 'compileSdkVersion=35', 'minSdkVersion=21', 'armeabi-v7a', 'arm64-v8a', 'ZIPRAF não comprime fisicamente', 'RAFAELIA deterministic VCPU state kernel']:
        assert token in text

def test_device_smoke_has_required_gate():
    text = (ROOT / 'scripts/device_runtime_smoke.sh').read_text(encoding='utf-8')
    assert 'DEVICE_SMOKE_REQUIRED' in text
    assert 'DEVICE_VALIDATED' in text
