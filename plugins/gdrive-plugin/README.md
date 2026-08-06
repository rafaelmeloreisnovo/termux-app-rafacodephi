# Google Drive Sync Plugin for Termux RAFCODEΦ — V2 Draft

```text
maturity=DRAFT_FAIL_CLOSED_CLI
claim_allowed=false
release_allowed=false
device_runtime=TOKEN_VAZIO
```

This branch is a narrow, fail-closed prototype. It is not a released Drive client and is not installed by the APK/bootstrap yet.

## Sustaining invariant

```text
SUCCESS ⇔ AUTH ∧ HTTP_2XX ∧ UNIQUE_DRIVE_ID ∧ NO_CONFLICT ∧ HASH_CONFIRMED ∧ STATE_COMMITTED
```

A failure, conflict or unknown state never becomes `success`. Planning completes before the first mutation.

## Implemented scope

| Capability | State |
|---|---|
| OAuth installed-app flow | Structural implementation; real-account receipt pending |
| OAuth state verification | Implemented |
| PKCE | S256 implemented |
| Token refresh | Implemented; JSON via `jq`, files mode `0600` |
| Flat-folder upload | Implemented structurally |
| Same-ID update | Implemented using Drive file ID |
| Download | Temporary file → MD5 → atomic rename |
| Planner | Last confirmed state × local current × remote current |
| Initial/concurrent mismatch | Explicit conflict; no overwrite |
| Duplicate remote names | Explicit conflict |
| Pagination | Implemented |
| Nested hierarchy | Disabled; nested local files are rejected |
| Deletion propagation | Disabled; deletion becomes conflict |
| Google-native export | Disabled pending format policy |
| Browser writes | Disabled |
| Status page | Optional, read-only, loopback only |
| ARMv7/ARM64 device proof | `TOKEN_VAZIO` |
| APK/bootstrap integration | `TOKEN_VAZIO` |

## Dependencies

Required:

```text
bash curl jq md5sum awk find stat base64 od tr python3
```

Optional:

```text
termux-open-url
```

This is a low-dependency Bash + Python implementation, not a dependency-free one. Bash/`jq` handle OAuth and protected state; Python standard library implements the sync planner; `curl` performs transfers; Drive `md5Checksum` confirms ordinary binary content.

## Source-checkout setup

```bash
cd plugins/gdrive-plugin
chmod +x gdrive-auth.sh gdrive-sync.sh gdrive-webservice.sh tests/test_gdrive_plugin.sh
```

Repository presence is not installation proof.

## OAuth

Create a Google OAuth **Desktop app** client and enable Drive API. Default least-privilege scope:

```text
https://www.googleapis.com/auth/drive.file
```

For an explicitly authorized test requiring an arbitrary existing Drive folder:

```bash
export GDRIVE_SCOPE='https://www.googleapis.com/auth/drive'
```

Optional client secret:

```bash
export GDRIVE_CLIENT_SECRET='...'
```

Authenticate:

```bash
./gdrive-auth.sh init YOUR_CLIENT_ID
```

The flow creates random OAuth `state`, creates a PKCE S256 verifier/challenge, prints or opens the authorization URL, requires the complete loopback redirect URL, verifies `state`, exchanges the code with `code_verifier`, and stores tokens/credentials as `0600`.

```bash
./gdrive-auth.sh status
./gdrive-auth.sh token      # data-only stdout
./gdrive-auth.sh refresh
./gdrive-auth.sh revoke
./gdrive-auth.sh selftest
```

Logs use stderr and do not intentionally include tokens.

## Plan before mutation

```bash
./gdrive-sync.sh plan /path/to/flat-folder DRIVE_FOLDER_ID | jq .
```

The planner compares:

```text
S0 = last confirmed state
SL = current local MD5
SR = current Drive MD5
```

Actions include:

```text
upload | update_remote | download_remote | baseline | noop
conflict_initial_mismatch | conflict_both_changed
conflict_duplicate_remote | conflict_local_deleted | conflict_remote_deleted
unsupported_google_native | state_orphan
```

Any conflict, unsupported object, unsafe name or orphan state blocks the run.

## Execute

```bash
./gdrive-sync.sh mirror /path/to/flat-folder DRIVE_FOLDER_ID
./gdrive-sync.sh upload /path/to/flat-folder DRIVE_FOLDER_ID
./gdrive-sync.sh download DRIVE_FOLDER_ID /path/to/flat-folder
./gdrive-sync.sh status | jq .
```

One-way modes also plan the whole operation first. A required action in the opposite direction blocks the run before mutation.

### V2 boundaries

- local files must be direct children of the selected directory;
- paths are never silently flattened;
- local/remote deletions are conflicts because tombstones are undefined;
- initial same-name/different-content objects are conflicts;
- changed local content updates the same Drive ID;
- downloads are temporary, checksum-verified and atomically renamed;
- state is committed only after confirmed transfer.

## Ignore patterns

`~/.config/gdrive-plugin/.gdrive-ignore` uses direct-child glob patterns:

```text
*.tmp
*.part
.DS_Store
```

It is not a complete `.gitignore` parser.

## Read-only status service

```bash
./gdrive-webservice.sh daemon 8080
./gdrive-webservice.sh status
./gdrive-webservice.sh stop
```

Endpoints:

```text
GET /
GET /api/status
GET /api/logs
```

All POST requests return `405 write_api_disabled`.

## Local contract tests

```bash
./tests/test_gdrive_plugin.sh
```

The current suite verifies Bash/Python syntax, fail-closed JSON contracts, OAuth JSON/PKCE selftest, three-state planning, nested-path rejection, correct upload endpoint and metadata, conflict detection, and loopback read-only service.

A local PASS does not prove a real Google account, real Drive transfer, Android device behavior or packaging.

## Promotion gates

Remain draft until receipts exist for:

1. real OAuth account;
2. upload → same-ID update → verified download;
3. network interruption, HTTP errors and bounded retry/backoff;
4. conflict preservation;
5. Termux ARMv7 physical device;
6. Termux ARM64 physical device;
7. bootstrap/APK installation route;
8. security review and scope decision.

```text
F_ok   = local contracts 9/9; PKCE; planner; identity; verified transfer boundaries
F_gap  = real account/Drive; fault injection; ARM devices; packaging/signature
F_next = isolated real-account folder + device receipts, with PR still draft
```
