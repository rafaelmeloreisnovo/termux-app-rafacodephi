#!/usr/bin/env python3
from pathlib import Path

WORKFLOW = Path('.github/workflows/rafcodephi-v1-termux-packages.yml')
PIN = '7b59383c25f7557ba8a29a24f715c5fb5b26cc53'


def require(text: str, token: str) -> None:
    if token not in text:
        raise AssertionError(f'missing V1 integration invariant: {token}')


def main() -> int:
    text = WORKFLOW.read_text(encoding='utf-8')

    for token in (
        'repository: rafaelmeloreisnovo/termux-packages',
        f'TERMUX_PACKAGES_PIN: {PIN}',
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
        'physical_android: PASS',
        "'physical_android': 'PASS'",
        'claim_allowed: true',
        "'claim_allowed': True",
        'packages.rafcodephi.com',
    )
    for token in forbidden:
        if token in text:
            raise AssertionError(f'forbidden premature V1 promotion: {token}')

    print('PASS: RAFCODEPHI V1 binds pinned termux-packages -> source-built ARM/ARM64 bootstrap -> APK; device remains TOKEN_VAZIO')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
