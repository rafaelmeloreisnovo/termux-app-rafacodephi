# Upstream parallel (Termux official ↔ RAFCODEΦ fork)

Goal: keep this fork *side-by-side* with upstream while preserving RAFAELIA/RMR additions.

## Three always-on deltas ("3 into my fork")
1. **Package identity split**: applicationId / authorities / permissions remain unique (`com.termux.rafacodephi`).
2. **RMR compatibility layer**: small, freestanding-friendly shims & validation scripts (no hidden deps).
3. **Environment reproducibility**: `rafaelia_env/` doctor + profiles + launcher (host/proot) to keep dev parity.

## Minimal workflow
- Upstream changes: rebase/merge regularly.
- Run validation:
  - `./rafaelia_env/tools/doctor.sh`
  - `./rafaelia_env/tools/check_zombies.sh .`

## Notes
- Keep core modules low-level: no implicit libc calls; prefer explicit loops.
- Keep host-only helpers isolated under `rafaelia_env/` and `scripts/`.
