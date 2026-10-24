/* Hosted testing only; NO standard headers included in L0. */
#include <stdio.h>
#include "../raf_mw.h"
static unsigned failures, executed;
static void run(unsigned primary,unsigned allowed,unsigned evidenced,unsigned faults,
                unsigned alternates,unsigned flags,unsigned idle,unsigned total,
                unsigned auth,unsigned quality,unsigned floor,unsigned profile,
                unsigned expected_state,unsigned expected_lane,unsigned expected_roll) {
  raf_mw_u32 x[18]={0}, y[18];
  x[RAF_IN_ABI]=1; x[RAF_IN_PRIMARY]=primary; x[RAF_IN_ALLOWED]=allowed;
  x[RAF_IN_EVIDENCED]=evidenced; x[RAF_IN_FAULTED]=faults;
  x[RAF_IN_FALLBACK]=alternates; x[RAF_IN_FORMAT]=1;
  x[RAF_IN_PROFILE]=profile; x[RAF_IN_FLAGS]=flags;
  x[RAF_IN_IDLE_TICKS]=idle; x[RAF_IN_IDLE_LIMIT]=20;
  x[RAF_IN_TOTAL_TICKS]=total; x[RAF_IN_TOTAL_LIMIT]=100;
  x[RAF_IN_HOST_PERMISSION]=auth; x[RAF_IN_QUALITY]=quality;
  x[RAF_IN_QUALITY_FLOOR]=floor;
  y[16]=0xdeadbeefu; y[17]=0xdeadbeefu;
  raf_mw_route(x,16,y,16);
  executed++;
  if(y[0]!=expected_state || y[1]!=expected_lane || y[8]!=expected_roll ||
     y[14]!=0 || y[15]!=1 || y[16]!=0xdeadbeefu || y[17]!=0xdeadbeefu) {
    fprintf(stderr,"FAIL test %u: got [%u,%u,%u]\n",executed,y[0],y[1],y[8]);failures++;
  }
}
int main(void) {
  const unsigned all=RAF_LANE_SCALAR|RAF_LANE_NEON|RAF_LANE_SIMD;
  const unsigned flags=RAF_MW_ALLOW_FALLBACK|RAF_MW_ALLOW_FAILOVER|RAF_MW_WANT_ROLLBACK;
  run(RAF_LANE_SCALAR,all,all,0,all,flags,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_PRIMARY_PLAN,RAF_LANE_SCALAR,0);
  run(RAF_LANE_NEON,all,all,0,all,flags,0,0,1,100,80,RAF_MW_THROUGHPUT,RAF_MW_PRIMARY_PLAN,RAF_LANE_NEON,0);
  run(RAF_LANE_GPU,all|RAF_LANE_GPU,all,0,all,flags,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_FALLBACK_PLAN,RAF_LANE_SCALAR,0);
  run(RAF_LANE_GPU,RAF_LANE_GPU,RAF_LANE_GPU,0,RAF_LANE_GPU,0,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_HOST_REQUIRED,0,0);
  run(RAF_LANE_SCALAR,all,all,RAF_LANE_SCALAR,all,flags,0,0,1,100,80,RAF_MW_THROUGHPUT,RAF_MW_FAILOVER_PLAN,RAF_LANE_NEON,0);
  run(RAF_LANE_NEON,all,all,RAF_LANE_NEON,all,RAF_MW_ALLOW_FALLBACK|RAF_MW_WANT_ROLLBACK,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_NO_ROUTE,0,1);
  run(RAF_LANE_SCALAR,all,all,0,all,flags,21,0,1,100,80,RAF_MW_SAFE,RAF_MW_WATCHDOG,0,1);
  run(RAF_LANE_SCALAR,all,all,0,all,flags,0,101,1,100,80,RAF_MW_SAFE,RAF_MW_WATCHDOG,0,1);
  run(RAF_LANE_SCALAR,all,all,0,all,flags,0,0,0,100,80,RAF_MW_SAFE,RAF_MW_NOT_PERMITTED,0,0);
  run(RAF_LANE_SCALAR,all,all,0,all,flags,0,0,1,60,80,RAF_MW_SAFE,RAF_MW_NO_ROUTE,0,1);
  run(RAF_LANE_NEON,RAF_LANE_SCALAR,RAF_LANE_SCALAR,0,RAF_LANE_SCALAR,flags,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_FALLBACK_PLAN,RAF_LANE_SCALAR,0);
  run(RAF_LANE_NEON,all,all,RAF_LANE_NEON,RAF_LANE_SIMD,flags,0,0,1,100,80,RAF_MW_ENERGY,RAF_MW_FAILOVER_PLAN,RAF_LANE_SIMD,0);
  run(RAF_LANE_GPU,RAF_LANE_GPU,0,0,0,flags,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_HOST_REQUIRED,0,1);
  run(3,all,all,0,all,flags,0,0,1,100,80,RAF_MW_SAFE,RAF_MW_BAD_ABI,0,0);
  raf_mw_u32 y[18]={0},x[16]={0};
  y[0]=77;raf_mw_route(x,16,y,15);if(y[0]!=77)failures++;
  raf_mw_route(NULL,16,y,16);if(y[0]!=RAF_MW_INVALID)failures++;
  raf_mw_route(x,15,y,16);if(y[0]!=RAF_MW_INVALID)failures++;
  raf_mw_route(x,16,NULL,16);
  printf("%s: %u scenario fixtures + 4 safety guards\n",failures?"FAIL":"PASS",executed);
  return failures?1:0;
}
