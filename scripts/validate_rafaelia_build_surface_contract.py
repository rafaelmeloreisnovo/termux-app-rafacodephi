#!/usr/bin/env python3
from pathlib import Path

WORKFLOW = Path(".github/workflows/rafaelia_pipeline.yml")


def require(text: str, token: str) -> None:
    if token not in text:
        raise SystemExit(f"RAFAELIA_BUILD_SURFACE_CONTRACT=FAIL missing={token!r}")


def main() -> int:
    text = WORKFLOW.read_text(encoding="utf-8")

    # Routine pushes must never silently promote themselves to signed Release.
    if '[[ -n "$BUILD_INPUT" ]] || BUILD_INPUT=both' in text:
        raise SystemExit(
            "RAFAELIA_BUILD_SURFACE_CONTRACT=FAIL "
            "reason=implicit_push_release_promotion"
        )

    for token in (
        'if [[ "$GITHUB_EVENT_NAME" == "pull_request" ]]; then',
        'elif [[ "$GITHUB_EVENT_NAME" == "workflow_dispatch" ]]; then',
        'BUILD_INPUT="${{ inputs.build_type }}"',
        'BUILD_INPUT=debug',
        'BUILD_FULL_DEBUG=true',
        'BUILD_RELEASE=false',
        'BUILD_RELEASE=true',
        'needs.psi-perception.outputs.build_release == \'true\'',
    ):
        require(text, token)

    dispatch_idx = text.index('elif [[ "$GITHUB_EVENT_NAME" == "workflow_dispatch" ]]; then')
    fallback_idx = text.index("else", dispatch_idx)
    block = text[dispatch_idx:fallback_idx]
    require(block, 'BUILD_INPUT="${{ inputs.build_type }}"')

    # The non-PR/non-manual fallback is the normal push surface.
    tail = text[fallback_idx:text.index('case "$BUILD_INPUT" in', fallback_idx)]
    require(tail, "BUILD_INPUT=debug")
    require(tail, "BUILD_FULL_DEBUG=true")

    print(
        "RAFAELIA_BUILD_SURFACE_CONTRACT=PASS "
        "push=debug+full-debug manual=explicit release=fail-closed"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
