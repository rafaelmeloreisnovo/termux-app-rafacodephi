# COPILOT ASSIGNMENT — PHASE 2

Use the block below with GitHub Copilot Coding Agent after RafPolimata Phase 1 has established the final segment/CLI contract.

---

## PROMPT

You are working in `rafaelmeloreisnovo/termux-app-rafacodephi`.

Your task is to implement **Phase 2: RAFAELIA Android runtime service, durable worker queue and artifact publication**.

Read completely before editing:

1. `.github/copilot-instructions.md`
2. `docs/copilot/TASK_03_RUNTIME_SERVICE_AND_WORKER_QUEUE.md`
3. `scripts/rafaelia/run_index_conversations.sh`
4. `.github/workflows/rafaelia-runtime-runner-ci.yml`
5. the current Android manifest, application/package constants, foreground-service implementation, process helpers, bootstrap paths and service tests
6. the final RafPolimata Phase 1 CLI/segment contract and `include/rafaelia_runtime_protocol.h`
7. `RafGitTools/schemas/rafaelia_runtime_job.schema.json`

First inspect the real repository and write a reconnaissance section in the PR description. Do not invent package paths. Reuse existing service, storage, notification and process infrastructure where coherent.

Implement a working, versioned, explicit Android service boundary for typed RAFAELIA jobs. The service must accept a read-only source descriptor, validate the runtime job, stage the source by streaming, persist the job, queue it, execute the pinned RafPolimata tool, publish validated artifacts atomically and expose progress/cancellation/read-only artifacts to RafGitTools.

Mandatory features:

- explicit Android component;
- app-controlled permission and caller validation;
- protocol version negotiation;
- bounded request payload;
- stable operation enum, starting with `index_conversations`;
- no generic command-text API;
- durable state machine;
- duplicate identical-job handling and conflicting-job rejection;
- fixed-buffer staging with 64-bit sizes;
- private job directories derived from validated job IDs;
- bounded queue, default single active worker on low-memory devices;
- foreground service and cancellation notification;
- exact worker binary version/hash recording;
- controlled worker environment and separate arguments;
- progress events with bytes, records and checkpoint;
- cancellation before/during execution;
- process-death recovery;
- checkpoint/source identity validation;
- atomic `output.tmp -> output` publication;
- manifest/segment/hash validation before `COMPLETED`;
- read-only access only to an artifact allowlist.

Use the persisted states defined by the repository instructions. Every state transition must be audit-recorded. Never expose incomplete output as complete.

Test at minimum:

- valid request and source descriptor;
- invalid version, operation and policy;
- caller validation failure;
- oversized request;
- source read error/size mismatch;
- insufficient storage;
- duplicate identical job;
- conflicting duplicate ID;
- queue order and concurrency lock;
- cancellation queued/running;
- worker failure;
- process restart during staging/running/publication;
- invalid checkpoint or changed source identity;
- artifact allowlist and path traversal rejection;
- callback correlation and client death;
- foreground-service lifecycle;
- existing shell runner contract.

CI must build the relevant APK, compile AIDL, run unit/lint/service tests, run the runner contract and publish reports/hashes. Do not weaken existing workflows and do not claim physical-device proof unless it actually ran.

Do not stop at stubs or interfaces. Implement the full service vertical, run tests, fix failures and open a PR only after the focused CI is green. Document unresolved external dependencies as `TOKEN_VAZIO`, but do not replace them with false success.

PR description must include:

- current files inspected;
- service/API version and exact operations;
- state-transition table;
- private storage layout;
- memory and disk limits;
- worker invocation/version/hash;
- commands executed;
- CI results;
- security boundary;
- recovery evidence;
- remaining `TOKEN_VAZIO` items;
- exact handoff requirements for RafGitTools.

Execute the defined architecture; do not redesign it into a generic terminal or cloud service.

---
