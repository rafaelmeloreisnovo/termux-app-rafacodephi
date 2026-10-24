/* RAFAELIA-L0-FILE-CONTRACT
 * PURPOSE: stable 16x32-bit ABI for deterministic, zero-runtime route planning.
 * SCOPE: PURE_CORE; no OS/JNI/ART/NUMA/GPU operations.
 * PRECONDITIONS: caller-owned non-overlapping buffers of declared word counts.
 * REGISTER_OWNERSHIP: compiler; CLOBBERS: NONE; MEMORY_ORDER: NONE.
 * TAIL_SHADOW: none; no hidden retries/loops/tails/allocations.
 * EVIDENCE: a plan is not execution, authorization or device attestation.
 */
#ifndef RAFAELIA_L0_MIDDLEWARE_V1_H
#define RAFAELIA_L0_MIDDLEWARE_V1_H
typedef unsigned int raf_mw_u32;
_Static_assert(sizeof(raf_mw_u32) == 4, "requires 32-bit unsigned int");
enum { RAF_MW_WORDS = 16, RAF_MW_ABI = 1 };
enum {
  RAF_LANE_SCALAR=1u, RAF_LANE_NEON=2u, RAF_LANE_SIMD=4u,
  RAF_LANE_GPU=8u, RAF_LANE_NUMA=16u,
  RAF_LANE_CPU=RAF_LANE_SCALAR|RAF_LANE_NEON|RAF_LANE_SIMD,
  RAF_LANE_EXTERNAL=RAF_LANE_GPU|RAF_LANE_NUMA
};
enum { RAF_MW_ALLOW_FALLBACK=1u, RAF_MW_ALLOW_FAILOVER=2u,
       RAF_MW_WANT_ROLLBACK=4u };
enum { RAF_MW_SAFE=0u, RAF_MW_THROUGHPUT=1u, RAF_MW_ENERGY=2u };
enum {
  RAF_MW_INVALID=1u, RAF_MW_BAD_ABI=2u, RAF_MW_NOT_PERMITTED=3u,
  RAF_MW_WATCHDOG=4u, RAF_MW_NO_ROUTE=5u, RAF_MW_HOST_REQUIRED=6u,
  RAF_MW_PRIMARY_PLAN=7u, RAF_MW_FALLBACK_PLAN=8u,
  RAF_MW_FAILOVER_PLAN=9u
};
enum {
  RAF_IN_ABI=0, RAF_IN_PRIMARY=1, RAF_IN_ALLOWED=2,
  RAF_IN_EVIDENCED=3, RAF_IN_FAULTED=4, RAF_IN_FALLBACK=5,
  RAF_IN_FORMAT=6, RAF_IN_PROFILE=7, RAF_IN_FLAGS=8,
  RAF_IN_IDLE_TICKS=9, RAF_IN_IDLE_LIMIT=10,
  RAF_IN_TOTAL_TICKS=11, RAF_IN_TOTAL_LIMIT=12,
  RAF_IN_HOST_PERMISSION=13, RAF_IN_QUALITY=14, RAF_IN_QUALITY_FLOOR=15
};
enum {
  RAF_OUT_STATUS=0, RAF_OUT_SELECTED=1, RAF_OUT_CPU_ELIGIBLE=2,
  RAF_OUT_EXTERNAL_DENIED=3, RAF_OUT_WD_PROGRESS=4,
  RAF_OUT_WD_TOTAL=5, RAF_OUT_USED_FALLBACK=6,
  RAF_OUT_USED_FAILOVER=7, RAF_OUT_ROLLBACK_SUGGESTED=8,
  RAF_OUT_FORMAT=9, RAF_OUT_PROFILE=10, RAF_OUT_QUALITY_OK=11,
  RAF_OUT_ORIGINAL_PRIMARY=12, RAF_OUT_FLAGS=13,
  RAF_OUT_EXECUTED=14, RAF_OUT_ABI=15
};
/* Buffer words must be >=16 and disjoint. There is no external callback.
 * Any selected lane is a plan token; output[14] remains 0 always.
 * A caller-supplied EVIDENCED mask is NOT independent capability evidence.
 * The HOST_PERMISSION bit is a policy input, not proof of human approval.
 */
void raf_mw_route(const raf_mw_u32 *request, raf_mw_u32 request_words,
                  raf_mw_u32 *result, raf_mw_u32 result_words);
#endif
