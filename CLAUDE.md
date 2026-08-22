# CLAUDE.md — Claude Code adapter for termux-app-rafacodephi

@AGENTS.md
@docs/00_BUG_MASTER_INDEX.md
@STATUS.md

This file is a Claude Code adapter, not a second source of architectural truth.
The repository-wide contract is `AGENTS.md`; detailed bug analysis is in `docs/00_BUG_MASTER_INDEX.md`.

## Session start

Before editing:

1. Read `AGENTS.md` and `docs/00_BUG_MASTER_INDEX.md` for critical bugs and **mandatory resolution order**.
2. Read `STATUS.md` for current cycle and release profile state.
3. Inspect branch, HEAD and working tree:
   ```sh
   git branch --show-current
   git rev-parse HEAD
   git status --short
   ```
4. **Identify bug dependencies:** BUG-02 → BUG-01 → BUG-03 → BUG-08 is a blocking cascade.
5. Do not merge without explicit human authorization.

## Project orientation

termux-app-rafacodephi is a fork of termux/termux-app integrating RAFAELIA runtime components:

- `src/native/` — Assembly and native code (vectra_pulse.S, JNI bridges)
- `src/attractor_table.c` — Mathematical invariants (42-state Lyapunov dynamics)
- `src/lyapunov_validator.c` — Convergence assertion φ = (1-H)·C
- `src/bootstrap/` — Package selection, initialization, hardcoded values
- `tests/` — Contract tests, gate validation
- `.github/workflows/` — CI orchestration (currently incomplete observability)

**Do not reduce the whole repository to one subsystem.** Bug resolution must follow the documented cascade; parallelization is only safe for independent bugs (BUG-04, 05, 07).

## Critical truth corrections

### BUG-02: Attractor #22 VOID paradox

**Status:** BLOCKING all downstream work (BUG-01, BUG-03, BUG-08)

**The problem:** Mathematical invariant violation — attractor #22 has no canonical state encoding. Must be resolved before completing attractor_table.

**4 documented resolution options:**
1. **Remove #22** (break mirror symmetry; cost: 1 algorithm reversion)
2. **Redefine #22 as proxy** (preserve duality; cost: 3-month verification)
3. **Split into two attractors** (break period=42; cost: invariant invalidation)
4. **Extend phase space** (add dimension; cost: orthogonality unproven)

**Human decision required:** Choose option + justification. Cannot proceed without explicit choice.

### BUG-01: Attractor table 40/42 missing

**Status:** BLOCKED on BUG-02 decision

**The problem:** Only 40/42 attractors defined. Remaining 2 depend on BUG-02 resolution.

**Closure criteria:**
- All 42 attractors defined and encoded
- `gcd(Δr, 42) = 1` coprimality validated
- `period(BitOmega) = 42` verified
- Table SHA-256 hash recorded
- Gate `make attractor-table-complete-gate` passes

### BUG-03: Vectra pulse AArch64 (4 ASM bugs)

**Status:** BLOCKED on BUG-01 completion

**4 enumerated AArch64 issues** (from docs/00_BUG_MASTER_INDEX.md):
1. Register allocation (depends on attractor_table validation)
2. Page alignment 16KB (Android 15+)
3. Memory barrier (cache coherency)
4. [4th issue per STATUS.md]

**Closure criteria:**
- All 4 ASM bugs fixed
- Golden test with frequency validation
- Physical ARM64 device validation
- Gate `make aarch64-vectorpulse-gate` passes

### BUG-08: Lyapunov invariant φ = (1-H)·C

**Status:** BLOCKED on BUG-01/03 completion

**The invariant:** φ = (1 - H)·C where H=entropy, C=coherence. Must satisfy 0 ≤ φ ≤ 1 (convergence bounds).

**Closure criteria:**
- φ computed at each gate
- Assertion `φ ∈ [0, 1]` enforced
- Receipt with H, C, φ values logged
- Gate `make lyapunov-convergence-gate` passes

### Do NOT claim "42 fixed-point attractors"

**Correct:** "42-state phase space with encoded attractor regions"  
**Incorrect:** "Mathematical proof of 42 dynamical fixed points"

The value 42 is a bounded construction parameter in the implementation. It does not imply a theorem about 42 distinct physical attractors.

### Current-commit evidence

Do not promote source/binary/APK existence into device-runtime proof.

```text
source code (in src/)
  != compiled ARM32/ARM64 artifact
  != APK packaging
  != device signature verification
  != app launch on physical Android
  != runtime execution receipt (logcat)
```

Each missing link remains TOKEN_VAZIO.

## Coding discipline

- **Bounds checks:** Manifest entries, buffer sizes, state indices explicit
- **Error paths:** NULL checks and error returns always explicit
- **Binary layouts:** Never alter ZrManifest or ELF segment layouts silently
- **ASM changes:** Golden tests + frequency validation required for vectra_pulse.S
- **Gate failures:** Never suppress with `|| true` or unconditional success
- **Invariant validation:** φ computation and assertion must not be optimized away

## Independent parallel bugs

These can be resolved in parallel without blocking each other:

| Bug | Type | Effort | Gate |
|-----|------|--------|------|
| **BUG-04** | Package hardcode | 1 day | Migrate `com.termux` → config |
| **BUG-05** | Stack overflow | 1 day | Move ZrManifest off stack → heap |
| **BUG-07** | Hash mismatch | 1 day | Add `exit 1` on BLAKE3 fail |

## Release profiles (reference state)

| Profile | Status | Blockers |
|---------|--------|----------|
| **safe-core** | Candidate closure | BUG-05, manifest audit, gate validation |
| **functional-distribution** | BLOCKED | CI observability, signing, device receipts, runtime-lock.json |
| **full-platform** | BLOCKED | Research phase, VCPU→VM promotion pending |

## Useful entrypoints

```sh
# Critical bug analysis:
cat docs/00_BUG_MASTER_INDEX.md
cat STATUS.md

# Bug resolution gates:
make attractor-coherence-gate        # BUG-02 decision point
make attractor-table-complete-gate   # BUG-01 closure
make aarch64-vectorpulse-gate        # BUG-03 closure
make lyapunov-convergence-gate       # BUG-08 closure
```

## Handoff

Finish with:

```text
F_ok   = what was actually changed/executed/demonstrated
F_gap  = what remains unknown, blocked, contradicted or unexecuted
F_next = smallest reproducible next action
```

**Important:** BUG-02 decision is a human gate. Do not bypass it. TOKEN_VAZIO cascades remain visible.
