# AGENTS.md — RAFAELIA / termux-app-rafacodephi

## Federation entry

This repository is the RAFAELIA **Android runtime**. Do not treat conversation context or copied instructions from another repository as local authority.

On entry:

1. bind the exact ref/commit before reading status;
2. read this file, then the smallest relevant local index/contract;
3. classify open work on independent axes: knowledge, attention, urgency, operational state and claim gate;
4. preserve `TOKEN_VAZIO`, deferred and ignored-with-reason items explicitly;
5. choose the next action by urgency + upstream dependency + observable exit criterion;
6. record rollback/baseline before mutation;
7. verify repository-local gates separately from physical-device/cross-repository gates;
8. emit `F_ok`, `F_gap`, `F_next` with exact evidence refs.

Federation kernel authority: `rafaelmeloreisnovo/RafGitTools:configs/agent-entry-kernel.v1.json` when cross-repository access is available.

## Local authority and entry routes

Role: bootstrap, package identity, Android execution services, local receipts and the provider side of Vectra↔Termux integration.

Open indices before broad crawling:

- `README_TERMUX.md`
- `docs/agents/README.md`
- `docs/00_BUG_MASTER_INDEX.md`
- relevant `tests/` contract for the changed surface
- relevant `.github/workflows/` gate for the exact claim being tested

## Package/runtime invariants

- Canonical app package: `com.termux.rafacodephi`.
- Preserve `armeabi-v7a`; do not silently make ARM64 the only supported path.
- A CI build is not physical Android runtime evidence.
- A materialized bridge bootstrap is not proof of a real `pkg/apt` runtime.
- `device_validation=TOKEN_VAZIO` stays open until an exact APK/device/runtime receipt exists.
- Bootstrap evidence must remain bound to the archive/profile/hash actually consumed by the APK.
- Do not inherit VERIFIED state from an older bootstrap/APK/commit without explicit evidence linkage.

## Vectra provider boundary

The repository exposes the Termux side of the Vectra integration. Keep these boundaries distinct:

- discovery/capability negotiation and execution are separate protocols;
- discovery acceptance does not prove QEMU execution;
- RunCommand dispatch does not prove exit code, guest boot or VM correctness;
- private Termux paths must not be exposed as the public integration contract;
- permission/package/action drift is a cross-repository blocker, not a local cosmetic issue.

For any Vectra↔Termux claim, require producer + consumer contract evidence and keep physical E2E as `TOKEN_VAZIO` until a device receipt exists.

## Manifest / Android component discipline

- Component presence is not component uniqueness.
- Conflicting duplicate declarations are a gate failure candidate and must be tested explicitly.
- Settings/navigation targets must resolve to exactly the intended component.
- Exported state, permissions and theme are part of the contract when callers depend on them.

Known current P0 is tracked separately in PR #373; do not infer it is merged from its implementation branch.

## Build/validation guidance

CI-observed Android preparation/build path includes:

```bash
./scripts/setup_android_toolchain.sh
./scripts/prepare_bootstrap_env.sh
./gradlew :app:assembleDebug --no-daemon
```

Use the workflow/task appropriate to the changed surface rather than assuming one build proves every contract. Preserve release-signing boundaries.

## Safety / historical determinism

- `TOKEN_VAZIO != 0 != false != PASS`.
- `READY_TO_TEST != RESOLVED`.
- Urgency orders execution; it does not increase truth.
- Append/supersede evidence; do not rewrite older observations.
- High-risk changes require a rollback reference.
- If a gate reveals a new gap, index it instead of weakening the gate.
- Do not copy Vectra assembly/register/attractor rules into this root file as Termux authority; those belong to the Vectra/local module that owns them.
