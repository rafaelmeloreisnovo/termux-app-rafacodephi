# RAFAELIA — Tri-Repo Mission Cohesion V1

**Contract ID:** `RAFAELIA-TRI-REPO-MISSION-COHESION-V1`  
**Canonical mission contract:** `docs/contracts/mission_execution_boundary.v1.json`  
**Status:** `SOURCE_COHESION_DEFINED / EXTERNAL_GATES_OPEN`  
**claim_allowed:** `false`  
**weight_training_authorized:** `false`

## Pinned source baselines

- `termux-app-rafacodephi@db95f0a64b1d28d1d50b70ab44bc2571fabef29d`
- `Vectras-VM-Android@89e47837e551a565ceb7158cba1ced6954e72d93`
- `RafPolimata@3956bc6a7d7527ce3535661d170507c761b27fcb`

These pins identify the source state inspected when this cohesion contract was created. They are not runtime, device, provider or scientific evidence.

## Authority split

### termux-app-rafacodephi
Owns local Android/Termux orchestration, bounded governance, provider binding and authorized execution-plan composition. It may continue already-authorized work and emit receipts, but it cannot invent mission goals, external authority, provider permissions, reviewer approval, scientific promotion or model-weight training authority.

### Vectras-VM-Android
Is an optional governed execution/runtime backend. It may consume a bounded authorized request and emit runtime evidence. It does not own mission semantics, corpus semantics, model-training authority, external authorization or scientific claim promotion.

### RafPolimata
Owns analysis/compiler/freestanding/research-validation patterns and bounded evidence-producing gates in its declared scope. It may produce source/build/test evidence and reusable contracts. It does not gain authority to execute user actions, grant provider/legal/repository authority, promote scientific claims, or update model weights merely because an implementation or validation artifact exists.

## Non-equivalences

```text
RETRIEVAL_CONTEXT            != WEIGHT_UPDATE
LEARN_APPEND_ONLY            != ONLINE_SELF_TRAINING
CONTINUE_APPROVED_SCOPE      != AUTONOMOUS_GOAL_CREATION
TOKEN_VAZIO                  != 0
SOURCE                       != EXECUTION
EXECUTION                    != EVIDENCE
EVIDENCE                     != CLAIM
DATASET_INFORMS              != MISSION_AUTHORITY
IMPLEMENTED                  != AUTHORIZED
REMOTE_IDENTITY              != RUNTIME_PROOF
```

## Open gates — not source gaps

```text
Android/Termux físico          = TOKEN_VAZIO_DEVICE
multi-repo runtime real        = TOKEN_VAZIO_EXECUTION
identidade remota              = TOKEN_VAZIO_RUNTIME
autorização provider/legal     = TOKEN_VAZIO_EXTERNAL_AUTHORITY
ruleset live                   = TOKEN_VAZIO_EXTERNAL_AUTHORITY
server-side enforcement        = TOKEN_VAZIO_EXTERNAL_AUTHORITY
promoção manual                = TOKEN_VAZIO_MANUAL_AUTHORITY
CodeScan credencial/análise    = TOKEN_VAZIO_SECRET
treino/fine-tuning de pesos    = NÃO AUTORIZADO
scientific claim promotion     = false
claim_allowed                  = false
```

No repository may locally convert any state above to PASS without the evidence/authority type named by that gate.

## Allowed continuation

- source-first audit of already-approved scope;
- minimal reversible hotfixes for observed defects;
- bounded build/test/evidence gates;
- append-only receipts and successor records;
- independent safe work lanes when an external gate is blocked;
- cross-repo pointer/index updates that preserve authority separation.

## Forbidden promotion

- treating a source implementation as physical execution;
- treating CI/build evidence as device proof;
- treating model output or retrieval as evidence of correctness;
- treating a repository role as provider/legal authority;
- silently changing model weights;
- creating new goals from a blocked lane;
- bypassing a ruleset, provider, secret, reviewer or manual-approval gate;
- promoting a scientific claim without its own scoped evidence and review path.

## Append-only succession

A correction must create a successor that points to the superseded record. Existing receipts are not rewritten to hide prior uncertainty or failure.

`LEARN` records:

```text
observation -> authorized action -> result/evidence -> F_ok -> F_gap -> F_next -> successor receipt
```

`LEARN` does not update model weights.

## R3

- `F_ok`: mission authority, runtime authority and research/validation authority are explicitly separated across the three repositories.
- `F_gap`: physical Android/Termux execution, real multi-repo runtime identity, provider/legal/ruleset/server enforcement, manual promotion and secret-backed CodeScan remain externally gated.
- `F_next`: execute only the next gate for which the required physical environment or external authority is actually present; otherwise preserve the typed `TOKEN_VAZIO` and continue independent safe lanes.
