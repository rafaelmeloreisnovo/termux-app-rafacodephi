---
name: runtime-pattern-leaf
description: Apply a reusable architecture pattern to the Termux Android runtime/provider role while preserving package identity, ABI support, provider boundaries, privacy, and execution-evidence semantics. Use when a prior language, database, event system, storage layout, OS resource map, or runtime suggests a safer state, trigger, validation, logging, allocation, or failure mechanism.
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

## Event and durability mapping

When an older database architecture exposes create/add, update, delete and commit, map them to explicit runtime/service transitions rather than implicit side effects.

```text
request
-> validate
-> stage/dispatch
-> observable execution result
-> durable receipt if required
-> derived current state
```

Dispatch is not commit, and commit is not proof that the requested physical action occurred.

## OS resource and memory geometry

Legacy memory windows, FILES/BUFFERS-like configuration, interrupt/resource maps and address-space tuning transfer only as a general model:

```text
resource/address range
-> owner
-> capability
-> allocation state
-> pressure/conflict
-> release/recovery
```

Android/Linux owns the actual process, virtual-memory, fd, binder/provider and kernel resource semantics. Do not import DOS-era addresses, IRQs or memory limits as current constants.

## Storage locality geometry

Sector/block/fragmentation reasoning may inform current file/archive/bootstrap design as:

```text
artifact
-> logical file/record
-> filesystem allocation/page/cache
-> physical/storage backend
-> measured access cost
```

Prefer sequential/bounded access and deterministic offsets when they reduce work without weakening correctness. Historical HDD/SSD latency recollections are not current performance evidence.

## Protocol and device boundary

Serial/parallel/modem/network examples transfer as state-machine and framing ideas only. Keep physical medium, framing, network/transport and application command layers separate.

Software success does not prove an external electrical or hardware transition without a device-level observation.

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
resource_or_storage_geometry_if_relevant
falsifier
receipt_scope
F_ok
F_gap
F_next
rollback
claim_allowed=false unless a separate exact-scope gate promotes it
```
