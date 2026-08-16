# Beta CI manual diagnostic gate — root cause — 2026-08-16

## Scope

Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
Baseline master SHA: `b4a6b97235cf8f20563b722f2dbad64dcb4d22db`
Observed workflow run: `31928675937` (`RafCodePhi Beta Build`, `workflow_dispatch`)

## Proven observation

The source-built real ARM32 + ARM64 bootstrap, fail-closed bootstrap gate, Android SDK/NDK setup, exact bootstrap hash preparation, embedded real-pkg profile gate, and signed/unsigned APK matrix all completed successfully.

The run failed only at `Final beta diagnostic gate`.

For `workflow_dispatch`, diagnostic steps such as `build_guard`, `build_pss3`, and `blocker_gate` may legitimately be skipped when their corresponding manual inputs are disabled. The final gate currently evaluates some of those skipped outcomes as unconditionally `required`; `check_step` treats any required outcome other than `success` as failure. Therefore a legitimate disabled manual diagnostic can deterministically make the final gate fail.

## Required invariant

A diagnostic is blocking iff that diagnostic was enabled by policy for the current event/input set. Disabling an explicitly optional diagnostic must not be interpreted as diagnostic failure.

This does **not** relax real-bootstrap/APK gates. Source-build, provenance, Android setup, real profile, and APK matrix remain fail-closed whenever `build_apk_matrix` is enabled (and on push).

## Evidence policy

`PHYSICAL_ANDROID=TOKEN_VAZIO` remains valid until physical-device runtime evidence exists. `claim_allowed=false` remains unchanged for device-runtime/release claims.
