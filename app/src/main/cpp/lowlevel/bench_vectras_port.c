/* bench_vectras_port.c — NEON/CRC32C/ALU benchmark kernels
 * Ported from vectras-vm-android engine/rmr/src/rmr_neon_simd.c
 *              and engine/rmr/src/rmr_bench_suite.c
 * freestanding · no-malloc · no-libc · NEON intrinsics · CRC32C HW · branchless
 */
#include "bench_vectras_port.h"
#include "api_lowlevel.h"  /* api_ll_cycle_read, api_ll_crc32c_hw */

#if defined(__aarch64__) && defined(HAS_NEON)
#include <arm_neon.h>
#endif

/* ── BSS arena (no malloc — bump allocator, static) ─────────────────────── */
static uint8_t __attribute__((aligned(64))) g_bench_arena[VECTRAS_BENCH_ARENA_SZ];
static uint32_t g_bench_mark;   /* arena mark for VOS_MARK/VOS_RESTORE pattern */

#define ARENA_MARK()     uint32_t _mark = g_bench_mark
#define ARENA_ALLOC(n)   (g_bench_mark + (n) <= VECTRAS_BENCH_ARENA_SZ \
                         ? (void*)(g_bench_arena + (g_bench_mark += (n)) - (n)) \
                         : (void*)0)
#define ARENA_RESTORE()  g_bench_mark = _mark

/* ── CRC32C HW/SW dispatch ───────────────────────────────────────────────── */
uint32_t raf_vectras_crc32c(const void *buf, uint32_t len) {
#if defined(__aarch64__) && defined(HAS_CRC32C_HW)
    return api_ll_crc32c_hw(0xFFFFFFFFu, buf, len) ^ 0xFFFFFFFFu;
#else
    return api_ll_crc32c_sw(buf, len);
#endif
}

/* ── NEON XOR fold: 64 bytes/cycle (4× unrolled), ARM64 only ──────────── */
uint32_t raf_vectras_xor_fold32(const void *buf, uint32_t len) {
#if defined(__aarch64__) && defined(HAS_NEON)
    const uint8_t *p = (const uint8_t*)buf;
    uint8x16_t acc = vdupq_n_u8(0u);
    /* 64-byte main loop */
    for (; len >= 64u; len -= 64u, p += 64u) {
        uint8x16x4_t v = vld1q_u8_x4(p);
        acc = veorq_u8(acc, v.val[0]);
        acc = veorq_u8(acc, v.val[1]);
        acc = veorq_u8(acc, v.val[2]);
        acc = veorq_u8(acc, v.val[3]);
    }
    /* 16-byte tail */
    for (; len >= 16u; len -= 16u, p += 16u)
        acc = veorq_u8(acc, vld1q_u8(p));
    /* horizontal fold 128→32 bits */
    uint32x4_t a32 = vreinterpretq_u32_u8(acc);
    uint64x2_t a64 = vreinterpretq_u64_u32(a32);
    uint32_t lo = (uint32_t)vgetq_lane_u64(a64, 0);
    uint32_t hi = (uint32_t)(vgetq_lane_u64(a64, 0) >> 32u);
    uint32_t lo2= (uint32_t)vgetq_lane_u64(a64, 1);
    uint32_t hi2= (uint32_t)(vgetq_lane_u64(a64, 1) >> 32u);
    uint32_t r = lo ^ hi ^ lo2 ^ hi2;
    /* scalar tail */
    const uint8_t *q = p;
    while (len--) r ^= *q++;
    return r;
#else
    /* SW scalar fallback */
    const uint8_t *p = (const uint8_t*)buf;
    uint32_t acc = 0u;
    uint32_t i;
    for (i = 0u; i + 4u <= len; i += 4u) {
        uint32_t v;
        __builtin_memcpy(&v, p + i, 4u);
        acc ^= v;
    }
    for (; i < len; i++) acc ^= (uint32_t)p[i];
    return acc;
#endif
}

/* ── NEON memcpy: 64 bytes/cycle (ARM64), scalar fallback ───────────────── */
void raf_vectras_memcpy_neon(void *dst, const void *src, uint32_t n) {
#if defined(__aarch64__) && defined(HAS_NEON)
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    for (; n >= 64u; n -= 64u, d += 64u, s += 64u) {
        uint8x16x4_t v = vld1q_u8_x4(s);
        vst1q_u8_x4(d, v);
    }
    for (; n >= 16u; n -= 16u, d += 16u, s += 16u)
        vst1q_u8(d, vld1q_u8(s));
    while (n--) *d++ = *s++;
#else
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
#endif
}

/* ── PHI32 bulk step: SplitMix64 mixing over n iterations ───────────────── */
/* Equivalent to rmr_neon_phi_step_bulk — NEON vmulq_u32 × 4 lanes */
uint64_t raf_vectras_phi_step(uint64_t acc, uint32_t n) {
#if defined(__aarch64__) && defined(HAS_NEON)
    uint32x4_t phi = vdupq_n_u32(VECTRAS_PHI32);
    uint32x4_t s   = vdupq_n_u32((uint32_t)acc);
    uint32_t bulk  = n >> 2u;  /* process 4 at a time */
    while (bulk--) {
        s = vmulq_u32(s, phi);
        /* zero-guard: s = csel(s==0, 1, s) branchless via vceqq/vorrq */
        uint32x4_t z = vceqq_u32(s, vdupq_n_u32(0u));
        s = vorrq_u32(s, vandq_u32(z, vdupq_n_u32(1u)));
    }
    acc ^= (uint64_t)vgetq_lane_u32(s, 0) |
           ((uint64_t)vgetq_lane_u32(s, 1) << 32u);
    /* scalar tail */
    n &= 3u;
    while (n--) {
        acc ^= acc >> 30u;
        acc *= VECTRAS_MIX_A;
        acc ^= acc >> 27u;
        acc *= VECTRAS_MIX_B;
        acc ^= acc >> 31u;
    }
    return acc;
#else
    /* SplitMix64 scalar */
    while (n--) {
        acc ^= acc >> 30u; acc *= VECTRAS_MIX_A;
        acc ^= acc >> 27u; acc *= VECTRAS_MIX_B;
        acc ^= acc >> 31u;
    }
    return acc;
#endif
}

/* ── Popcount bulk: vcntq_u8 + vpadalq (ARM64 HW popcount) ──────────────── */
uint32_t raf_vectras_popcount(const void *buf, uint32_t len) {
#if defined(__aarch64__) && defined(HAS_NEON)
    const uint8_t *p = (const uint8_t*)buf;
    uint64x2_t sum = vdupq_n_u64(0u);
    for (; len >= 16u; len -= 16u, p += 16u) {
        uint8x16_t v = vld1q_u8(p);
        uint8x16_t c = vcntq_u8(v);
        sum = vpadalq_u32(sum, vpaddlq_u16(vpaddlq_u8(c)));
    }
    uint32_t r = (uint32_t)(vgetq_lane_u64(sum, 0) + vgetq_lane_u64(sum, 1));
    while (len--) r += (uint32_t)__builtin_popcount(*p++);
    return r;
#else
    const uint8_t *p = (const uint8_t*)buf;
    uint32_t r = 0u;
    while (len--) r += (uint32_t)__builtin_popcount(*p++);
    return r;
#endif
}

/* ══════════════════════════════════════════════════════════════════════════
 * Benchmark kernels — one per category
 * Score formula (rmr_bench_suite.c): score = ((ops<<8)/cycles) ^ checksum
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── CPU Single-threaded: XOR/shift/mul ALU benchmark ───────────────────── */
static void bench_cpu_single(uint32_t profile, VectrasBenchResult *r) {
    const uint32_t iters = (profile == VECTRAS_PROF_THROUGHPUT) ? 8192u : 4096u;
    uint64_t t0 = api_ll_cycle_read();
    uint64_t acc = VECTRAS_PHI64;
    uint32_t i;
    for (i = 0u; i < iters; i++) {
        /* SplitMix64 step — branchless, no memory — measures pure ALU */
        acc ^= acc >> 30u; acc *= VECTRAS_MIX_A;
        acc ^= acc >> 27u; acc *= VECTRAS_MIX_B;
        acc ^= acc >> 31u;
        /* XOR with Boost hash_combine constant */
        acc += 0x9E3779B97F4A7C15ULL + (acc << 6u) + (acc >> 2u);
    }
    uint64_t t1 = api_ll_cycle_read();
    r->cycles   = t1 - t0;
    r->ops      = (uint64_t)iters;
    r->checksum = acc;
    r->score    = VECTRAS_SCORE(r->ops, r->cycles, r->checksum);
}

/* ── CPU Multi-threaded simulation: NEON 4-lane parallel PHI step ────────── */
static void bench_cpu_multi(uint32_t profile, VectrasBenchResult *r) {
    const uint32_t iters = (profile == VECTRAS_PROF_THROUGHPUT) ? 16384u : 8192u;
    uint64_t t0 = api_ll_cycle_read();
    uint64_t acc = raf_vectras_phi_step(VECTRAS_PHI64, iters);
    uint64_t t1 = api_ll_cycle_read();
    r->cycles   = t1 - t0;
    r->ops      = (uint64_t)iters;
    r->checksum = acc;
    r->score    = VECTRAS_SCORE(r->ops, r->cycles, r->checksum);
}

/* ── Memory: NEON memcpy + XOR fold, 64-byte stride through BSS arena ───── */
static void bench_memory(uint32_t profile, VectrasBenchResult *r) {
    ARENA_MARK();
    const uint32_t blksz = (profile == VECTRAS_PROF_THROUGHPUT) ? 32768u : 16384u;
    void *src = ARENA_ALLOC(blksz);
    void *dst = ARENA_ALLOC(blksz);
    if (!src || !dst) { r->score = 0u; ARENA_RESTORE(); return; }
    /* fill src with PHI pattern */
    uint32_t *s = (uint32_t*)src;
    uint32_t n = blksz >> 2u;
    uint32_t v = VECTRAS_PHI32;
    while (n--) { *s++ = v; v *= VECTRAS_PHI32; }
    uint64_t t0 = api_ll_cycle_read();
    raf_vectras_memcpy_neon(dst, src, blksz);
    uint32_t cs = raf_vectras_xor_fold32(dst, blksz);
    uint64_t t1 = api_ll_cycle_read();
    r->cycles   = t1 - t0;
    r->ops      = (uint64_t)blksz;
    r->checksum = (uint64_t)cs;
    r->score    = VECTRAS_SCORE(r->ops, r->cycles, r->checksum);
    ARENA_RESTORE();
}

/* ── Storage: BSS-backed simulated sequential R/W (memory-mapped) ────────── */
static void bench_storage(uint32_t profile, VectrasBenchResult *r) {
    ARENA_MARK();
    const uint32_t blksz = (profile == VECTRAS_PROF_THROUGHPUT) ? 65536u : 32768u;
    uint8_t *buf = (uint8_t*)ARENA_ALLOC(blksz);
    if (!buf) { r->score = 0u; ARENA_RESTORE(); return; }
    uint64_t t0 = api_ll_cycle_read();
    /* Sequential write with stride pattern */
    uint32_t i;
    uint32_t acc = VECTRAS_PHI32;
    for (i = 0u; i < blksz; i += 64u) {
        acc *= VECTRAS_PHI32;
        buf[i] = (uint8_t)(acc >> 24u);
    }
    /* Sequential read with XOR accumulation */
    uint32_t cs = 0u;
    for (i = 0u; i < blksz; i += 64u) {
        acc ^= (uint32_t)buf[i] * VECTRAS_PHI32;
        cs ^= acc;
    }
    uint64_t t1 = api_ll_cycle_read();
    r->cycles   = t1 - t0;
    r->ops      = (uint64_t)(blksz >> 6u) * 2u;  /* read + write passes */
    r->checksum = (uint64_t)cs;
    r->score    = VECTRAS_SCORE(r->ops, r->cycles, r->checksum);
    ARENA_RESTORE();
}

/* ── Integrity: CRC32C 4KB + XOR stripe (mirrors vectras k2 kernel) ──────── */
static void bench_integrity(uint32_t profile, VectrasBenchResult *r) {
    ARENA_MARK();
    const uint32_t blksz = (profile == VECTRAS_PROF_THROUGHPUT) ? 8192u : 4096u;
    uint8_t *buf = (uint8_t*)ARENA_ALLOC(blksz);
    if (!buf) { r->score = 0u; ARENA_RESTORE(); return; }
    /* Fill with deterministic pattern */
    uint32_t i;
    uint32_t v = VECTRAS_PHI32;
    for (i = 0u; i < blksz; i++) { v *= VECTRAS_PHI32; buf[i] = (uint8_t)(v >> 16u); }
    uint64_t t0 = api_ll_cycle_read();
    uint32_t crc = raf_vectras_crc32c(buf, blksz);
    uint32_t xf  = raf_vectras_xor_fold32(buf, blksz);
    uint32_t pc  = raf_vectras_popcount(buf, blksz);
    uint64_t t1  = api_ll_cycle_read();
    r->cycles    = t1 - t0;
    r->ops       = (uint64_t)blksz * 3u;  /* crc + xor + popcount passes */
    r->checksum  = ((uint64_t)crc << 32u) ^ ((uint64_t)xf << 16u) ^ (uint64_t)pc;
    r->score     = VECTRAS_SCORE(r->ops, r->cycles, r->checksum);
    ARENA_RESTORE();
}

/* ── Emulation: cycle-counter precision + context overhead measurement ───── */
static void bench_emulation(uint32_t profile, VectrasBenchResult *r) {
    const uint32_t samples = (profile == VECTRAS_PROF_DETERMINISTIC) ? 1024u : 512u;
    uint64_t t0 = api_ll_cycle_read();
    uint64_t acc = 0u;
    uint32_t i;
    for (i = 0u; i < samples; i++) {
        uint64_t t = api_ll_cycle_read();
        acc ^= t;                          /* XOR accumulate — branchless */
        /* simulate context switch overhead: PHI mix step */
        acc ^= acc >> 30u; acc *= VECTRAS_MIX_A;
    }
    uint64_t t1 = api_ll_cycle_read();
    r->cycles   = t1 - t0;
    r->ops      = (uint64_t)samples;
    r->checksum = acc;
    r->score    = VECTRAS_SCORE(r->ops, r->cycles, r->checksum);
}

/* ── Public run function ─────────────────────────────────────────────────── */
void raf_vectras_bench_run(uint32_t profile, uint32_t category, VectrasBenchResult *out) {
    /* zero out result — no memset from libc */
    uint8_t *p = (uint8_t*)out;
    uint32_t n = (uint32_t)sizeof(VectrasBenchResult);
    while (n--) *p++ = 0u;
    out->profile  = profile;
    out->category = category;
#if defined(__aarch64__)
    out->hw_caps = VECTRAS_CAP_CNTVCT | VECTRAS_CAP_NEON;
#if defined(HAS_CRC32C_HW)
    out->hw_caps |= VECTRAS_CAP_CRC32C;
#endif
#endif
    /* Dispatch table: array of function pointers — no switch */
    typedef void (*bfn_t)(uint32_t, VectrasBenchResult*);
    static const bfn_t tbl[VECTRAS_CAT_MAX] = {
        bench_cpu_single,
        bench_cpu_multi,
        bench_memory,
        bench_storage,
        bench_integrity,
        bench_emulation,
    };
    const uint32_t idx = (category < VECTRAS_CAT_MAX) ? category : 0u;
    tbl[idx](profile, out);
}
