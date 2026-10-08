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
