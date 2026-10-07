# Source Alias Graph V1 — Exact-Blob DNA Map

Base: `b9052bda9a88017941124d2e1efd5f8019fbbb74`; tree `443764cae7b5ece35aa11dabcf312fb042fa3fff`.

- C/C++/ASM sources: **361**
- unique blobs: **318**
- exact duplicate groups: **29**
- duplicate excess: **43**
- canonical bound by explicit authority: **5**
- authority still empty: **24**
- deletion allowed: **false**

## Bound groups

- `714c4883730c`: canonical `app/src/main/cpp/lowlevel/baremetal_nomalloc.h`; aliases: `Arme/Add/baremetal_nomalloc.h`, `BugOrAdd/baremetal_nomalloc.h`, `rmr/Rrr/baremetal_nomalloc.h`
- `cf1a7886fe2d`: canonical `app/src/main/cpp/lowlevel/baremetal_asm.S`; aliases: `Arme/Add/repo_baremetal_orig.S`
- `0fe3dedb7011`: canonical `app/src/main/cpp/lowlevel/rafaelia_commit_gate_ll.c`; aliases: `Arme/Add/repo_commit_gate.c`
- `25f4e4efa134`: canonical `app/src/main/cpp/lowlevel/rafaelia_commit_gate_ll.h`; aliases: `Arme/Add/repo_commit_gate.h`
- `cc5af8163e0a`: canonical `app/src/main/cpp/lowlevel/rafaelia_toroidal_inference.h`; aliases: `Arme/Add/repo_toroidal.h`

The remaining groups are not assigned a canonical path by directory-name
guessing. They remain `TOKEN_VAZIO_AUTHORITY_BLOCKED`.

Byte identity is not semantic identity. This graph authorizes **routing and
dedup analysis**, not deletion. Consumer edges must be observed before a
historical alias can be retired.

Gate:

```bash
python3 tools/validate_source_alias_graph.py
```
