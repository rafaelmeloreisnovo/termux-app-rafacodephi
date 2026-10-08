# ARMv7 Wizard — bootstrap BLAKE3 mismatch

Scope: user screenshots from 2026-10-07 vs source. No APK installation or physical benchmark executed here.

## Observed device UI

- Package: com.termux.rafacodephi, version shown 0.118.0-rafacodephi
- Android 15 Setup Wizard = title of screen, not verified Android OS version
- APK inspection UI: 17 DEX, 8 native libraries, ABI armeabi-v7a
- Exception: SecurityException / BOOTSTRAP_BLAKE3_MISMATCH
- Expected BLAKE3 (installed APK screenshot): 425a9f9d69ac33244f30aaf93c3a1290f5828cf465c11be46927e74ea03c4987
- Actual BLAKE3 (selected ZIP screenshot): 14240c566cb4238aa36cebb5ac391c6e752f30f0cdf3bf05293b96c5f8af2390
- External ZIP rejected. Hash mismatch by itself does not establish corruption.

## Source cause boundary

BetaBootstrapWizardActivity calls BootstrapWizardSource.accept. The latter computes the selected bytes' BLAKE3 and compares it to BootstrapIntegrityVerifier.expectedHashForCurrentAbi, derived from the installed APK BuildConfig ARM pin.

Thus any user-selected ZIP not matching the exact app/build pin is rejected, even if structurally valid. This is a deliberate fail-closed trust policy. Never replace expected with actual, disable hashing, or force install. UI remediation clarifies exact APK+bootstrap origin.

No installed APK SHA-256, selected actual file SHA-256, producer commit, or matching BLAKE3 computation for the uploaded desktop ZIP is established by these screenshots. Do not treat these distinct objects as the same artifact without evidence.

## P0 closure route

1. Record installed APK SHA-256/ABI/build identity without uninstalling or clearing data.
2. Record hashes of the exact ZIP selected on the phone and compare with producer artifact and CI receipt.
3. Check the BuildConfig ARM pin is generated from that same producer artifact.
4. Use the same verified build's bootstrap ZIP, or perform a governed rebuild and new APK; do not bypass a failed pin.
5. Back up and inventory prefix/home before any potentially destructive Repair/Install.
6. Capture installed sh/pkg and full runtime receipt before promoting runtime PASS.
7. Only thereafter run PA benchmark and homogeneous 30-trial series.

| Gate | State |
|---|---|
| Mismatch on screenshot | OBSERVED |
| Static BLAKE3 rejection path | SOURCE_CONFIRMED |
| Selected ZIP identity vs uploaded ZIP | TOKEN_VAZIO |
| Installed APK ↔ CI producer identity | TOKEN_VAZIO |
| Physical bootstrap install | BLOCKED |
| Native runtime shell/pkg | BLOCKED |
| PA benchmark | NOT_MEASURED |
| Release/legal | HOLD |

No original screenshots uploaded to public repository; PR is diagnostic UI only. SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM.

R3=<F_ok:fail-closed matcher/source hotfix,F_gap:exact-byte device binding,F_next:hash/readback before repair>.
