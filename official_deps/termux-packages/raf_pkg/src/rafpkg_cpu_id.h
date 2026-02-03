// file: rafpkg_cpu_id.h
// RAFAELIA :: low-level CPU/ABI feature probe (hosted, minimal deps)
// SPDX-License-Identifier: MIT

#pragma once

#include "rafpkg_common.h"

typedef struct {
  u32 is_aarch64;
  u32 page_size;
  u64 hwcap;
  u64 hwcap2;
  // common features (AArch64)
  u32 asimd;    // NEON/ASIMD
  u32 aes;
  u32 pmull;
  u32 sha1;
  u32 sha2;
  u32 sha512;
  u32 sha3;
  u32 crc32;
  u32 atomics;
  u32 sve;
} raf_cpu_id_t;

// Probe CPU features (best effort). Safe to call on any Linux.
void raf_cpu_probe(raf_cpu_id_t *id);

// Emit recommended compile flags for clang/gcc (best effort).
// out is always NUL-terminated.
void raf_cpu_recommend_cflags(const raf_cpu_id_t *id, char *out, u32 out_cap);
