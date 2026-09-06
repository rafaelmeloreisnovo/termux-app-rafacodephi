# Atlas × LLaMA × Governance Bridge V1

Status: `CONTRACT_WIRED_PROVIDER_GIT_PINNED_LOCAL_MODEL_EVIDENCE_PENDING`

## Closed route

```text
ATLAS:X
  -> NOVO snapshot
  -> L:X
  -> RMR-CTI retrieval
  -> ContextBundle + bounded chunks
  -> pinned llamaRafaelia IntentIR provider
  -> Termux local Governance Gate
  -> governed IntentIR
  -> existing fixed read-only Vertical Slice (only after explicit operator approval)
  -> immutable ExecutionResult
  -> LEARN:X / receipt chain
```

The bridge does **not** execute the final read-only runner. This is deliberate separation of concerns: model proposal, governance authorization, execution, and evidence remain distinct artifacts.

## Provider binding

`tools/atlas_llama_governance_bridge.py` requires both a provider file path and its exact SHA-256. A changed provider cannot run under a stale runtime pin.

The canonical provider is maintained in `rafaelmeloreisnovo/llamaRafaelia` as:

```text
rmrCti/atlas_intent_provider_v1.py
```

The merged repository identity is pinned by `ATLAS_LLAMA_PROVIDER_PIN_V1.json` to commit:

```text
7a667531b3411c63349a22b24ed5a7d7a314f79a
```

This closes the repository-level provider identity gap. The bridge independently requires the exact provider file SHA-256 at invocation time, so a local file cannot gain authority merely by claiming the pinned Git commit.

## Authority boundaries

1. Atlas/NOVO/RMR-CTI selects bounded context; it does not grant execution.
2. LLaMA converts selected data into a typed proposal; its target path, permissions, evidence claims and gate decisions are not trusted.
3. Termux owns capability policy and recomputes the gate from local policy documents.
4. The operator/user approval is an explicit bridge input; it is never inferred from model text.
5. The existing executor remains a separate stage and produces separate immutable receipts.

## Fixed-plan capability invariant

The legacy V1 runner always executes both provenance/status reads and `git diff --stat`. Therefore the federated strict gate requires exactly:

```json
["git.read", "git.diff"]
```

An intent requesting only `git.read` is **blocked** for this fixed plan because the executed command set would otherwise exceed the declared capability set. Extra capabilities are also blocked. This closes capability under-declaration without changing the legacy runner interface.

## Decisions

- missing/changed provider -> `blocked`
- missing local LLaMA binary/model -> `TOKEN_VAZIO`
- no explicit operator approval -> `human_review`
- unknown, extra or missing fixed-plan capability -> `blocked`
- exact read-only capabilities + local allow classification + explicit approval -> `allow`

`allow` means **authorized for the next executor stage**, not “executed”. The bridge receipt keeps `execution_performed=false`.

## Evidence states

```text
CONTRACT_WIRED
-> CI_FALSIFIERS_PASS
-> PROVIDER_PINNED
-> LOCAL_MODEL_RUNTIME_OBSERVED
-> GOVERNANCE_RUNTIME_OBSERVED
-> READONLY_EXECUTION_OBSERVED
-> DEVICE_REPRODUCED
```

Do not collapse these states. In particular:

```text
model output != permission
permission != execution
execution != device reproduction
absence of evidence != PASS
```

## TOKEN_VAZIO retained

- `TV-LLAMA-LOCAL-MODEL-RUNTIME` until exact binary/model hashes and runtime receipt exist
- `TV-LLAMA-DEVICE-REPRODUCTION` until physical Termux reproduction exists
- `TV-ATLAS-END-TO-END-MODEL-CAUSAL-USE` until an off/on context causal test is captured
