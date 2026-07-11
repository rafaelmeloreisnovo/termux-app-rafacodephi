# TASK 03 — RAFAELIA RUNTIME SERVICE AND WORKER QUEUE

## Objective

Implement the Android-side runtime service that receives a typed `index_conversations` request from RafGitTools, stages the selected source, runs the pinned RafPolimata indexer and returns validated progress/artifacts.

This task must close a functional vertical. Do not stop at AIDL interfaces or empty service methods.

---

## 1. Mandatory reconnaissance

Before editing, record in the PR description:

- real application ID and package namespace;
- existing foreground service implementation;
- existing Binder/AIDL patterns;
- current app-private path constants;
- bootstrap/runtime executable locations;
- process execution helpers;
- notification channels;
- existing CI workflows and build variants;
- current permission declarations;
- current tests for services/processes.

Do not duplicate an existing subsystem.

---

## 2. Proposed implementation map

Adapt to the repository's actual Java/Kotlin source roots:

```text
app/src/main/aidl/com/termux/rafacodephi/runtime/
  IRafRuntimeService.aidl
  IRafRuntimeCallback.aidl
  RafRuntimeRequest.aidl
  RafRuntimeEvent.aidl
  RafRuntimeArtifact.aidl

app/src/main/java/com/termux/rafacodephi/runtime/
  RafRuntimeService.kt
  RafRuntimeProtocol.kt
  RafRuntimeRequestValidator.kt
  RafRuntimeCallerValidator.kt
  RafRuntimeQueue.kt
  RafRuntimeStore.kt
  RafRuntimeWorker.kt
  RafRuntimeRecovery.kt
  RafRuntimeArtifacts.kt
  RafRuntimeNotification.kt

app/src/test/.../runtime/
  RafRuntimeProtocolTest.kt
  RafRuntimeRequestValidatorTest.kt
  RafRuntimeQueueTest.kt
  RafRuntimeStoreTest.kt
  RafRuntimeRecoveryTest.kt
  RafRuntimeArtifactsTest.kt
```

If the app is Java-first, use Java consistently. Do not migrate unrelated code to Kotlin.

---

## 3. Service contract

Minimum v1 operations:

```text
int getProtocolVersion()
long submitJob(RafRuntimeRequest request, ParcelFileDescriptor source, IRafRuntimeCallback callback)
void cancelJob(long requestId)
RafRuntimeEvent getJobState(long requestId)
List<RafRuntimeArtifact> listArtifacts(long requestId)
ParcelFileDescriptor openArtifact(long requestId, String artifactName)
```

Rules:

- explicit component;
- protected permission/caller validation;
- protocol version checked before accepting job;
- request size capped;
- source descriptor duplicated safely and closed deterministically;
- no unrestricted path parameter;
- artifact name validated against allowlist;
- every response correlated by request ID and job ID;
- callbacks are advisory; persisted state remains source of truth.

---

## 4. Request model

Required fields:

```text
protocolVersion: Int
jobId: String
jobHash: String
operation: String
displayName: String
expectedSizeBytes: Long
sourceContentHash: String?
policyJson: String
jobJson: String
```

Validation:

- bounded string lengths;
- UTF-8/JSON validity where applicable;
- operation must equal a supported stable enum;
- `read_only=true` required for `index_conversations`;
- expected size nonnegative;
- job JSON hash matches `jobHash`;
- duplicate job ID with different hash rejected;
- output request list restricted to supported artifacts.

---

## 5. Durable job store

Persist a compact durable record before queueing.

Minimum fields:

```text
request_id
job_id
job_hash
operation
state
created_at
updated_at
expected_size
staged_size
source_hash
worker_version
worker_hash
worker_pid
last_checkpoint
last_error_code
last_error_stage
```

Use an existing Room/SQLite facility when appropriate, but do not store full source text or full logs in the database.

State transitions are append-audited and transactionally reflected in the current-state row.

---

## 6. Staging

The service receives source bytes through a read-only descriptor or narrowly scoped URI.

Staging algorithm:

1. create job directory from validated job ID;
2. create `source/input.part` with private permissions;
3. stream fixed-size chunks;
4. maintain 64-bit byte count;
5. update staging checkpoint periodically;
6. abort on cancellation or descriptor error;
7. verify expected size;
8. compute/verify content identity when configured;
9. flush critical metadata;
10. atomically rename to `source/input.ready`;
11. only then transition to `QUEUED`.

A partial file is never passed to the parser.

---

## 7. Queue semantics

- bounded queue size;
- FIFO by accepted request unless priority is explicitly versioned later;
- one active writer per job output directory;
- configurable maximum concurrent workers, default one on low-memory devices;
- duplicate identical submission returns existing request state;
- conflicting duplicate returns `CONTRADICTION`;
- queue survives process death;
- cancellation removes queued jobs or signals active worker;
- no busy-loop polling.

---

## 8. Worker launch

Resolve the exact RafPolimata scanner/indexer from a pinned runtime manifest.

Worker invocation semantics:

```text
convsegment <input.ready> <output.tmp> <checkpoint.state> <job.json>
```

Use the actual CLI agreed with RafPolimata; update both specs if the signature differs.

Requirements:

- exact executable path;
- executable exists and is executable;
- version/hash recorded;
- arguments passed separately;
- controlled environment;
- working directory = job directory;
- stdout/stderr redirected to bounded files;
- exit code captured;
- cancellation mapped distinctly from parser error;
- only the owned worker is terminated;
- output validation before publication.

---

## 9. Progress protocol

Progress event fields:

```text
protocolVersion
requestId
jobId
state
stage
bytesRead
expectedBytes
recordsProcessed
checkpoint
errorCode
errorMessageCode
updatedAt
```

Do not send entire logs through callbacks.

Rate-limit callbacks while persisting important transitions immediately. A reconnected client can call `getJobState` and recover current state.

---

## 10. Artifact validation and publication

Before `COMPLETED`:

- required artifact files exist;
- no artifact is still under `output.tmp` only;
- each file size is within policy;
- manifest parses and says `VERIFIED`;
- segment reader/validator succeeds where available;
- hashes match manifest;
- source identity matches the job;
- output directory promotion is atomic.

Then:

```text
output.tmp -> output
state -> COMPLETED
result.json written
callback emitted
```

Failure leaves evidence in the job directory but does not expose incomplete files as completed artifacts.

---

## 11. Recovery

On service/app startup:

- find jobs in nonterminal states;
- verify job record and directory;
- check whether worker is still alive and owned;
- verify source and checkpoint;
- verify temporary output committed sizes;
- choose recovery, failure or contradiction;
- append recovery audit event;
- restore foreground notification when work resumes.

Test cases:

- process death while staging;
- process death while queued;
- process death while worker running;
- process death during publication;
- corrupted checkpoint;
- missing source file;
- changed source hash;
- stale worker PID reused by another process.

---

## 12. Foreground service

Long indexing must run as a proper foreground operation.

Notification shows:

- operation name;
- source display name;
- progress bytes/records;
- state;
- cancel action;
- failure/recovery status.

Do not expose private content or credentials in notification text.

Handle Android version-specific foreground-service requirements using the project's existing compatibility approach.

---

## 13. Tests

### Unit

- request validation;
- stable operation mapping;
- duplicate/conflict logic;
- state transition legality;
- path/name sanitization;
- artifact allowlist;
- protocol serialization;
- queue ordering;
- recovery decisions.

### Service/instrumented

- allowed caller;
- rejected caller;
- valid descriptor staging;
- cancellation;
- callback correlation;
- Binder client death;
- open approved artifact;
- reject unknown artifact;
- foreground lifecycle.

### Runner integration

Keep `.github/workflows/rafaelia-runtime-runner-ci.yml` passing and add a deterministic fixture using the actual RafPolimata CLI when cross-repository checkout is available.

---

## 14. CI requirements

Focused workflow must run:

```text
unit tests
lint/static analysis
AIDL compilation
relevant debug APK build
runner contract test
service integration tests feasible on CI
artifact/hash report
```

Do not claim device proof unless an emulator/device test actually ran.

---

## 15. Acceptance criteria

- functional service implementation, not only interface;
- source staged by streaming;
- no full corpus in Java/Kotlin memory;
- typed/versioned request only;
- durable queue and states;
- exact worker version/hash recorded;
- real parser output validated;
- cancellation and recovery tested;
- completed artifacts published atomically;
- only allowlisted artifacts accessible;
- CI passes;
- all gaps reported as `TOKEN_VAZIO`.
