# INTERNETMOUNT Ω V0 — cloud-backed virtual document namespace (REVIEW_ONLY)

**Producer:** termux-app-rafacodephi; this branch only. **Date:** 2026-11-14.
**Status:** SOURCE_WRITTEN; provider adapter, network I/O, APK build, CI exact HEAD, Android physical E2E = NOT_RUN.
**Claim:** claim_allowed=false. No working cloud mount is claimed.

## User goal / first useful outcome
Make existing Google Drive backups browseable and selectively readable on a low-storage, unrooted Android phone. **No bulk backup downloads and no backup deletion.**
Do not conflate RAID, RAM, battery, parity, cloud availability or durable storage.

## Authority and constraints
- Drive START HERE Ω V2.2 = routing; CURRENT_STATE = inventory checkpoint authority, not content integrity.
- Google Drive provider = stored bytes; source ID + account + version/revision + MIME + length define object identity.
- Supabase private tables `raf_private.drive_metadata_index_v1`,
  `raf_private.drive_metadata_identity_projection_v1` and
  `raf_private.drive_cloud_selection_intents_v1` = optional scoped metadata/intent;
  never public content, access token or identity authority for Google Drive itself.
- This Termux repository = Android adapter, provider and pure-core code authority.
- RLL producer = scientific dataset/likelihood authority; no cosmological parameter or dataset change.
- Historical backups follow Drive BACKUP_LINEAGE_V1__PRESERVE_EVERY_COPY:
  `POSSIBLE_DUPLICATE` is NOT `SAME_CONTENT_SHA256`.
- External references: Android SAF/DocumentsProvider and Google Drive `files.get?alt=media`
  `Range` reads for stored blob files. Export of native Google Docs has different semantics.

## Architecture: separate five layers
1. **Catalog/namespace (tiny local metadata):** provider account alias, remote file ID,
   observed version/revision, MIME, size, privacy class, chunk map, content evidence.
   Never infer content identity from name/size or metadata fingerprint.
2. **Read-through memory cache (volatile):** bounded e.g. initial target 1–4 MiB;
   LRU clock is a performance policy, not a retention or integrity receipt.
3. **Durable cache / journal (optional):** user-selected SD or app-specific storage;
   quota target initially 64 MiB, conditioned on real free space and filesystem capabilities.
   Reserve safety headroom; if unavailable, read-only/no-cache mode.
4. **Integrity:** independent authenticated version manifest + SHA-256 at hosted boundary;
   optional XOR parity stripes and local CRC detection inside PURE_CORE.
   Ciphertext authenticated before use; CRC is not a MAC and XOR parity is not ECC against
   adversarial alteration. Two data chunks + one XOR parity chunk = 50% parity overhead,
   reconstructs one known erasure only if other chunks and trusted checksums are valid.
5. **Remote backend:** authenticated, range-capable Drive blob reads; optional other
   provider later, but only with separate authorization and validated semantics.
   Handle 401/403/404/416/429/5xx, changed ETag/version, truncated ranges and offline.

## Android integration decision
- First present a **separate, read-only `DocumentsProvider` root** named INTERNETMOUNT.
  This is a SAF virtual namespace, NOT a POSIX/block mount and NOT a Linux `/dev/` disk.
- CLI `rafim ls/stat/read/verify` (future hosted boundary) can expose streaming to Termux.
  No root/FUSE mount assumption. `mmap`, random-write POSIX, fsync/rename atomics are
  not promised until explicitly implemented and tested.
- Do NOT attach untrusted remote IDs to existing home file paths.
- **Security P0 candidate in existing `TermuxDocumentsProvider`:** path-based document
  IDs reach `getFileForDocId` without an explicit canonical-home containment check, while
  `isChildDocument` uses string prefix. `createDocument` and recursive search need
  containment and symlink tests. Review independently before broad provider exposure.
  This proposal does not modify that implementation or assert exploitability.
- Root permissions are scoped, no credentials in project logs or Supabase public schemas.
  Per-account grants, account unlink and revocation must be tested.

## Write path (future; fail closed)
A write must not be acknowledged as **remote durable** before:
local durable journal -> fsync/readback as supported -> authenticated upload ->
remote version/receipt -> remote readback/hash for exact committed version ->
explicit `REMOTE_CONFIRMED`.
RAM/battery is not durable backup. Android battery != power-loss-safe NVRAM.
In V0, exposed remote content is **read-only**. Offline and quota exhaustion cannot
silently report success. Incomplete cache state remains UNKNOWN/PENDING, never zero.

## PURE_CORE implementation in this review branch
- `native/internetmount/raf_im_ecc_v1.h`: no malloc/libc/syscalls; fixed max 4KiB
  two-data/one-parity stripe, CRC32 IEEE accidental-corruption detection, isolated
  output scratch on single corruption; two corruptions fail closed.
- `tests/internetmount/test_raf_im_ecc_v1.c`: hosted harness/negative cases.
- Bounds/overlap contract: each buffer >=n, n in 1..4096; all buffers disjoint.
  Partial overlap is forbidden by the caller, not detected by this core.
- Expected CRC metadata MUST be authenticated out of band; merely computing
  checksums from the same corrupt buffers cannot establish integrity.
- This is an erasure recovery primitive, **not** a full RAID/ECC controller.
  No Reed-Solomon, two-parity recovery, AES, network, journaling, mount or APK included.

## Falsifiers / gates
G0 provider scope: malicious document IDs (`../`, absolute paths, symlink escape,
prefix-sibling paths), revoked account, permission denial, descriptor lifetime.
G1 local core: CRC known-answer, intact stripe, corrupt each of three members,
two corrupt members, bounds, invalid/null input, scratch aliasing, fuzz/sanitizers.
G2 network: HTTP range exact bytes, size/Content-Range, revision conflict, transient
timeouts, offline, retries with idempotency/backoff, quota, OAuth revocation.
G3 durability: simulated kill/power loss before/after each journal/upload boundary;
no false-ack; readback and restoration.
G4 scientific RLL: immutable dataset identities, pinned inputs, exact selection and
covariance; failed remote read cannot be substituted by a zero-filled sample.
G5 ARMv7/ARM64: exact HEAD build+CI and device tests; no implied PASS from C source.

## State / receipt
`UNKNOWN -> REMOTE_ONLY -> FETCHING -> CACHE_UNVERIFIED -> VERIFIED_CACHE`.
Write branch not enabled: `VERIFIED_CACHE -> DIRTY_JOURNALED -> UPLOAD_PENDING ->
REMOTE_CONFIRMED` only after exact durable receipts.
`CACHE_CORRUPT`, `OFFLINE`, `CONFLICT`, `REVOKED`, `TOKEN_VAZIO` are distinct.

R3: F_ok=existing provider and pure C source identified; F_gap=network/provider
integration, running tests and exact device E2E; F_next=review containment P0,
execute hosted tests, then implement read-only separate SAF adapter.
Rollback: discard this independent branch; master is untouched.
