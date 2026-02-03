// file: rafpkg_cpu_id.c
// RAFAELIA :: low-level CPU/ABI feature probe (hosted, minimal deps)
// SPDX-License-Identifier: MIT

#include "rafpkg_cpu_id.h"

#include <unistd.h>
#include <fcntl.h>

// Linux auxv tags
#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif
#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif

// AArch64 HWCAP bits (subset). See linux/arch/arm64/include/uapi/asm/hwcap.h
#define HWCAP_ASIMD   (1ULL << 1)
#define HWCAP_AES     (1ULL << 3)
#define HWCAP_PMULL   (1ULL << 4)
#define HWCAP_SHA1    (1ULL << 5)
#define HWCAP_SHA2    (1ULL << 6)
#define HWCAP_CRC32   (1ULL << 7)
#define HWCAP_ATOMICS (1ULL << 8)
#define HWCAP_SVE     (1ULL << 22)

// Some distros expose SHA3/SHA512 in HWCAP2 or later HWCAP expansions.
// We'll probe both fields and map conservatively.

typedef struct {
  unsigned long a_type;
  unsigned long a_val;
} auxv_t;

static void read_auxv(u64 *hwcap, u64 *hwcap2) {
  if (hwcap) *hwcap = 0;
  if (hwcap2) *hwcap2 = 0;

  int fd = open("/proc/self/auxv", O_RDONLY);
  if (fd < 0) return;

  auxv_t e;
  while (read(fd, &e, sizeof(e)) == (ssize_t)sizeof(e)) {
    if (e.a_type == 0) break;
    if (e.a_type == AT_HWCAP && hwcap) *hwcap = (u64)e.a_val;
    if (e.a_type == AT_HWCAP2 && hwcap2) *hwcap2 = (u64)e.a_val;
  }
  close(fd);
}

static void zmem(void *p, u32 n) {
  unsigned char *b = (unsigned char*)p;
  for (u32 i = 0; i < n; i++) b[i] = 0;
}

void raf_cpu_probe(raf_cpu_id_t *id) {
  if (!id) return;
  zmem(id, (u32)sizeof(*id));

#if defined(__aarch64__)
  id->is_aarch64 = 1;
#else
  id->is_aarch64 = 0;
#endif

  long ps = sysconf(_SC_PAGESIZE);
  id->page_size = (ps > 0) ? (u32)ps : 4096u;

  read_auxv(&id->hwcap, &id->hwcap2);

  if (id->is_aarch64) {
    id->asimd   = (id->hwcap & HWCAP_ASIMD)   ? 1u : 0u;
    id->aes     = (id->hwcap & HWCAP_AES)     ? 1u : 0u;
    id->pmull   = (id->hwcap & HWCAP_PMULL)   ? 1u : 0u;
    id->sha1    = (id->hwcap & HWCAP_SHA1)    ? 1u : 0u;
    id->sha2    = (id->hwcap & HWCAP_SHA2)    ? 1u : 0u;
    id->crc32   = (id->hwcap & HWCAP_CRC32)   ? 1u : 0u;
    id->atomics = (id->hwcap & HWCAP_ATOMICS) ? 1u : 0u;
    id->sve     = (id->hwcap & HWCAP_SVE)     ? 1u : 0u;

    // best-effort: try infer sha512/sha3 from hwcap2 non-zero (varies by kernel)
    id->sha512  = (id->hwcap2 != 0) ? 1u : 0u;
    id->sha3    = (id->hwcap2 != 0) ? 1u : 0u;
  }
}

static void app(char *out, u32 cap, const char *s) {
  if (!out || cap == 0) return;
  u32 n = 0;
  while (out[n] && n < cap) n++;
  if (n >= cap) return;
  for (u32 i = 0; s[i] && (n + 1) < cap; i++) out[n++] = s[i];
  out[n] = 0;
}

void raf_cpu_recommend_cflags(const raf_cpu_id_t *id, char *out, u32 out_cap) {
  if (!out || out_cap == 0) return;
  out[0] = 0;
  if (!id) { app(out, out_cap, "-O2"); return; }

  // Always safe baseline
  app(out, out_cap, "-O2 -fno-plt -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables");

  if (id->is_aarch64) {
    // AArch64 baseline; keep conservative to avoid breaking older devices.
    app(out, out_cap, " -march=armv8-a");
    if (id->asimd) app(out, out_cap, "+simd");
    if (id->crc32) app(out, out_cap, "+crc");
    if (id->aes || id->pmull || id->sha1 || id->sha2) {
      // crypto suite: prefer a single +crypto if supported by toolchain
      app(out, out_cap, "+crypto");
    }
    if (id->sve) {
      // optional: only if your code path actually uses SVE intrinsics
      app(out, out_cap, " -msve-vector-bits=128");
    }
  }
}
