# BETA_RUNTIME_TEST_PLAN

## Scope
Manual+automatable checks in real device for first beta runtime gate.

## Required sequence
1. Install signed `arm64-v8a` APK.
2. Open app.
3. Open terminal.
4. Run `echo rafcodephi_beta_ok`.
5. Run `pwd`.
6. Run `id`.
7. Run `uname -a`.
8. Run `exit 0`.
9. Open/close 20 terminal sessions.
10. Open/close 100 terminal sessions.
11. Verify critical crash absence.
12. Verify no critical zombie process.
13. Verify memory usage trend.
14. Verify background/foreground behavior.
15. Verify screen rotation behavior.
16. Verify across Android 11/12/13/14/15 when devices are available.

## Evidence to collect
- ADB install output.
- Logcat capture per run.
- Session lifecycle counters.
- Process snapshots before/after.
- Runtime report (`dist/runtime-smoke/`).

## Gate
If any required sequence step fails, keep state as `CI_BETA_BUILD_READY`.
