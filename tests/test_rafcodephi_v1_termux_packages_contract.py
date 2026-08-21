#!/usr/bin/env python3
import json
from pathlib import Path

WORKFLOW = Path('.github/workflows/rafcodephi-v1-termux-packages.yml')
PIN_CONTRACT = Path('data/contracts/termux-packages-rafcodephi-pin.v1.json')
RESOLVER = Path('scripts/resolve_termux_packages_pin.py')
DEPRECATED_MAGIC_PIN = '7a26629938452c6d6fd80cf3fccce8c2056aabac'


def require(text: str, token: str) -> None:
    if token not in text:
        raise AssertionError(f'missing V1 integration invariant: {token}')


def main() -> int:
    text = WORKFLOW.read_text(encoding='utf-8')
    contract = json.loads(PIN_CONTRACT.read_text(encoding='utf-8'))

    assert contract['schema'] == 'rafcodephi.termux-packages-pin/v1'
    assert contract['repository'].endswith('/rafaelmeloreisnovo/termux-packages.git')
    assert contract['package_name'] == 'com.termux.rafacodephi'
    assert contract['prefix'] == '/data/data/com.termux.rafacodephi/files/usr'
    assert contract['required_abis'] == ['armeabi-v7a', 'arm64-v8a']
    assert contract['channels']['canonical']['state'] == 'MERGED_BASELINE'
    assert contract['channels']['candidate']['claim_allowed'] is False
    assert contract['channels']['candidate']['physical_android'] == 'TOKEN_VAZIO'
    assert RESOLVER.is_file()

    for token in (
        'repository: rafaelmeloreisnovo/termux-packages',
        'default: canonical',
        "|| 'canonical'",
        'scripts/resolve_termux_packages_pin.py',
        '--github-env --json',
        'ref: ${{ env.TERMUX_PACKAGES_SHA }}',
        'test "$ACTUAL_SHA" = "$TERMUX_PACKAGES_SHA"',
        'scripts/apply-rafcodephi-build-properties.py',
        'scripts/validate-rafcodephi-build-properties.sh',
        'scripts/build-rafcodephi-real-bootstrap.sh',
        '--architectures arm,aarch64',
        'RAF_BOOTSTRAP_SOURCE=source-built-real',
        'RAF_REAL_BOOTSTRAP_ZIP_ARM=',
        'RAF_REAL_BOOTSTRAP_ZIP_AARCH64=',
        'RAF_REAL_BOOTSTRAP_MANIFEST=',
        'scripts/prepare_bootstrap_env.sh',
        'tests/test_source_built_real_bootstrap.py',
        './gradlew assembleDebug',
        'scripts/resolve_apk_output_dir.sh',
        "'physical_android': 'TOKEN_VAZIO'",
        "'claim_allowed': False",
        "'package_repo_runtime_state': 'BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED'",
        "'schema': 'rafcodephi.v1-termux-packages-to-apk/v1'",
    ):
        require(text, token)

    forbidden = (
        DEPRECATED_MAGIC_PIN,
        '7b59383c25f7557ba8a29a24f715c5fb5b26cc53',
        'physical_android: PASS',
        "'physical_android': 'PASS'",
        'claim_allowed: true',
        "'claim_allowed': True",
        'packages.rafcodephi.com',
    )
    for token in forbidden:
        if token in text:
            raise AssertionError(f'forbidden premature/stale V1 route: {token}')

    print('PASS: V1 resolves semantic pin -> exact termux-packages -> source-built ARM/ARM64 bootstrap -> APK; device remains TOKEN_VAZIO')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
