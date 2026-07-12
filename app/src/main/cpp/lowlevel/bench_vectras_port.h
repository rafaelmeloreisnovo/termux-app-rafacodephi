/* bench_vectras_port.h — benchmark kernel contracts
 * Ported from vectras-vm-android/engine/rmr/src/
 * freestanding · no-malloc · NEON · CRC32C · branchless · BSS arena
 */
#ifndef BENCH_VECTRAS_PORT_H
#define BENCH_VECTRAS_PORT_H
#include <stdint.h>
#include <stddef.h>

/* ── Constants (from vectras rmr_bench_suite.c / rmr_neon_simd.c) ──────── */
#define VECTRAS_PHI32      0x9E3779B9u
#define VECTRAS_PHI64      0x9E3779B97F4A7C15ULL
#define VECTRAS_MIX_A      0xBF58476D1CE4E5B9ULL
#define VECTRAS_MIX_B      0x94D049BB133111EBULL
#define VECTRAS_CRC_POLY   0x82F63B78u

/* Branchless conditional (no branch misprediction) */
#define VECTRAS_CSEL(c,t,f) ((t)^(((t)^(f))&(uint32_t)(-(uint32_t)(!!(c)))))

/* Hardware capability flags */
#define VECTRAS_CAP_CNTVCT  (1u<<0)
#define VECTRAS_CAP_CRC32C  (1u<<1)
#define VECTRAS_CAP_NEON    (1u<<2)
#define VECTRAS_CAP_SVE     (1u<<3)

/* Execution profiles (mirrors vectras BenchmarkManager.ExecutionProfile) */
#define VECTRAS_PROF_AUTO        0u
#define VECTRAS_PROF_DETERMINISTIC 1u
#define VECTRAS_PROF_THROUGHPUT  2u
#define VECTRAS_PROF_LOW_LATENCY 3u

/* Benchmark categories (6 total, mirrors vectras VectraBenchmark) */
#define VECTRAS_CAT_CPU_SINGLE   0u
#define VECTRAS_CAT_CPU_MULTI    1u
#define VECTRAS_CAT_MEMORY       2u
#define VECTRAS_CAT_STORAGE      3u
#define VECTRAS_CAT_INTEGRITY    4u
#define VECTRAS_CAT_EMULATION    5u
#define VECTRAS_CAT_MAX          6u

/* Score formula: score = ((ops<<8) / cycles) ^ checksum */
#define VECTRAS_SCORE(ops,cyc,cs) \
    (((uint64_t)((uint64_t)(ops) << 8u) / ((uint64_t)(cyc)|1ULL)) ^ (uint64_t)(cs))

/* ── BSS arena for benchmarks (no malloc) ───────────────────────────────── */
#define VECTRAS_BENCH_ARENA_SZ (128u * 1024u)

/* ── Result struct (64-byte aligned, matches JNI packed format) ─────────── */
typedef struct __attribute__((packed, aligned(64))) VectrasBenchResult {
    uint64_t score;     /* final benchmark score */
    uint64_t cycles;    /* raw cycle count */
    uint64_t ops;       /* operations completed */
    uint64_t checksum;  /* XOR integrity checksum */
    uint32_t category;  /* benchmark category */
    uint32_t profile;   /* execution profile used */
    uint32_t hw_caps;   /* detected HW capabilities */
    uint32_t pad;       /* align to 8 bytes */
} VectrasBenchResult;

/* ── Kernel function declarations ───────────────────────────────────────── */
uint32_t raf_vectras_crc32c(const void *buf, uint32_t len);
uint32_t raf_vectras_xor_fold32(const void *buf, uint32_t len);
void     raf_vectras_memcpy_neon(void *dst, const void *src, uint32_t n);
uint64_t raf_vectras_phi_step(uint64_t acc, uint32_t n);
uint32_t raf_vectras_popcount(const void *buf, uint32_t len);

void raf_vectras_bench_run(uint32_t profile, uint32_t category, VectrasBenchResult *out);

#endif /* BENCH_VECTRAS_PORT_H */
