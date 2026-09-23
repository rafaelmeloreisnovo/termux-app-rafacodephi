# Multilayer Geometry Portable C99 V1

**Repository:** `rafaelmeloreisnovo/termux-app-rafacodephi`  
**Role:** Termux native CLI consumer  
**State:** `IMPLEMENTED_ON_BRANCH / SOURCE_PARITY_WITH_RLL / PROVIDER_CI_NOT_RUN`

Build in Termux:

```sh
pkg install clang
CC=clang sh tools/multilayer_geometry/build_and_selftest.sh
```

Examples:

```sh
tools/multilayer_geometry/multilayer_geometry polygon 8 1 3
tools/multilayer_geometry/multilayer_geometry layer 2 0.4142135623730951 3
tools/multilayer_geometry/multilayer_geometry triangle 2
tools/multilayer_geometry/multilayer_geometry sphere 2.4 1.5707963267948966
```

Publication authority:
`rafaelmeloreisnovo/papers/papers/multilayer_circle_polygon_geometry_v1/paper.md`

RLL contract:
`instituto-Rafael/relativity-living-light/data/contracts/rll_multilayer_circle_polygon_geometry_v1.yml`

Boundary: this CLI is mathematical geometry. It does not establish physical or cosmological mechanisms.
