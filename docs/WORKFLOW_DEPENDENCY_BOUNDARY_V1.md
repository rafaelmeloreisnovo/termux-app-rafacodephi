# RAFCODEΦ Workflow Dependency Boundary V1

## Purpose

Machine/human reconstruction map for the GitHub Actions surface that is reachable
from the canonical operator entry:

`.github/workflows/00_START_HERE.yml`

This document does **not** call GitHub Actions freestanding. The provider, hosted
runner, Actions runtime, JDK, Android SDK/NDK and process/filesystem services are
explicit hosted/tooling boundaries.

The invariant remains:

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`

and `TOKEN_VAZIO != 0`.

## Governed front-door closure

The action-reference auditor resolves local reusable-workflow edges recursively:

```text
00_START_HERE.yml
├── provider-protection-gate.yml
├── run_tests.yml
├── compatibility-arm32.yml
│   └── _reusable-arm32-compat.yml
├── compatibility-arm32-ndk29.yml
│   └── _reusable-arm32-compat.yml
├── beta-real-bootstrap-contract.yml
├── apk-evidence-gate.yml
├── rafaelia_e2e_product_proof.yml
└── vectra-grade-benchmarks.yml
```

Gate:

```bash
python3 scripts/audit_github_actions_refs.py \
  --reachable-from .github/workflows/00_START_HERE.yml \
  --require-immutable
```

Every external `uses:` reference in that closure must be a full 40-hex commit
SHA. Local reusable-workflow references remain local edges and must resolve to an
existing YAML file. Missing edges fail closed.

This is a **source/supply-chain custody gate**. It is not workflow execution
evidence, provider enforcement evidence or physical-device evidence.

## Boundary matrix

| Layer | Allowed meaning | Current rule |
|---|---|---|
| `PURE_CORE` | deterministic authorial C/ASM semantics | no libc/heap/syscall/JNI/I/O/platform decision under its owning contract |
| `PLATFORM_GATE` | ELF/ABI/syscall leaf where required | explicit, replaceable, never relabeled as pure core |
| `HOSTED_BOUNDARY` | Android/JNI/POSIX/filesystem/process integration | explicit adapter with evidence and rollback |
| `CI_PROVIDER` | GitHub-hosted orchestration/tooling | external by definition; pin identities, minimize downloads, preserve receipts |
| `PHYSICAL_DEVICE` | actual handset execution | separate same-artifact receipt; never inherited from CI |

## Current dependency reductions

1. External Actions in the front-door closure are pinned by exact commit identity.
2. The reachable runner line is normalized to `ubuntu-24.04` rather than
   `ubuntu-latest`.
3. Vectra contract tests no longer install `pytest` dynamically. The two current
   contract modules are plain zero-argument `test_*` functions and execute through
   `scripts/run_plain_test_functions.py`, which uses only Python stdlib and rejects
   parameterized/framework-dependent tests instead of guessing their semantics.

The third item removes a network/package dependency from this gate; it does **not**
make Python or the hosted runner freestanding.

## External boundaries intentionally retained

The following remain external/hosted and must not be hidden:

- GitHub provider and hosted runner;
- JDK distribution acquisition;
- Android SDK/NDK/toolchain acquisition;
- Gradle/toolchain execution;
- artifact publication service and its quota;
- filesystem/process/clock services used by hosted adapters;
- provider branch/ruleset enforcement;
- physical Android installation/runtime.

Each retained dependency is a replacement candidate only when behavior,
provenance, licensing, rollback and falsifiers are preserved.

## Outside the front-door closure

Historical, specialist and alternate release workflows are **not promoted** by
this gate. Their immutable-reference state remains independently auditable. They
must be migrated by bounded successor changes rather than by a repository-wide
blind rewrite.

In particular:

`front-door closure PASS != all repository workflows immutable`.

## 6Σ / DMAIC execution discipline

- **Define:** front-door closure, CTQ = immutable external identity + truthful hosted boundary.
- **Measure:** discovered closure, references, runner labels, dynamic package acquisition.
- **Analyze:** separate source defect, provider failure, artifact quota and physical evidence.
- **Improve:** smallest behavior-preserving dependency reduction.
- **Control:** exact-head tests, receipts, rollback and append-only successor history.

This is a work method, not a claim of statistical Six Sigma certification.

## Promotion gates

```text
SOURCE
  -> reference/topology audit
  -> workflow execution
  -> produced artifact + digest
  -> provider receipt
  -> physical same-artifact execution
  -> evidence interpretation
  -> CLAIM
```

No arrow may be skipped by inference.

## R3

`F_ok`: front-door dependency boundary is explicit and machine-auditable.

`F_gap`: exact-head CI, provider enforcement, artifact quota, physical runtime and
repository-wide historical workflow migration remain independent.

`F_next`: consume exact-head CI; if source gates pass, migrate the next smallest
outside-closure dependency family without weakening provider or evidence gates.
