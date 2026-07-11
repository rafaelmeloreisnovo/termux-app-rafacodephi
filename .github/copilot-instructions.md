# GitHub Copilot Instructions — termux-app-rafacodephi

## Repository role

This repository is the local Android/Termux execution runtime for RAFAELIA. It accepts versioned jobs from RafGitTools, stages read-only sources, runs the pinned RafPolimata indexer, maintains checkpoints and publishes validated artifacts.

Read these files before editing RAFAELIA runtime behavior:

- `scripts/rafaelia/run_index_conversations.sh`
- `.github/workflows/rafaelia-runtime-runner-ci.yml`
- the current Android manifest, package constants and service conventions
- existing foreground-service/process-lifecycle implementation
- existing bootstrap and private-path constants
- `RafPolimata/include/rafaelia_runtime_protocol.h`
- `RafGitTools/schemas/rafaelia_runtime_job.schema.json`

## Architectural boundary

```text
RafGitTools
  -> explicit versioned Android service request
RAFCODEPhi service
  -> durable validated queue
  -> private worker inside the Termux sandbox
  -> pinned RafPolimata indexer
  -> atomic artifacts + audit + checkpoint
```

RafGitTools must not depend on direct access to this app's private filesystem. The external API must expose typed operations only; it must not be a generic command channel.

## Required job state machine

```text
RECEIVED
VALIDATING
STAGING
QUEUED
STARTING
RUNNING
CHECKPOINTING
COMPLETED
FAILED
CANCELLING
CANCELLED
RECOVERING
CONTRADICTION
```

Persist a state transition before reporting it. A job ID maps to immutable job bytes. Reusing a job ID with different bytes is a contradiction.

## Cross-app service requirements

- explicit Android component;
- app-controlled permission and caller validation;
- version negotiation;
- bounded request payload;
- stable operation enum matching the runtime schema;
- read-only source descriptor or narrowly scoped content URI;
- cancellation and progress callbacks;
- caller/process death handling;
- audit events for accepted, rejected, cancelled, failed and completed jobs;
- no generic command text in the service contract.

## Private storage layout

Use app-private paths derived from the actual package/runtime constants:

```text
$HOME/.rafaelia/
  jobs/<job-id>/
    job.json
    job.hash
    source/input.part
    source/input.ready
    output.tmp/
    output/
    checkpoint.state
    audit.jsonl
    worker.pid
    result.json
  locks/
  protocol/
```

Rules:

- user/display names are metadata, not path components;
- internal names derive from validated job IDs;
- reject traversal or unexpected separators;
- temporary output is never exposed as completed output;
- publish by atomic rename in the same filesystem;
- keep credentials outside job folders, reports and logs.

## Memory and I/O

- stream descriptors with fixed-size buffers;
- never read a large JSON source into Java/Kotlin memory;
- use 64-bit sizes, offsets and counters;
- verify expected size and available storage before staging;
- keep progress events bounded;
- keep worker logs in files with a documented size policy;
- flush checkpoint and critical metadata according to a documented durability policy.

## Internal worker frame

Use a versioned 32-byte little-endian control header:

```c
struct raf_rpc_header_v1 {
    uint8_t  magic[8];      /* "RAFRPC1\0" */
    uint16_t version;
    uint16_t type;
    uint32_t flags;
    uint64_t request_id;
    uint32_t payload_len;
    uint32_t crc32c;
};
```

- control payload maximum: 1 MiB;
- validate length and CRC before payload decoding;
- v1 payload is UTF-8 JSON matching the canonical runtime schema;
- large source bytes stay in files/descriptors, not control frames;
- partial frame reads/writes and early connection closure must return deterministic errors.

## Worker execution

Extend the proven boundary in `scripts/rafaelia/run_index_conversations.sh`.

- resolve an exact executable from a pinned runtime manifest;
- pass arguments as separate values, not concatenated untrusted text;
- use a controlled environment and job working directory;
- record executable version/hash and process exit status;
- distinguish cancellation from parser failure;
- prevent concurrent publication to one output directory;
- preserve audit and last valid checkpoint after failure/cancellation.

## Recovery

On app/process restart:

1. enumerate nonterminal jobs;
2. validate job bytes and hash;
3. validate source identity and staged size;
4. validate checkpoint version and committed output sizes;
5. choose `RECOVERING`, `FAILED` or `CONTRADICTION`;
6. never resume when source identity changed;
7. record the recovery decision.

## Artifact boundary

Expose only allowlisted, read-only artifacts:

```text
source.manifest.json
conversations.segment
messages.segment
timeline.segment
relations.segment
audit.jsonl
checkpoint.state
coverage_report.json
result.json
```

No external API accepts an unrestricted filesystem path. Preview access must be paged or byte-capped.

## Tests

Required coverage:

- valid and invalid protocol versions;
- unknown operation;
- read-only policy violation;
- caller validation failure;
- oversized request;
- source descriptor failure and size mismatch;
- insufficient storage;
- duplicate identical job and conflicting duplicate ID;
- queue and concurrency behavior;
- cancellation before and during execution;
- process restart and recovery;
- checkpoint/source mismatch;
- worker failure;
- atomic publication;
- artifact allowlist and traversal rejection;
- client death;
- foreground service lifecycle.

Preserve the existing runner contract CI. Add focused tests without weakening unrelated workflows.

## Definition of done

The runtime vertical is complete only when a typed `index_conversations` job crosses the Android boundary, stages the source read-only, executes the pinned RafPolimata indexer, publishes real artifacts atomically, supports progress/cancellation/recovery, exposes only approved artifacts and passes CI. Unsupported behavior remains explicit as `TOKEN_VAZIO` or `CONTRADICTION`.
