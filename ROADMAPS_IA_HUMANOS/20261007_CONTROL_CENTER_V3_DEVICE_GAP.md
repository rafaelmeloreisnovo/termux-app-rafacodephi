# RAFCODEΦ Control Center — Vectra V3: device-gap receipt (2026-10-07)

**Scope**: one **user-submitted** export, not a fresh execution of the current APK or CI. Internal Vectra is the diagnostic surface **inside** `com.termux.rafacodephi`, not a separate app.

## Rebuild path / checkpoints

1. **Source evidence:** ZIP export `rafcodephi-control-center-evidence-1791415759603.zip` SHA-256 `5a5d1ada7cbd0a1a79bdd5cf442d5119602aaf6e8c9e2aa1e1b023e0247febf3`; methods V3 SHA-256 `d009b4993e828f518708f5a8e1ac943efb8b40228a38c23bcd93c69fbf91308a`. Original input bytes are **not committed publicly**.
2. **Archive tests:** 4 entries, 7/7 archive/contract invariants `PASS_SCOPED`, 4/4 adversarial archive mutations rejected by a local stdlib-only verifier; the verification **does not** test the app, Android device, build, energy or benchmark.
3. **Bootstrap reality from this export:** `$PREFIX=PASS`; `$PREFIX/bin`, `$HOME`, `bin/sh`, `bin/pkg`, `BOOTSTRAP_PROFILE.json` `BLOCKED`; installed package runtime `BLOCKED`, cause `BOOTSTRAP_REAL_PROFILE_CONTRACT_BLOCKED`. Optional compatibility wrappers and absent APT/dpkg are reported individually as `UNAVAILABLE`. Do not conflate optional wrapper absence with startup blocker.
4. **Sensor inventory:** accelerometer/light/proximity `PASS`; gyroscope/magnetometer `UNAVAILABLE`; `framework_sensor_count=15`. Android sensor `minDelayUs` is requested sampling capability, **not** measured callback latency/throughput.
5. **PA/V3 evidence:** physical PA `NOT_MEASURED`, homogeneous governed n≥30 series `NOT_MEASURED` (0), PMU/callback/energy/comparability not promoted. See [machine receipt](./receipts/20261007_RAFCODEPHI_CONTROL_CENTER_V3.json).
6. **Source-vs-installed contradiction:** repository `master@d0277bcc6c0354935b907f9f8d18f22af294610f` has previously merged producer/consumer bootstrap hotfixes; however the export has **no installed APK hash, embedded archive hash or source commit**. Thus it is **not proven** that current source is defective or that current code has been physically tested. Do not blindly patch source, uninstall, clear app data or rewrite the prefix. Distinguish stale installation, packaging/custody, profile materialization and source bug with falsifiers.
7. **Next exact gate:** establish APK-installed→CI SHA-256 and embedded bootstrap identity; run the least destructive authorized installer/Wizard readback; expect startup `sh/pkg` and required real-pkg profile. Only then run one explicit PA protocol-v2 observation, followed by a separate governed homogeneous n≥30 series if needed. Capture per-gate PASS/FAIL/NOT_MEASURED receipts in the APK; publication remains blocked until independent rights, environment, uncertainty and physical gates pass.

## Provenance / owner matrix

| Domain | Authority | State in this export |
|---|---|---|
| Debian bootstrap and packages | `rafaelmeloreisnovo/termux-packages` | installed byte binding = TOKEN_VAZIO |
| APK installer, Control Center, receipt export | `rafaelmeloreisnovo/termux-app-rafacodephi` | export evidence exists; runtime BLOCKED |
| Physical PA benchmark / statistical V3 | installed device receipt | NOT_MEASURED |
| Licenses, rights, release/enforcement | independent governed review | TOKEN_VAZIO / HOLD |

**Rollback:** documentary-only successor on a separate branch. Preserve the original export, prior 7M index, installed app and user data; no source hotfix/merge/installation performed by this checkpoint.

**Invariant:** `SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`; `TOKEN_VAZIO != 0`; `claim_allowed=false`.

**Navigation:** [7M root](./README.md) → [human 7M](./TERMUX_RAFCODEPHI_7M.md) → [machine receipt](./receipts/20261007_RAFCODEPHI_CONTROL_CENTER_V3.json) → [μWRITE ledger](https://docs.google.com/document/d/1NEU7lg7iUZf1u7lX-SEKuc-knSDGlF5gOx7qzXcJ1yc/edit) → [HOTSTATE](https://docs.google.com/document/d/1d0J5SkF2S2emBq6jLuTkhYVaSOQiWazJWyRCTMJ-iRI/edit).
