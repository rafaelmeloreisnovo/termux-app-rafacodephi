---
name: runtime-pattern-leaf
description: Apply a reusable architecture pattern to the Termux Android runtime/provider role while preserving package identity, ABI support, provider boundaries, privacy, and execution-evidence semantics. Use when a prior language, database, event system, or runtime suggests a safer state, trigger, validation, logging, or failure mechanism.
---

# Android Runtime / Provider Pattern Leaf

Read `AGENTS.md` first. This skill is subordinate to the repository runtime/provider contract.

## Local projection

```text
extracted mechanism
-> Android/provider local problem
-> explicit state + trigger + guard
-> minimum runtime delta
-> bounded request/result behavior
-> local falsifier
-> runtime receipt
```

Reference transfers:

- Pascal-like discipline -> explicit numeric widths/ranges, initialization, state typing, deterministic error/result states and no silent coercion where the local contract requires precision.
- InterBase-like trigger discipline -> explicit provider/service event activation, guarded transitions, bounded logging and deterministic post-event actions.

Use the mechanism, not the old syntax.

## Preserve

- canonical package `com.termux.rafacodephi`;
- `armeabi-v7a`; do not silently collapse to ARM64-only;
- discovery, dispatch, process execution, exit and guest behavior as distinct claims;
- minimum-necessary IPC/request/result data;
- rollback for high/critical mutation;
- `device_validation=TOKEN_VAZIO` until an exact device/runtime receipt closes it.

## Forbidden transfer

```text
provider dispatch != process execution
CI build != physical Android runtime
bootstrap materialization != pkg runtime proof
pattern similarity != evidence
skill != authority
```

Private reconstruction may inform the mechanism, but raw private prompts/corpus/locators must not be copied into this public repository.

## Completion

Record:

```text
source_pattern
runtime_leaf
local_state_or_trigger
falsifier
receipt_scope
F_ok
F_gap
F_next
rollback
claim_allowed=false unless a separate exact-scope gate promotes it
```
