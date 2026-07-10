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

| Stage            | Schema file                        |
|------------------|------------------------------------|
| Chunk input      | conversation_chunk.schema.json     |
| Context assembly | context_bundle.schema.json         |
| Intent IR        | intent_ir.schema.json              |
| Tool invocation  | tool_request.schema.json           |
| Execution plan   | execution_plan.schema.json         |
| Audit output     | execution_result.schema.json       |

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

## TOKEN_VAZIO Gaps (v1)

| Gap | Description |
|-----|-------------|
| TV-01 | LLM invocation to extract IntentIR from ContextBundle |
| TV-02 | Signature/attestation of ExecutionResult |
| TV-03 | Append-only audit log backend (file or DB) |
| TV-04 | Cross-repo evidence linking (llamaRafaelia ↔ RafPolimata) |
| TV-05 | Rollback mechanism for write-capable v2 |

## Falsification Condition

The pipeline is correctly implemented if and only if:
- A valid `intent_ir.json` with `execution_gate: "allow"` and
  `requested_capabilities: ["git.read"]` produces an `execution_result.json`
  with `exit_code: 0`, non-empty `stdout_sha256`, and `final_state: "success"`.
- An invalid intent (wrong schema, blocked capability, or `risk: "critical"`)
  is rejected **before** any subprocess is spawned.
