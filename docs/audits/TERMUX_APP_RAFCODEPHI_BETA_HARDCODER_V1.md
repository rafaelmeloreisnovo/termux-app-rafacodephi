# TERMUX APP RAFCODEΦ — Beta Hardcoder Audit V1

Date: 2026-08-03  
Scope: `termux-app-rafacodephi` only.  
Excluded: B1–B7 kernels, mathematical research claims and performance claims.

## Epistemic rule

```text
file exists
!= executable works
!= package backend works
!= network repository works
!= package install works
!= release proven
```

`claim_allowed=false` and `release_allowed=false` remain invariant.

## What is already materially implemented

### Android application and private prefix

- application package defaults to `com.termux.rafacodephi`;
- expected prefix is `/data/data/com.termux.rafacodephi/files/usr`;
- bootstrap is embedded per ABI through `termux-bootstrap-zip.S`;
- `TermuxInstaller` loads the embedded ZIP through `libtermux-bootstrap`;
- BLAKE3 is verified before extraction;
- extraction occurs in a staging prefix;
- unsafe ZIP paths and unsafe symlink destinations are rejected;
- file modes are applied explicitly;
- staging is renamed to the final prefix;
- failed installation removes incomplete staging/prefix;
- `$HOME`, `$PREFIX`, `tmp`, `var`, terminal shell and storage placeholder are initialized.

### What the installed bridge beta can honestly do

The current bridge payload supports a bounded Android shell environment:

- open the terminal;
- run `sh`;
- navigate the app prefix and permitted Android/storage paths;
- use generated wrappers for common applets;
- run `ls`, `cat`, `grep`, `pwd`, `mkdir`, `cp`, `mv` and related commands when a Toybox/Toolbox backend exists;
- display `pkg help` and `apt help`;
- preserve an explicit error instead of silently pretending that the APT backend exists.

This is useful as a filesystem and shell beta. It is not yet a complete Termux package distribution.

## Main gap found

The default bootstrap creates paths named:

```text
bin/pkg
bin/apt
bin/apt-get
bin/busybox
bin/proot
```

The default payload is a bridge. Presence of these paths does not establish that real APT, DPKG, BusyBox or PRoot binaries are installed.

The profile materializer now forces:

```text
BOOTSTRAP_FULLENGINE_READY=0
RAFCODEPHI_CLAIM_ALLOWED=0
RAFCODEPHI_DEVICE_VALIDATION=TOKEN_VAZIO
```

until a physical device receipt exists.

## Changes introduced

### 1. Explicit profiles

Two profiles are sealed inside rewritten bootstrap ZIPs:

```text
bridge
real-pkg
```

The profile is recorded in:

```text
BOOTSTRAP_PROFILE.json
BOOTSTRAP_INFO
```

The manifest records package, prefix, ABI, required entries, source ZIP hash, structural state and evidence limits.

### 2. Build path wired

The default local bootstrap path now calls:

```text
scripts/build_bootstrap_profile.sh
```

Default:

```text
RAF_BOOTSTRAP_PROFILE=bridge
```

Candidate real package profile:

```text
RAF_BOOTSTRAP_PROFILE=real-pkg
```

The real profile delegates to the existing ARM real-package builder and remains fail-closed when legacy-prefix risk is found. Non-ARM artifacts remain honest bridge profiles.

### 3. Runtime guard wired

`TermuxInstaller` already invokes:

```text
BootstrapBaremetalGuard.validateAfterBootstrap(prefix)
```

The guard now validates `BOOTSTRAP_PROFILE.json`.

For both profiles it verifies:

- schema;
- package name;
- exact prefix;
- ABI/profile match;
- required entries;
- `claim_allowed=false`;
- `release_allowed=false`;
- `device_validation=TOKEN_VAZIO`.

For `bridge`, explicit bridge markers are required.

For `real-pkg`, the guard additionally requires:

- ELF `bin/apt`;
- ELF `bin/apt-get`;
- ELF `bin/dpkg`;
- absence of bridge markers;
- `libapt-pkg`;
- usable `sources.list`;
- no legacy prefix in critical files.

Release/strict builds fail closed. Debug builds preserve diagnostic access while logging failure.

### 4. Physical beta audit

The read-only device script is:

```text
scripts/device_beta_hardcoder_audit.sh
```

It can run inside the installed app without ADB and without Python. It records:

- Android/device/ABI identity;
- prefix and home;
- bootstrap profile;
- classification of shell/package binaries as ELF, bridge script, normal script, missing or non-executable;
- APT/DPKG directories and status database;
- certificate presence;
- storage links and read access;
- shell/pkg/apt/dpkg non-mutating probes;
- DNS probe;
- HTTPS repository HEAD probe when real curl exists;
- append-only receipt directory and SHA-256 when available.

It does not run `pkg update` unless:

```text
RAF_BETA_MUTATING=1
```

is explicitly set.

## Beta tester matrix

| Surface | Current likely state | Acceptance proof |
|---|---|---|
| App starts | observed by user | terminal opens |
| Shell | bridge/minimal | `shell_exec_exit=0` |
| Directory navigation | available within Android permissions | `ls`, `pwd`, storage receipt |
| Common file operations | wrapper/Toybox-backed | command exit codes |
| `pkg help` | available | `pkg_help_exit=0` |
| Real `apt` | not established | ELF classification + version |
| Real `dpkg` | not established | ELF classification + status DB |
| DNS | not established | device receipt |
| TLS/repository | not established | HTTPS Release HEAD |
| `pkg update` | not executed by default | explicit mutating gate |
| `pkg install` | not established | `DEVICE_REAL_PKG_VALIDATED` |
| ARM32 physical receipt | open | exact APK/device receipt |
| ARM64 physical receipt | open | exact APK/device receipt |

## Hardcoder reading of the installed beta

The installed APK is not empty. It already proves:

```text
Android app
→ private prefix
→ verified ZIP
→ staging/rename
→ shell bridge
→ filesystem navigation
→ Android storage boundary
```

It does not yet prove:

```text
real package binaries
→ package database
→ repository metadata
→ DNS/TLS
→ dependency resolution
→ package installation
```

The next engineering move is not to add more wrapper names. It is to rebuild the package closure for the RAFCODEΦ prefix and preserve that identity through repository metadata, bootstrap generation, APK hash and physical receipt.

## Validation performed

Local equivalent-source validation:

```text
Python profile tests: 6/6 PASS
runtime wiring tests: 4/4 PASS
total focused tests: 10/10 PASS
Java syntax with Android stubs: PASS
bash syntax: PASS
POSIX shell syntax: PASS
```

Not performed:

```text
full Android Gradle build = TOKEN_VAZIO
APK install = TOKEN_VAZIO
installed app audit = TOKEN_VAZIO
real package rebuild = TOKEN_VAZIO
DNS/TLS device test = TOKEN_VAZIO
pkg update/install = TOKEN_VAZIO
```

## F_ok

- bootstrap installation path exists;
- BLAKE3, staging and rollback exist;
- bridge shell is operational enough for navigation and file work;
- build profiles are explicit;
- manifest is sealed into rewritten bootstraps;
- runtime profile validation is connected;
- device audit is read-only by default.

## F_gap

- exact installed APK has not emitted the new receipt;
- real package stack is not rebuilt for the RAFCODEΦ prefix;
- real ARM ZIPs still depend on passing legacy-prefix validation;
- no physical DNS/TLS/package installation receipt;
- no dual ARM evidence;
- full APK build for this branch has not run.

## F_next

1. build the branch in `bridge` profile and verify no regression;
2. install the exact APK and run the device audit;
3. preserve receipt and APK SHA-256;
4. rebuild ARM package closure for `/data/data/com.termux.rafacodephi/files/usr`;
5. build `real-pkg` profile;
6. reinstall and rerun the read-only audit;
7. explicitly enable the mutating package gate;
8. promote only after `DEVICE_REAL_PKG_VALIDATED`.
