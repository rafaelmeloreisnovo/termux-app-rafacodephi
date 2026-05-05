#include <assert.h>
#include <stdio.h>
#include "../rmr/Rrr/rafaelia_pss3.h"
int main(void){
  rafaelia_pss3_state s; rafaelia_pss3_init(&s);
  rafaelia_pss3_step(&s,0,0,0,0); assert(rafaelia_pss3_is_stable(&s,0));
  assert(rafaelia_pss3_token_invariant(&s)==PSS3_DELTA_Q16);
  rafaelia_pss3_step(&s,6553,6553,65535,1); assert(rafaelia_pss3_is_stable(&s,0));
  rafaelia_pss3_step(&s,11796,65535,65535,1); assert(rafaelia_pss3_is_stable(&s,0));
  rafaelia_pss3_step(&s,65535,65535,0,1); assert(!rafaelia_pss3_is_stable(&s,0));
  assert(rafaelia_pss3_token_invariant(&s)==0u);
  rafaelia_pss3_step(&s,20000,60000,20000,2); uint32_t risk_hi=s.delta_q16;
  rafaelia_pss3_step(&s,5000,10000,50000,2); uint32_t risk_lo=s.delta_q16; assert(risk_hi>risk_lo);
  rafaelia_pss3_step(&s,20000,50000,60000,2); uint32_t with_stability=s.delta_q16; assert(with_stability<risk_hi);
  rafaelia_pss3_step(&s,10000,10000,10000,3); assert(!rafaelia_pss3_is_stable(&s,1));
  uint32_t c1=rafaelia_pss3_crc(&s), c2=rafaelia_pss3_crc(&s); assert(c1==c2);
  puts("pss3 selftest ok");
  return 0;
}
