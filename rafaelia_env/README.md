# RAFAELIA_ENV (RMR interoperability layer)

This directory is a *low-level, no-surprises* developer layer:

- `bin/rafx` : one-command launcher (Termux host vs proot-distro)
- `profiles/*` : stable compiler flags and environment
- `tools/doctor.sh` : environment audit
- `tools/packages.manifest` : baseline packages
- `docs/UPSTREAM_PARALLEL.md` : keep fork compatible with upstream

Principle: core modules must not depend on libc; any I/O goes through explicit backends.
