# Vertical Slice v1 — Orchestrator Contract

## Pipeline

```
ConversationChunk(s)
       │
       ▼
  ContextBundle          ← assembles chunk refs + working_directory
       │
       ▼
   IntentIR              ← typed, schema-validated intent
       │
       ▼
  Governance Gate        ← capability allowlist + policy rules
       │  (blocked / sandbox_only / human_review → STOP)
       │  (allow → continue)
       ▼
  ExecutionPlan          ← ordered ToolRequest list
       │
       ▼
  Execution Engine       ← subprocess, no shell, no env leak
  (git status / git diff --stat only in v1)
       │
       ▼
  ExecutionResult        ← sha256-stamped, timestamped, immutable
       │
       ▼
  Audit Register         ← append-only log entry
```

## Contracts (schemas in docs/contracts/)

| Stage | Schema file |
|---|---|
| Federated Atlas/NOVO context | `atlas_llm_context_envelope.schema.json` |
| Chunk input | `conversation_chunk.schema.json` |
| Context assembly | `context_bundle.schema.json` |
| Intent IR | `intent_ir.schema.json` |
| Tool invocation | `tool_request.schema.json` |
| Execution plan | `execution_plan.schema.json` |
| Audit output | `execution_result.schema.json` |

The federated envelope is an **additive pre-context stage**. It does not replace `ContextBundle` or `IntentIR`.

```text
ATLAS:X -> NOVO:X -> L:X -> adapters -> AtlasLLMContextEnvelope
                                             |
                                             v
                                      ContextBundle
                                             |
                                             v
                                          IntentIR
                                             |
                                             v
                                      Governance Gate
```

## Atlas/NOVO LLM context extension

Canonical contract:

- `docs/contracts/ATLAS_NOVO_LLM_NAVIGATION_CONTRACT_V1.md`
- `docs/contracts/ATLAS_NOVO_LLM_NAVIGATION_CONTRACT_V1.json`
- `docs/contracts/atlas_llm_context_envelope.schema.json`

Route semantics:

```text
ATLAS:X = choose authority and route
NOVO:X  = inspect NOVOexport/JSON source first
L:X     = recover longitudinal predecessors/deltas
LEARN:X = append learning delta; never rewrite predecessor
```

Canonical V1 model/memory backend is `LLAMA_LOCAL_RMRCTI` through the existing `llamaRafaelia/rmrCti` CTI-memory route. External GPT/provider binding, GAIA runtime adapter, Private bounded adapter, RLL image replay and Vectras device execution remain separately gated.

This extension preserves:

```text
retrieval != training
model_output != evidence
private_pointer != public_disclosure
measured_delta_p != attractor
visual_similarity != physical_equivalence
TOKEN_VAZIO != 0
```

No model call gains execution capability. Context selection happens before the existing Governance Gate; execution remains governed by the same allowlist/policy contracts.

## Governance

- Capability allowlist: `internal/governance/capabilities.json`
- Policy rules: `internal/governance/policy.json`
- Default: **BLOCKED** (deny-by-default)
- Free text → shell: **BLOCKED** (no exceptions in v1)

## v1 Invariants

1. Only `git status` and `git diff --stat` may execute.
2. `read_only: true` must be set on every ToolRequest.
3. Every execution produces an `execution_result.json` with sha256 hashes.
4. No heap allocation, no network call, no write to repository.
5. `rollback_available` is always `false` for read-only commands.
6. Atlas/NOVO retrieval cannot bypass the Governance Gate.
7. Derived context cannot overwrite source identity or evidence state.

## TOKEN_VAZIO Gaps (v1)

| Gap | Description |
|-----|-------------|
| TV-01 | Live LLM invocation to extract IntentIR from ContextBundle; Atlas/LLM context shape is now contracted, runtime binding remains open |
| TV-02 | Signature/attestation of ExecutionResult |
| TV-03 | Append-only audit log backend (file or DB) |
| TV-04 | Cross-repo evidence linking runtime; route/pointer contract now includes llamaRafaelia, NOVOexport, GAIA, Rafaelia_Private, RLL and optional Vectras |
| TV-05 | Rollback mechanism for write-capable v2 |
| TV-06 | Live `ATLAS:X -> NOVO:X -> L:X -> LEARN:X` adapter execution and replay receipt |

## Falsification Condition

The pipeline is correctly implemented if and only if:
- A valid `intent_ir.json` with `execution_gate: "allow"` and
  `requested_capabilities: ["git.read"]` produces an `execution_result.json`
  with `exit_code: 0`, non-empty `stdout_sha256`, and `final_state: "success"`.
- An invalid intent (wrong schema, blocked capability, or `risk: "critical"`)
  is rejected **before** any subprocess is spawned.
- A federated context envelope that violates source/private/claim constraints is rejected or carries an explicit `TOKEN_VAZIO`/blocked state before model interaction.
