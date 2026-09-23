# Receipt — Termux Multilayer Geometry C99 V1

State: `IMPLEMENTED_ON_BRANCH / DRAFT_PR / PROVIDER_CI_NOT_RUN`

The portable source is the same consumer kernel used by the RLL bridge. A source-equivalent local build before materialization produced 35 PASS / 0 FAIL.

Target command:

```sh
CC=clang sh tools/multilayer_geometry/build_and_selftest.sh
```

Device-native Termux execution remains a separate evidence gate.
