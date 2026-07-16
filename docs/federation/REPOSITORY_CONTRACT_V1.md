# Termux RAFCODEΦ — Federated Repository Contract v1

**Federated role:** Android shell/runtime substrate for local execution.  
**Local authority remains:** `docs/STATUS.md`, `docs/ENGINEERING_SYSTEM_RUNBOOK.md`, ABI policy, bootstrap manifests, build/release workflows and device evidence.

## Concrete interface

```text
input: APK + bootstrap payload + Android device capabilities
output: shell/runtime state with package, ABI, filesystem and command evidence
```

The federation may consume status from this repository. It may not infer a complete Termux distribution from a shell prompt, wrapper command or package-name-compatible APK.

## Non-negotiable invariants

1. `pkg`, `apt`, `apt-get`, `dpkg`, `libapt` and `proot` remain `TOKEN_VAZIO` until their real backend and install/update tests are proven.
2. `armeabi-v7a` and `arm64-v8a` remain explicit release gates.
3. Bootstrap payload identity and hash are checked before use.
4. A wrapper/bridge cannot be reported as the wrapped subsystem.
5. Device evidence must identify package, Android version, ABI, prefix and commit.
6. Release signing and internal unsigned validation remain separate lanes.

## Ordered health gates

```text
apk_install
package_identity
prefix_exists
shell_exec
busybox_or_coreutils
bootstrap_hash
abi_policy
certificates
dns
repository_metadata
dpkg_database
apt_update
pkg_install_smoke
proot_session
```

A gate without direct command output is `TOKEN_VAZIO`.

## Fail-safe

On package/bootstrap inconsistency:

- stop package mutation;
- preserve existing `$PREFIX`;
- switch to diagnostic/read-only mode;
- record the missing binary, path, exit code and hash;
- never fabricate an `apt`/`pkg` success response.

## Failover

`UserLAnd` may provide an alternate Linux userspace only as an availability path. It does not prove the Termux package backend, package identity, bootstrap or prefix contract.

## Rollback

Rollback anchor:

```text
base commit + previous APK SHA-256 + bootstrap hash + device smoke report
```

Recovery requires reinstall/upgrade validation and rerunning the ordered gates through at least `shell_exec`, `bootstrap_hash` and `abi_policy`.

## Watchdog expectations

A bounded watchdog monitors command duration and child-process exit. It may terminate a stalled command, but must save:

- command and sanitized environment;
- PID/process state when available;
- elapsed monotonic duration;
- last output bytes;
- termination reason.

No watchdog may silently convert a killed command into success.

## Blind tests

- hide one required bootstrap binary and require the correct blocked gate;
- supply one deliberately incorrect bootstrap hash;
- permute non-semantic environment variable order;
- run one ABI fixture selected by recorded seed;
- compare wrapper output with direct backend evidence and require disagreement to become `CONTRADICTION`.

## Temporal refusal

“Android 15/16 ready”, “production” and “functional” require a dated build/device result tied to a commit and artifact hash. Documentation alone remains `DECLARED_BY_AUTHOR` or `PARTIAL`.

## Federated output

```text
F_ok: directly observed runtime gates
F_gap: TOKEN_VAZIO / CONTRADICTION / BLOCKED
F_next: smallest missing runtime gate
rollback_anchor: commit + APK/bootstrap hashes
```
