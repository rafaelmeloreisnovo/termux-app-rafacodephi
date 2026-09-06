# RAFAELIA — ATLAS × NOVOexport × LLM Navigation Contract V1

**State:** `CONTRACT_MATERIALIZED / EXECUTION_PENDING`  
**Target authority:** `termux-app-rafacodephi`  
**Model/memory authority:** `rafaelmeloreisnovo/llamaRafaelia/rmrCti`  
**Claim gate:** `claim_allowed=false`

## 1. Purpose

Define a source-first, append-only contract for a local LLM to **navigate and interact with** the RAFAELIA corpus without copying authority, silently training on private data, or collapsing retrieval into scientific proof.

The canonical route is:

```text
ATLAS:X
  -> choose authority/route
NOVO:X
  -> inspect NOVOexport/JSON source first
L:X
  -> recover longitudinal predecessors/deltas
LEARN:X
  -> append learning delta without rewriting predecessors
  -> ContextBundle
  -> EvidenceBundle
  -> local model interaction
  -> receipt
```

This V1 chooses **Termux as the orchestration owner** because the repository already contains the typed `ContextBundle -> IntentIR -> Governance Gate -> ExecutionPlan -> ExecutionResult` vertical slice and the local package/runtime surface. Vectras remains an optional runtime/VM consumer through its existing Termux provider contract.

## 2. Existing authorities reused, not duplicated

| Domain | Authority | Role in this contract |
|---|---|---|
| Route/authority/gates | `rafaelmeloreisnovo/Mapa` | `ATLAS:X`, evidence and route selection |
| Raw conversational corpus | Google Drive `NOVOexport` | read-only source; IDs/hashes/pointers are preferred over body copies |
| LLM + CTI long-term memory | `rafaelmeloreisnovo/llamaRafaelia/rmrCti` | deterministic scan, curators, retrieval, `llama-server --cti-memory` |
| Local orchestrator | `rafaelmeloreisnovo/termux-app-rafacodephi` | ContextBundle, IntentIR, governance, local interaction |
| Optional VM/runtime | `rafaelmeloreisnovo/Vectras-VM-Android` | execution backend only; does not own corpus semantics |
| Voynich protected implementation | `rafaelmeloreisnovo/Rafaelia_Private/native/voynich_impl` | protected source/overlay under Three-Pillars route |
| GAIA memory experiment | `rafaelmeloreisnovo/GAIA_phi` | candidate Nexus/hash/vector retrieval adapter |
| RLL image rigor | `instituto-Rafael/relativity-living-light` | image provenance/falsification gate; not automatic scientific truth |

## 3. Model backend contract

### 3.1 Canonical V1 backend

```text
backend_id = LLAMA_LOCAL_RMRCTI
engine     = llama-server
memory     = --cti-memory on
transport  = localhost only by default
```

The current `rmrCti` contract already distinguishes long-term CTI retrieval from the model KV cache. Retrieval is provenance-bearing context injection; it is not hidden model memory.

### 3.2 Other backends

```text
GPT_OR_OTHER_EXTERNAL_PROVIDER = TOKEN_VAZIO_UNTIL_EXPLICIT_PROVIDER_BINDING
GAIA_NANOGPT                  = RETRIEVAL/DEMO_ONLY
VECTRAS_RUNTIME               = OPTIONAL_EXECUTION_BACKEND
```

`GAIA_phi/gaia_nanogpt.c` may contribute the retrieval path
`semantic_hash_djb2 -> hash_to_vector -> shift_attention -> resolve_memory_content`, but its `nanogpt_generate` path is source-observed as a simulation/demo and MUST NOT be promoted to trained-model inference.

## 4. Data use modes

V1 has three distinct modes:

1. `RETRIEVE_READ_ONLY` — allowed candidate default.
2. `CURATE_DERIVED_INDEX` — allowed only as append-only derived artifact with source IDs/hashes.
3. `TRAIN_OR_FINETUNE_WEIGHTS` — **disabled** until an explicit dataset manifest, privacy/license review, deterministic train/validation/test split, model identity, hyperparameters, device budget and reproducible training receipt exist.

Therefore:

```text
retrieval != training
indexing   != training
context injection != weight update
```

## 5. NOVOexport source-first route

For a user query `X`:

```text
NOVO:X
  -> locate raw JSON/shard/object identity
  -> preserve provider/file/object/conversation/message IDs
  -> preserve byte/hash evidence when available
  -> derive/query CTI artifacts locally
  -> never rewrite raw source
```

Preferred derived chain from `rmrCti`:

```text
raw / zip / conversations.json
 -> rafa_cti_scan.c
 -> *.bitstack.jsonl + CSV
 -> triad_cti_couple.py
 -> *.coupled.jsonl + report
 -> omega_forest / omega_search_fast / omega_frames_export
 -> omega_msgs.jsonl / forest.jsonl / frames_seed.json
 -> llama-server --cti-memory
```

Missing source or missing generated artifact is `TOKEN_VAZIO` and retrieval fails open.

## 6. RMRCTI ΔP ≈ 0.18 boundary

The recurring `ΔP ≈ 0.18` route is admissible as a **provenance-bearing measured feature**, never as a universal constant or attractor by repetition alone.

Required injected label:

```text
[RMRCTI ΔP report: measured association; attractor not established]
```

Allowed use in V1:
- attach a real `rmrcti_delta_p_*` report to evidence;
- use report identity as a filter/facet;
- compare repeated measurements across explicitly named traces.

Forbidden promotion:

```text
recurrent ~= causal
recurrent ~= universal
recurrent ~= dynamical attractor
0.18 observation ~= semantic relevance score
```

If the report artifact is absent, the feature is `TOKEN_VAZIO` and contributes zero ranking authority.

## 7. Toro / TRIAD / geometric layer

The deterministic binary evidence layer remains separate from the interpretative layer:

```text
offset/size/CRC/bit counts/entropy/flips/transitions
  !=
toro/TRIAD/TEXTURE10/semantic interpretation
```

A geometric/toro feature may be attached as a typed feature vector, but cannot overwrite source identity or binary provenance.

## 8. Voynich route

Voynich queries must follow the existing Mapa Three-Pillars route:

```text
P1 SOURCE_PROVENANCE
P2 EXECUTION_REPLAY
P3 INTERPRETATION_CLAIM_BOUNDARY
```

Protected implementation authority remains `Rafaelia_Private/native/voynich_impl`. The orchestrator may receive hashes, stable IDs, derived descriptors and approved excerpts; raw protected bodies are not copied into public routing artifacts.

For image-backed Voynich material:

```text
image source
 -> immutable ID/hash
 -> deterministic descriptor/classification artifact
 -> repeated/recurrent comparison
 -> contradiction/falsifier check
 -> evidence label
 -> optional LLM context
```

## 9. RLL image rigor adapter

RLL contributes the **method boundary**, not an automatic answer. The adapter MUST preserve:

```text
source image != processed image != feature vector != classification != physical claim
visual similarity != physical equivalence
image edge != causal edge
```

A recurrent image classification becomes stronger engineering evidence only when the result is reproducible across named inputs/runs/transforms and provenance is retained. Scientific promotion still belongs to the RLL falsification/evidence gate.

Recommended image evidence tuple:

```text
<ImageID, SourceHash, TransformID, DescriptorVersion,
 ClassLabel, Score, RepetitionCount, Stability, Contradictions,
 Provenance, ClaimState>
```

## 10. GAIA adapter

Candidate read-only GAIA route:

```text
prompt
 -> semantic_hash_djb2
 -> hash_to_vector
 -> shift_attention
 -> resolve_memory_content
 -> ContextFragment
```

V1 restrictions:
- no GAIA source mutation;
- no assertion that one hash/vector is semantic truth;
- no promotion of the demo generator to a trained GPT;
- returned fragments must carry GAIA path/identity and adapter version.

## 11. Rafaelia_Private adapter

Private is a protected authority container, not a bulk context dump.

The adapter returns only a bounded `PrivateContextRef`:

```text
<repository, ref, path, content_hash?, artifact_class,
 disclosure_class, evidence_state>
```

`NAVIGATION.md`/generated indexes should be used before deep reads. A private source may support navigation while its body remains undisclosed.

## 12. ContextBundle extension

This contract does not replace `context_bundle.schema.json`. It defines an additive envelope:

```text
AtlasLLMContextEnvelope =
  <route,
   query,
   source_refs,
   cti_hits,
   image_evidence,
   scientific_boundaries,
   private_refs,
   gaia_refs,
   learning_predecessors,
   token_vazio,
   model_backend>
```

The envelope is converted to a normal `ContextBundle` before `IntentIR`; execution permissions still belong to the existing Governance Gate.

## 13. Query and interaction state machine

```text
Q0_USER_QUERY
 -> Q1_ATLAS_ROUTE
 -> Q2_NOVO_SOURCE_FIRST
 -> Q3_LONGITUDINAL_RECOVERY
 -> Q4_ADAPTER_FANOUT
      {RMRCTI, VOYNICH_PRIVATE, RLL_IMAGE, GAIA, OPTIONAL_VECTRAS}
 -> Q5_EVIDENCE_NORMALIZE
 -> Q6_CONTEXT_BUNDLE
 -> Q7_MODEL_GENERATE
 -> Q8_RESPONSE_WITH_PROVENANCE
 -> Q9_LEARN_APPEND_ONLY
```

Failure states:

```text
TOKEN_VAZIO_SOURCE
TOKEN_VAZIO_BINDING
TOKEN_VAZIO_RUNTIME
CLAIM_BLOCKED
PRIVATE_WITHHELD
CONTRADICTED
```

No failure state is coerced to `0`, `false`, `PASS` or `no evidence`.

## 14. Ranking rule

V1 ranking may combine existing CTI keyword/path/curated signals, but evidence authority is a separate channel.

```text
retrieval_score != evidence_weight != scientific_confidence
```

A suggested normalized tuple is:

```text
R = <keyword_score, curated_bonus, recency_or_longitudinal_fit,
     provenance_quality, reproducibility, contradiction_penalty>
```

No fixed coefficients are canonical in V1. Existing RMRCTI ranking remains authoritative until an executable benchmark justifies a replacement.

## 15. LEARN:X append-only rule

After a completed interaction, only a learning delta may be appended:

```text
<learning_id, predecessor_ids, source_ids, query_hash,
 route_id, selected_context_ids, model_id, response_hash,
 corrections, contradictions, unresolved_token_vazio,
 next_verifiable_step>
```

The model response itself never becomes source authority merely because it was generated.

## 16. Falsification / acceptance gates

### G1 — Route
Same query + same routing indexes selects the same authority set.

### G2 — Source custody
A retrieved fragment is traceable to a source ID/hash or is visibly `TOKEN_VAZIO`.

### G3 — CTI causal-use check
With CTI off/on, a rare held-out fact must change generation only when the relevant retrieved context is injected; unrelated negative control yields `no_hits`.

### G4 — ΔP boundary
No context labels `ΔP≈0.18` as attractor without the stronger evidence gates.

### G5 — Voynich privacy/claim boundary
Private body is not copied to public route; Three-Pillars metadata is preserved.

### G6 — RLL image recurrence
Repeated classification is tied to exact image hashes/transforms/runs; visual similarity alone cannot promote a physical/scientific claim.

### G7 — Learning
A new interaction adds a successor delta and never rewrites its predecessor.

## 17. Current evidence and open gaps

Source-observed now:
- Termux typed orchestrator exists;
- `llamaRafaelia/rmrCti` has deterministic CTI scan/navigation components;
- `llama-server --cti-memory` integration and Termux run guide exist;
- Drive has an Assistant Bridge RMRCTI↔NOVOexport ledger and a GAIA_RMRCTI_MEMORY_BRIDGE folder;
- Mapa has the Voynich Three-Pillars route;
- GAIA has a source-observed hash/vector/Nexus query demo;
- RLL has explicit image provenance/falsification boundaries.

Open gates:
- `TV-ATLAS-LLM-NOVO-LIVE-BINDING`;
- `TV-NOVO-CURRENT-MANIFEST-EXACT-SCOPE`;
- `TV-GAIA-RMRCTI-ADAPTER-RUNTIME`;
- `TV-PRIVATE-BOUNDED-ADAPTER-RUNTIME`;
- `TV-RLL-IMAGE-ADAPTER-REPLAY`;
- `TV-VECTRAS-OPTIONAL-BACKEND-DEVICE-PROOF`;
- `TV-EXTERNAL-GPT-PROVIDER-BINDING`;
- `TV-TRAINING-DATASET-GOVERNANCE-AND-REPRODUCTION`.

## 18. Core invariants

```text
ATLAS route != source body
NOVOexport source != derived CTI index
retrieval != training
model output != evidence
visual similarity != physical equivalence
repetition != causality
measured ΔP != attractor
private pointer != public disclosure
source_present != build_proven != runtime_proven != device_proven
VISÃO != ARTEFATO != EXECUÇÃO != EVIDÊNCIA != CLAIM
TOKEN_VAZIO != 0
```

## 19. Executable successor and ordering clarification

The successor is [`ATLAS_NOVO_CONTEXT_ADAPTER_V1.md`](ATLAS_NOVO_CONTEXT_ADAPTER_V1.md).
The command list in section 1 is a navigation index, not temporal execution
order: `LEARN:X` follows retrieval/interaction and its receipt. The executable
retrieval path is `ATLAS -> NOVO snapshot -> L -> RMRCTI -> ContextBundle -> LEARN`.
The full model path still requires `IntentIR -> Governance -> LLM -> LEARN`.

Section 5's fail-open behavior belongs to ordinary upstream model completion
without memory. The local context adapter fails closed on malformed inputs,
hash drift, ambiguous source identity or producer-binding failure; it never
turns those failures into successful retrieval evidence.
