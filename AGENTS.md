# AGENTS.md — RAFAELIA / termux-app-rafacodephi

## Federation entry

This repository is the RAFAELIA **Android runtime producer/provider**. Do not treat conversation context or copied instructions from another repository as local authority.

Federated routing/state authority: `rafaelmeloreisnovo/Mapa`.  
Control-plane executor contract: `rafaelmeloreisnovo/RafGitTools:configs/agent-entry-kernel.v1.json` when cross-repository access is available.

### Mandatory service preflight — Q01..Q12

Before mutating or promoting state, answer with exact pointers or typed `TOKEN_VAZIO`:

1. **Quem sou?** — Android-runtime/provider agent + local repository role.
2. **Qual repo/ref/path/hash estou lendo?** — repository, ref, exact commit, path and object/blob/artifact identity.
3. **Qual minha autoridade?** — Termux owns local Android/bootstrap/provider implementation; `Mapa` owns federated route/state; state the write scope.
4. **Qual minha fronteira?** — build/static/provider claims are distinct from physical device/runtime/guest claims; keep `claim_allowed=false` unless a bounded gate explicitly promotes scope.
5. **Quais índices locais devo abrir?** — minimum local docs/tests/workflow only.
6. **Qual rota do Mapa corresponde ao objetivo?** — route/anchors or typed `TOKEN_VAZIO`; do not invent a Mapa route.
7. **Que lacunas já existem?** — gap IDs, `TOKEN_VAZIO`, uncertainties and dependencies.
8. **Qual evidência é atual?** — exact commit/APK/bootstrap/protocol/device/receipt scope plus staleness.
9. **Qual gate posso executar?** — test/workflow/device fixture, falsifier, exit criterion and rollback.
10. **Quando devo parar?** — stop on dependency/authority/privacy/security block, observed exit, or no marginal reconstruction gain.
11. **Onde registro o delta?** — local receipt first; route material state to `Mapa`; update Drive reconstruction only when navigation/provenance materially changes.
12. **Quais regras de governança, dados, privacidade e segurança governam esta unidade?** — classify all four before mutation.

### Local governance/data/privacy/security defaults

These are starting boundaries, not substitutes for per-work classification:

- **Governance:** local implementation authority; cross-repo claim/state promotion requires federated routing/receipt. High/critical mutation needs rollback.
- **Data:** build metadata and source/config may be PUBLIC/INTERNAL; device/runtime/log payload is not assumed public. Preserve exact schema/identity and minimum necessary fields.
- **Privacy:** device IDs, user paths, logs, environment values, retrieved files and app/user payload must be minimized/redacted or represented by typed references when possible. Unknown sensitivity blocks copying to public indices.
- **Security:** exported components, permissions, package identity, signing, bootstrap/archive integrity, secrets, result bounding and provider IPC are security surfaces. Success booleans must derive from terminal gate evidence.

Unknown governance/privacy/security classification is a **fail-closed mutation blocker**, not a permissive default.

## Local authority and entry routes

Role: bootstrap, package identity, Android execution services, local receipts and the provider side of Vectra↔Termux integration.

Open indices before broad crawling:

- `README_TERMUX.md`
- `docs/agents/README.md`
- `docs/00_BUG_MASTER_INDEX.md`
- relevant `tests/` contract for the changed surface
- relevant `.github/workflows/` gate for the exact claim being tested

If those reconstruct the target and additional history cannot change the gate/evidence/provenance/privacy/security classification, stop crawling and execute the bounded gate.

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

IPC/request/result receipts should carry only the minimum fields needed for identity, replay protection, validation and reconstruction; do not expose unrelated private filesystem or user payload.

## Manifest / Android component discipline

- Component presence is not component uniqueness.
- Conflicting duplicate declarations are a gate failure candidate and must be tested explicitly.
- Settings/navigation targets must resolve to exactly the intended component.
- Exported state, permissions and theme are part of the contract when callers depend on them.

Historical P0 work must be rebound to current `master`/candidate head before state promotion; a prior PR number is provenance, not current proof.

## Build/validation guidance

CI-observed Android preparation/build path includes:

```bash
./scripts/setup_android_toolchain.sh
./scripts/prepare_bootstrap_env.sh
./gradlew :app:assembleDebug --no-daemon
```

Use the workflow/task appropriate to the changed surface rather than assuming one build proves every contract. Preserve release-signing boundaries.

## Required transition receipt

For material work record at minimum:

```text
event/parent
repo/ref/commit/path
authority + write scope
gap/goal IDs
urgency + risk
governance/data/privacy/security classification
action + falsifier + exit criterion + stop reason
evidence refs
F_ok / F_gap / F_next
uncertainty delta
rollback ref
claim_allowed
```

## Safety / historical determinism

- `TOKEN_VAZIO != 0 != false != PASS`.
- `READY_TO_TEST != RESOLVED`.
- Urgency orders execution; it does not increase truth.
- Append/supersede evidence; do not rewrite older observations.
- High-risk changes require a rollback reference.
- If a gate reveals a new gap, index it instead of weakening the gate.
- Do not copy Vectra assembly/register/attractor rules into this root file as Termux authority; those belong to the Vectra/local module that owns them.
- Do not copy private/sensitive payload into a public receipt when a hash/pseudonym/typed reference is sufficient.
