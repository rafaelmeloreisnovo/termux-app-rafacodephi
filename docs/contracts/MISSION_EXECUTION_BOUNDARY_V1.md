# RAFAELIA — Mission Execution Boundary V1

**Contract ID:** `RAFAELIA-MISSION-EXECUTION-BOUNDARY-V1`  
**Execution authority:** `termux-app-rafacodephi` orchestrator  
**Routing authority:** `Mapa / ATLAS:X`  
**Corpus authority:** source repositories + Drive/NOVOexport  
**State:** `CONTRACT_MATERIALIZED / EXECUTION_EVIDENCE_SCOPED`  
**Claim gate:** `claim_allowed=false`

## 1. Purpose

Separate three things that must never collapse into one another:

```text
DATASET / CORPUS
  -> informs context and evidence
MODEL
  -> retrieves / interprets / proposes under the active mission
PROGRAM / ORCHESTRATOR
  -> executes the user-authorized mission through typed gates and receipts
```

The corpus is not an instruction for uncontrolled self-training. The model is not an
independent goal owner. The active mission is the explicit execution boundary.

```text
dataset informs mission execution != dataset becomes mission
retrieval/context != weight update
LEARN:X append != online self-training
continuation within approved scope != autonomous goal creation
```

## 2. Canonical execution route

```text
MISSION
  -> ATLAS:X
  -> NOVO:X source-first
  -> L:X predecessor recovery
  -> O:X independent evidence axes
  -> T:X permitted transverse adapters
  -> REL:X typed relations
  -> EVID:X evidence normalization
  -> GAP:X TOKEN_VAZIO classification
  -> AtlasLLMContextEnvelope
  -> ContextBundle
  -> IntentIR
  -> Governance
  -> ExecutionPlan
  -> authorized mutation / test / read
  -> ExecutionResult
  -> provenance receipt
  -> LEARN:X append-only successor
  -> repeat remaining approved work
```

The loop may continue across independent work lanes when one lane is externally blocked.
A blocked gate does not authorize bypass and does not terminate unrelated safe lanes.

## 3. Mission object

Every substantial execution cycle SHOULD be representable as:

```text
MissionExecution =
  <mission_id,
   user_authorized_scope,
   authorities,
   source_refs,
   predecessor_refs,
   invariants,
   allowed_actions,
   forbidden_promotions,
   evidence_requirements,
   open_gaps,
   external_gates,
   rollback_refs,
   execution_receipts,
   learn_successor>
```

### Required invariants

1. `SOURCE != DERIVED_INDEX != EXECUTION != EVIDENCE != CLAIM`.
2. `TOKEN_VAZIO != 0` and missing evidence is never synthesized.
3. Raw NOVOexport/source evidence is read-only unless that source is itself the explicitly authorized edit target.
4. Repository mutations bind an exact base SHA and retain rollback provenance.
5. Existing debt cannot justify new debt on changed surfaces.
6. A green gate promotes only the scope that it measured.
7. External authority requirements cannot be simulated by the orchestrator.
8. Training or fine-tuning weights remains disabled unless separately and explicitly authorized with its own reproducible contract.

## 4. Continuation semantics

`CONTINUE_APPROVED_SCOPE` means:

- keep processing already authorized work items;
- choose another independent lane when one lane is blocked;
- apply minimal reversible hotfixes supported by source/evidence;
- update documentation, indexes, receipts and longitudinal state with the same evidence boundary;
- never invent a new product goal, permission, reviewer, credential, device proof or scientific conclusion.

It does **not** mean:

- self-modifying mission goals;
- silent model training;
- bypassing repository/provider governance;
- treating an absent runner, secret, ruleset, reviewer or physical device as PASS;
- merging because implementation merely exists.

## 5. Dataset and model boundary

The existing ATLAS/NOVO contract remains authoritative for data modes:

```text
RETRIEVE_READ_ONLY      = allowed when source/provenance rules pass
CURATE_DERIVED_INDEX    = allowed append-only with source identity
TRAIN_OR_FINETUNE       = disabled unless a separate explicit contract is authorized
```

Model output is an execution input, not execution evidence. A proposed patch becomes an
observed implementation only after repository mutation. It becomes build/runtime evidence
only after the corresponding gate actually runs and emits an attributable receipt.

## 6. Failure and blocking states

Use typed states instead of one global failure:

```text
SOURCE_BLOCKED
IMPLEMENTED_PENDING_GATE
REMOTE_EXECUTION_TOKEN_VAZIO
EXTERNAL_AUTHORITY_REQUIRED
RUNTIME_TOKEN_VAZIO
PHYSICAL_DEVICE_TOKEN_VAZIO
INDEPENDENT_REVIEW_REQUIRED
PROVEN_WITHIN_SCOPE
```

When a gate is `EXTERNAL_AUTHORITY_REQUIRED`, record the blocker and continue independent
safe work. Do not weaken the gate to manufacture closure.

## 7. Priority / urgency

Within the authorized mission, choose work in this order unless a narrower contract says otherwise:

1. real correctness/safety defects with localized evidence;
2. provenance, custody, rollback and fail-closed gaps;
3. broken execution/build/runtime bindings;
4. anti-regression ratchets for inherited debt;
5. documentation/index drift that can misroute execution;
6. style/redundancy debt that does not affect correctness.

## 8. LEARN:X semantics

`LEARN:X` is an append-only operational learning layer:

```text
observation
 -> action
 -> result/evidence
 -> F_ok
 -> F_gap
 -> F_next
 -> successor receipt/index
```

It does not imply neural-weight mutation. If weight training is later authorized, that is a
different execution class with explicit dataset manifest, privacy/license review, split,
model identity, hyperparameters, compute/device identity and reproducible training receipt.

## 9. Current federation interpretation

For the Vectras / RAFCODEPHI / llamaRafaelia / GAIA_phi / Rafaelia_Private federation:

- Termux owns orchestration, not scientific truth;
- Mapa owns routing/state, not producer implementation claims;
- LLaMA/RMRCTI supplies local model/retrieval surfaces, not mission authority;
- GAIA supplies bounded retrieval experiments/adapters, not trained-model proof;
- Vectras supplies optional execution/runtime backend, not source semantics;
- Rafaelia_Private supplies protected implementation authority, not public bulk context;
- MemRafcode/Drive custody preserve longitudinal succession and provenance.

## 10. Completion condition

A mission cycle is `FINISHED_WITH_EXTERNAL_GATES` when all currently executable authorized
work has either:

- `PROVEN_WITHIN_SCOPE`, or
- a concrete `TOKEN_VAZIO` / `EXTERNAL_AUTHORITY_REQUIRED` state with the next verifiable transition recorded.

This is not equivalent to claiming every runtime or scientific hypothesis is proven.
It means the program has exhausted the safe actions available under the current authority
without bypass, regression, invented evidence or silent scope expansion.
