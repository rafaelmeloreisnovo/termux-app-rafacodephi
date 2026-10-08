# CI fastlane and queue containment — 2026-10-08

Copyright (c) 2026 Rafael Melo Reis. Retain original project license and provenance.

Beta Build was an unconditional push-triggered 240-minute dual-ABI pipeline. Change: beta-build manual workflow_dispatch only; retain entire full ARM/APK build for explicit requests, NOT_RUN otherwise. START_HERE superseded push runs cancelled, manual never. Safety CI feature push duplication removed; installed multilib toolchain reused, apt install only when missing. Device physical receipt and protected branch rules remain independent. Rollback by reverting PR; no secrets or APK touched.

## Steps for a human or AI operator

1. Keep work in a Draft PR; inspect quick source-contract results without promoting them to binary PASS.
2. Mark Ready for review to trigger all retained full PR/ABI tests that apply.
3. Review source and rights; merge only with required checks and provider branch-protection readback.
4. Invoke expensive producer/beta/publish workflows deliberately after source is stable; capture exact source SHA, run ID and artifacts.
5. Never call source-only, QEMU-only or CI-only evidence physical Android validation. Record TOKEN_VAZIO, NOT_RUN, FAIL or PASS precisely.

R3 = <F_ok: source gate/refactor on PR, F_gap: CI executed exact head, provider P0, hardware receipts, F_next: CI quick -> ready full -> explicit delivery -> device readback>.

## Canonical ψχρΔΣΩ source/binary scope

The existing rafaelia_pipeline.yml still orchestrates every required stage for ready PR, main push, manual build and release. For a draft PR only ψ Perception and χ Feedback are executed; ρ ARM APK, Δ full tests and Σ Android compatibility are intentionally skipped. Ω terminal requires ψχ but emits FAST_SOURCE_CONTRACT_PASS / FULL_APK_NOT_RUN / CLAIM_ALLOWED=false rather than presenting its source-only verdict as a full Android PASS. Explicit release and manual dispatch remain unchanged.

## Failing baseline gate discovered and surgically fixed

PR #499 START inventory failed its immutable-action-reference test because the two freestanding workflows contained **11 floating major-version action refs**. This was an independent source issue (not the CI speed routing). Exact official GitHub tag commits were verified using the public Git refs API and pinned without changing action major-version behavior:

- actions/checkout@v7 -> 3d3c42e5aac5ba805825da76410c181273ba90b1
- actions/upload-artifact@v7 -> cf430e030ddbb5b0abf93d22962f4752f3646cd9
- actions/download-artifact@v8 -> 9000827ccba6bdab643e8b6fd33ac0654aef8333

The separate stale contract asserting PR-only cancellation was updated to match PR+push cancellation while preserving per-run-id isolation for explicit workflow_dispatch routes. Exact-head CI readback is required before promoting these source patches to PASS.
