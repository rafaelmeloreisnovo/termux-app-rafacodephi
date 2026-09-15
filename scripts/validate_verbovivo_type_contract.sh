#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/hosted.c" <<'EOF'
#include <stdint.h>
#include <stddef.h>
#include "rafaelia/verbovivo_graph.h"

_Static_assert(sizeof(uint8_t) == 1, "uint8_t width drift");
_Static_assert(sizeof(uint16_t) == 2, "uint16_t width drift");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t width drift");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t width drift");
_Static_assert(sizeof(int64_t) == 8, "int64_t width drift");
_Static_assert(sizeof(size_t) == sizeof(void *), "size_t width drift");
EOF

echo "→ hosted header compatibility"
clang -std=c11 -Wall -Wextra -Werror -I. -fsyntax-only "$tmpdir/hosted.c"

cat > "$tmpdir/freestanding.c" <<'EOF'
#include "rafaelia/verbovivo_graph.h"

_Static_assert(sizeof(uint8_t) == 1, "uint8_t width drift");
_Static_assert(sizeof(uint16_t) == 2, "uint16_t width drift");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t width drift");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t width drift");
_Static_assert(sizeof(int64_t) == 8, "int64_t width drift");
_Static_assert(sizeof(size_t) == sizeof(void *), "size_t width drift");
EOF

for target in armv7a-none-eabi aarch64-none-elf; do
  echo "→ freestanding type contract: $target"
  clang --target="$target" \
    -std=c11 -ffreestanding -nostdinc \
    -Wall -Wextra -Werror -I. \
    -fsyntax-only "$tmpdir/freestanding.c"
done

echo "✅ Verbovivo hosted + ARM32/ARM64 freestanding type contract passed"
