# Cross-Domain Static Inventory — 2026-08-14 — V1

- mode: `APPEND_ONLY | EVIDENCE_FIRST | FAIL_CLOSED`
- base_commit: `1f9b7e7abc76cf0d19d9bd497949b3efbe05d3c5`
- evidence_class: `OBSERVED_STATIC`
- scope: `DIRECT_DECLARATIONS_ONLY`
- claim_allowed: `false`
- runtime_claim_promoted: `false`

## What this closes

This materializes the queued P1.2 inventory of current Gradle/native declarations with source blobs. It does **not** claim a complete SBOM, resolved transitive graph, installed state, loaded state, device runtime, benchmark, release readiness, or security clearance.

## Build baseline observed

| Surface | Observed declaration |
|---|---|
| AGP | `8.13.2` |
| compileSdk | `35` |
| targetSdk | `28` |
| minSdk | `21` |
| NDK | `26.3.11579264` |
| Required ABI | `armeabi-v7a, arm64-v8a` |
| Optional ABI | empty |
| Universal APK | enabled |

Primary sources: `build.gradle`, `gradle.properties`, `app/build.gradle` at the pinned base commit.

## Module topology

Current `settings.gradle` declares seven modules: `:app`, `:termux-shared`, `:terminal-emulator`, `:terminal-view`, `:rafaelia`, `:rmr`, and `:loader`.

Observed direct project edges from current build declarations:

```text
:app -> :terminal-view
:app -> :termux-shared
:termux-shared -> :terminal-view
:terminal-view -> :terminal-emulator
```

No direct Gradle edge from `:app` to `:rafaelia` or `:rmr` was observed in the current root dependency injection. Native source coupling still exists: the app Android.mk compiles a RAFAELIA ZERO runtime source from the `rafaelia` tree directly. These are different relations and must not be collapsed.

## Native declarations

`app/src/main/cpp/Android.mk` declares:
- `libtermux-bootstrap`
- `termux-baremetal`
- `termux_rafaelia_direct`
- `api_lowlevel`
- `raf_pa_core`
- `termux_rafaelia_zero_runtime`

`rmr/src/main/cpp/Android.mk` declares `rmr`.

`rafaelia/src/main/cpp/Android.mk` declares `termux-rafaelia` and `termux_rafaelia_zero`.

The canonical Gradle ABI matrix is ARM32 + ARM64. Source-level x86/x86_64 conditionals exist in native files, but optional ABI configuration is empty; therefore source presence is **not** promoted to supported/runtime-verified ABI status.

## Drift found in historical dependency documentation

`docs/DEPENDENCIAS_TOTAIS.md` is useful historical documentation but is not an exact current build mirror:

1. it records `termux-shared` Guava `24.1-jre`; current `termux-shared/build.gradle` declares `32.1.3-jre`;
2. it omits current `:rafaelia` implementation dependencies `androidx.annotation:1.8.2`, `androidx.work:work-runtime:2.9.1`, plus the `org.json:20240303` test dependency;
3. its module list omits `:loader`;
4. it lists `:rafaelia` and `:rmr` as app dependencies, while the current root injection declares only `:terminal-view` and `:termux-shared`.

State: `ERRATUM_REQUIRED`. Rule: append a dated correction/snapshot; do not silently rewrite the historical document.

## License authority conflict

Two repository authorities currently describe RAFAELIA/RMR scope differently:

- `LICENSE.md`: RAFAELIA modifications/components are described as GPLv3-compatible within the Termux fork.
- `rmr/docs/LICENCIAMENTO_RAFAELIA.md`: describes a dual layer with MIT infrastructure and a separate non-commercial RAFAELIA Core layer.

This pass does not adjudicate legal precedence. Operational state:

```text
GAP-XDOM-LICENSE-AUTHORITY-001 = OPEN / P0
promotion = BLOCKED_UNTIL_AUTHORITY_MAP
```

`termux-shared/LICENSE.md` separately declares MIT primary licensing with file-level GPLv3, GPLv2+Classpath-exception and Apache-2.0 exceptions.

## Lifecycle boundary

Every component in this inventory is at most `INVENTORIED`. This artifact does not imply `INSTALLED`, `LOADED`, `RUNTIME_REACHABLE`, `EXECUTED`, `MEASURED`, or `REPRODUCED`; those transitions require runtime/device evidence.

## Remaining gaps

- `GAP-XDOM-LICENSE-AUTHORITY-001` — P0 — resolve file-level normative license authority.
- `GAP-XDOM-RUNTIME-STATE-001` — P0 — device/runtime receipts remain `TOKEN_VAZIO`.
- `GAP-XDOM-TRANSITIVE-001` — P1 — resolved transitive dependency graph + hashes remain `TOKEN_VAZIO`.
- `GAP-XDOM-EXTERNAL-LICENSES-001` — P1 — authoritative license metadata per external coordinate remains `TOKEN_VAZIO`.
- `GAP-XDOM-UPSTREAM-ADVISORY-001` — P1 — upstream/advisory bindings remain `TOKEN_VAZIO`.
- `GAP-XDOM-ABI-DORMANT-001` — P1 — x86/x86_64 source branches are non-canonical until explicit restoration/evidence.

## Validation contract

`tools/validate_cross_domain_static_inventory.py` fails closed if claim/runtime promotion becomes true, the static lifecycle boundary is removed, canonical module/ABI declarations disappear silently, the license-authority conflict is hidden before resolution, document drift is erased, or source blob provenance becomes malformed.

The PR-specific workflow validates the inventory and emits a JSON receipt. A CI PASS proves this artifact's structural/static contract only.

## Invariant

```text
STATIC_DECLARATION != RESOLVED_TRANSITIVE_GRAPH
INVENTORIED != INSTALLED != LOADED != EXECUTED
SOURCE_ABI_BRANCH != SUPPORTED_ABI
DOCUMENTATION != BUILD_TRUTH
LICENSE_DECLARATION_CONFLICT => PROMOTION_BLOCKED
CI_STATIC_PASS != DEVICE_RUNTIME
```
