# Google Drive Sync Plugin for Termux RAFCODEΦ — V2 Draft

This branch contains a **fail-closed CLI prototype**, not a released Google Drive client.

```text
claim_allowed=false
release_allowed=false
device_runtime=TOKEN_VAZIO
```

The V2 revision intentionally narrows the original proposal. It preserves one local directory level, stable Google Drive file identity, explicit conflict states and verified transfers. Features that do not yet have a safe policy—recursive trees, deletion propagation and browser-triggered writes—are disabled rather than simulated.

## Sustaining invariant

A sync result may be called successful only when all applicable conditions hold:

```text
authentication valid
+ HTTP response is 2xx
+ local path maps to one stable Drive file ID
+ no unresolved initial/concurrent conflict
+ uploaded/downloaded MD5 is confirmed
+ state is written only after the confirmed operation
```

Formally:

```text
SUCCESS ⇔ AUTH ∧ HTTP_2XX ∧ UNIQUE_ID ∧ NO_CONFLICT ∧ HASH_CONFIRMED ∧ STATE_COMMITTED
```

A failure or unknown state never becomes `success` through `|| true`.

## Current scope

| Capability | State |
|---|---|
| OAuth 2.0 installed-application flow | Implemented structurally; real-account receipt pending |
| OAuth state verification | Implemented |
| Token refresh | Implemented using protected credentials file |
| Flat folder upload | Implemented structurally |
| Update existing Drive file | Implemented using stable Drive file ID |
| Flat folder download | Temporary file → MD5 → atomic rename |
| Bidirectional planning | Implemented with three-state comparison |
| Concurrent local + remote change | Explicit conflict; no overwrite |
| Duplicate remote names | Explicit conflict |
| Pagination beyond 1,000 files | Implemented |
| Nested directory hierarchy | Disabled in V2 |
| Deletion propagation | Disabled in V2 |
| Google Docs/Sheets/Slides export | Disabled until format policy exists |
| Browser write API | Disabled |
| Read-only localhost status page | Optional, implemented with Python standard library |
| Android ARMv7/ARM64 device proof | `TOKEN_VAZIO` |
| APK/bootstrap packaging | `TOKEN_VAZIO` |

## Dependencies

Required:

```text
bash curl jq md5sum awk find stat base64 od tr python3
```

Optional:

```text
termux-open-url opens the OAuth URL automatically
```

This is a low-dependency Bash + Python implementation. It is **not dependency-free**: OAuth orchestration uses Bash and `jq`; the three-state sync planner/executor uses Python standard library; HTTP transfers use `curl`; content identity is checked against Google Drive `md5Checksum`.

## Installation in a source checkout

```bash
cd plugins/gdrive-plugin
chmod +x gdrive-auth.sh gdrive-sync.sh gdrive-webservice.sh tests/test_gdrive_plugin.sh
```

The PR does not yet copy these scripts into the APK, bootstrap ZIP or `$PREFIX/bin`. Repository presence is not installation proof.

## OAuth setup

Create an OAuth client of type **Desktop app** in Google Cloud and enable Google Drive API.

The default scope is the least-privilege scope:

```text
https://www.googleapis.com/auth/drive.file
```

That scope may not expose an arbitrary pre-existing folder unless the folder/files were created or explicitly opened through the application. Full arbitrary Drive access requires an explicit wider scope:

```bash
export GDRIVE_SCOPE='https://www.googleapis.com/auth/drive'
```

The full Drive scope is restricted and may require additional Google verification depending on distribution and use.

Authenticate without putting the client secret in shell history:

```bash
./gdrive-auth.sh init YOUR_CLIENT_ID
```

The script:

1. prompts for the desktop client secret with hidden input;
2. creates a random OAuth `state` value;
3. opens or prints the Google authorization URL;
4. asks for the **complete loopback redirect URL**;
5. verifies `state` before extracting the authorization code;
6. stores credentials and tokens with mode `0600`.

Commands:

```bash
./gdrive-auth.sh status
./gdrive-auth.sh token      # prints only the access token to stdout
./gdrive-auth.sh refresh
./gdrive-auth.sh revoke
./gdrive-auth.sh selftest
```

Logs are written to stderr and never intentionally include access or refresh tokens.

## Planning before execution

Always inspect a plan first:

```bash
./gdrive-sync.sh plan /path/to/local-folder DRIVE_FOLDER_ID | jq .
```

The planner compares:

```text
S0 = last confirmed state
SL = current local MD5
SR = current Drive MD5
```

Possible actions include:

```text
upload
update_remote
download_remote
baseline
noop
conflict_initial_mismatch
conflict_both_changed
conflict_duplicate_remote
conflict_local_deleted
conflict_remote_deleted
unsupported_google_native
state_orphan
```

Any conflict, unsupported object, unsafe name or orphaned state blocks execution.

## Sync commands

```bash
# Bidirectional, only when the plan has no blocker
./gdrive-sync.sh mirror /path/to/local-folder DRIVE_FOLDER_ID

# One-way local → Drive; a required download blocks the run before mutation
./gdrive-sync.sh upload /path/to/local-folder DRIVE_FOLDER_ID

# One-way Drive → local; a required upload/update blocks the run before mutation
./gdrive-sync.sh download DRIVE_FOLDER_ID /path/to/local-folder

./gdrive-sync.sh status | jq .
```

### Important V2 boundaries

- The local directory must contain files only at depth 1.
- Nested files cause a hard failure; their paths are never silently flattened.
- Remote deletions and local deletions become conflicts because tombstone semantics are not yet defined.
- Initial local/remote name matches with different content become conflicts.
- A changed local file updates the same Drive ID; it does not create another same-name object.
- A download is written to a temporary file, checked against the Drive MD5, then atomically renamed.
- State records are updated only after a confirmed transfer.

## Ignore patterns

`~/.config/gdrive-plugin/.gdrive-ignore` contains Bash glob patterns matched against direct child names:

```text
*.tmp
*.part
.DS_Store
```

This is not a full `.gitignore` implementation.

## Read-only status service

The optional web component is intentionally read-only and binds only to loopback:

```bash
./gdrive-webservice.sh daemon 8080
# Open http://127.0.0.1:8080

./gdrive-webservice.sh status
./gdrive-webservice.sh stop
```

Available endpoints:

```text
GET /api/status
GET /api/logs
GET /
```

All `POST` requests return `405 write_api_disabled`. Sync remains CLI-only until request-body parsing, authentication, CSRF protection and an authorization model are implemented and tested.

## Local contract tests

```bash
./tests/test_gdrive_plugin.sh
```

The test suite checks:

- Bash syntax and Python bytecode compilation;
- JSON contracts;
- token JSON roundtrip without stdout contamination;
- planner decisions for local, remote and simultaneous changes;
- rejection of nested paths;
- correct upload endpoint and escaped metadata;
- loopback read-only status service and disabled writes.

A passing local contract suite does **not** prove Google OAuth, real Drive transfer, Android device behavior or packaging.

## Promotion gates

The branch must remain draft until receipts exist for:

1. real OAuth account flow;
2. upload → same-ID update → download roundtrip;
3. network interruption and HTTP error behavior;
4. conflict preservation;
5. Termux ARMv7 physical device;
6. Termux ARM64 physical device;
7. bootstrap/APK installation route;
8. security review and restricted-scope decision.

## Epistemic state

```text
F_ok:
  local parser/planner/endpoint/status contracts are testable
  identity and conflict invariants are explicit
  false-success paths were removed

F_gap:
  real Google account and Drive receipts
  PKCE
  resumable uploads and bounded retry policy
  recursive hierarchy and tombstones
  Workspace-native export
  ARMv7/ARM64 device and packaging proof

F_next:
  run the real-account matrix on an isolated test folder
  preserve HTTP, IDs, MD5 values, timestamps and device metadata in a receipt
```
