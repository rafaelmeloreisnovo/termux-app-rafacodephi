/* RAFAELIA-L0-FILE-CONTRACT
 * PURPOSE: deterministic, bounded caller-owned matrix routing and dual watchdog.
 * SCOPE: OS-independent L0. No syscalls, libc, heap, network, Java, JNI or logging.
 * PRECONDITIONS: disjoint buffers, word counts accurate, caller measures ticks.
 * REGISTER_OWNERSHIP: compiler; CLOBBERS: NONE; MEMORY_ORDER: NONE.
 * TAIL_SHADOW: fixed frame; no loop, dynamic dispatch, callback or hidden retry.
 * EVIDENCE: policy/plan only, not a running watchdog or physical failover.
 */
#include "raf_mw.h"
void raf_mw_route(const raf_mw_u32 *request, raf_mw_u32 request_words,
                  raf_mw_u32 *result, raf_mw_u32 result_words) {
  if (!result || result_words < RAF_MW_WORDS) return;
  result[0]=RAF_MW_INVALID; result[1]=0u; result[2]=0u;
  result[3]=0u; result[4]=0u; result[5]=0u;
  result[6]=0u; result[7]=0u; result[8]=0u;
  result[9]=0u; result[10]=0u; result[11]=0u;
  result[12]=0u; result[13]=0u; result[14]=0u; result[15]=RAF_MW_ABI;
  if (!request || request_words < RAF_MW_WORDS) return;
  const raf_mw_u32 primary=request[RAF_IN_PRIMARY];
  const raf_mw_u32 allowed=request[RAF_IN_ALLOWED];
  const raf_mw_u32 evidence=request[RAF_IN_EVIDENCED];
  const raf_mw_u32 faulted=request[RAF_IN_FAULTED];
  const raf_mw_u32 alternates=request[RAF_IN_FALLBACK];
  const raf_mw_u32 profile=request[RAF_IN_PROFILE];
  const raf_mw_u32 flags=request[RAF_IN_FLAGS];
  result[RAF_OUT_FORMAT]=request[RAF_IN_FORMAT];
  result[RAF_OUT_PROFILE]=profile;
  result[RAF_OUT_ORIGINAL_PRIMARY]=primary;
  result[RAF_OUT_FLAGS]=flags;
  result[RAF_OUT_EXTERNAL_DENIED]=(allowed|primary|alternates)&RAF_LANE_EXTERNAL;
  if (request[RAF_IN_ABI]!=RAF_MW_ABI || profile>RAF_MW_ENERGY ||
      request[RAF_IN_FORMAT]>13u ||
      (flags & ~(RAF_MW_ALLOW_FALLBACK|RAF_MW_ALLOW_FAILOVER|RAF_MW_WANT_ROLLBACK))!=0u ||
      primary==0u || (primary&(primary-1u))!=0u ||
      (primary & ~(RAF_LANE_CPU|RAF_LANE_EXTERNAL))!=0u ||
      request[RAF_IN_QUALITY]>100u || request[RAF_IN_QUALITY_FLOOR]>100u ||
      request[RAF_IN_IDLE_LIMIT]==0u || request[RAF_IN_TOTAL_LIMIT]==0u) {
    result[RAF_OUT_STATUS]=RAF_MW_BAD_ABI;
    return;
  }
  if (request[RAF_IN_HOST_PERMISSION]!=1u) {
    result[RAF_OUT_STATUS]=RAF_MW_NOT_PERMITTED;
    return;
  }
  const raf_mw_u32 progress_fail=(request[RAF_IN_IDLE_TICKS]>request[RAF_IN_IDLE_LIMIT]);
  const raf_mw_u32 total_fail=(request[RAF_IN_TOTAL_TICKS]>request[RAF_IN_TOTAL_LIMIT]);
  result[RAF_OUT_WD_PROGRESS]=progress_fail;
  result[RAF_OUT_WD_TOTAL]=total_fail;
  if (progress_fail|total_fail) {
    result[RAF_OUT_STATUS]=RAF_MW_WATCHDOG;
    result[RAF_OUT_ROLLBACK_SUGGESTED]=(flags&RAF_MW_WANT_ROLLBACK)!=0u;
    return;
  }
  const raf_mw_u32 quality_ok=request[RAF_IN_QUALITY]>=request[RAF_IN_QUALITY_FLOOR];
  result[RAF_OUT_QUALITY_OK]=quality_ok;
  if (!quality_ok) {
    result[RAF_OUT_STATUS]=RAF_MW_NO_ROUTE;
    result[RAF_OUT_ROLLBACK_SUGGESTED]=(flags&RAF_MW_WANT_ROLLBACK)!=0u;
    return;
  }
  const raf_mw_u32 eligible=(allowed&evidence&~faulted&RAF_LANE_CPU);
  result[RAF_OUT_CPU_ELIGIBLE]=eligible;
  const raf_mw_u32 primary_ok=(primary&eligible)!=0u;
  if (primary_ok) {
    result[RAF_OUT_STATUS]=RAF_MW_PRIMARY_PLAN;
    result[RAF_OUT_SELECTED]=primary;
    return;
  }
  const raf_mw_u32 had_fault=(primary&faulted)!=0u;
  const raf_mw_u32 fallback_allowed=((flags&RAF_MW_ALLOW_FALLBACK)!=0u) &&
                               (!had_fault || (flags&RAF_MW_ALLOW_FAILOVER)!=0u);
  const raf_mw_u32 options=eligible&alternates;
  if (fallback_allowed && options) {
    /* Stable total ordering; no hardware detection or hidden scheduling.
       Safe/energy prefer scalar; throughput prefers NEON > SIMD > scalar.
       Non-loop mask selection is a plan, not an SIMD/GPU kernel.
    */
    const raf_mw_u32 a=(profile==RAF_MW_THROUGHPUT)
       ? ((options&RAF_LANE_NEON) ? RAF_LANE_NEON :
          (options&RAF_LANE_SIMD) ? RAF_LANE_SIMD : RAF_LANE_SCALAR)
       : ((options&RAF_LANE_SCALAR) ? RAF_LANE_SCALAR :
          (profile==RAF_MW_ENERGY && (options&RAF_LANE_SIMD)) ? RAF_LANE_SIMD :
          (options&RAF_LANE_NEON) ? RAF_LANE_NEON : RAF_LANE_SIMD);
    result[RAF_OUT_SELECTED]=a;
    result[RAF_OUT_STATUS]=had_fault ? RAF_MW_FAILOVER_PLAN : RAF_MW_FALLBACK_PLAN;
    result[RAF_OUT_USED_FAILOVER]=had_fault;
    result[RAF_OUT_USED_FALLBACK]=1u;
    return;
  }
  result[RAF_OUT_STATUS]=(primary&RAF_LANE_EXTERNAL) ? RAF_MW_HOST_REQUIRED : RAF_MW_NO_ROUTE;
  result[RAF_OUT_ROLLBACK_SUGGESTED]=(flags&RAF_MW_WANT_ROLLBACK)!=0u;
}
