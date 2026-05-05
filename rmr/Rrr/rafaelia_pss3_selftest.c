#include <assert.h>
#include <stdio.h>
#include "rafaelia_pss3.h"

#define Q16(x) ((uint32_t)((x) * 65536.0))

int main(void){
    rafaelia_pss3_state s; rafaelia_pss3_init(&s);
    assert(rafaelia_pss3_delta(&s)==0); assert(rafaelia_pss3_is_stable(&s,1));

    rafaelia_pss3_step(&s, Q16(0.5), Q16(0.4), Q16(0.8), 1); assert(rafaelia_pss3_delta(&s) <= PSS3_DELTA_Q16);
    rafaelia_pss3_step(&s, Q16(1.0), Q16(0.5), Q16(0.64), 1); assert(rafaelia_pss3_delta(&s) <= PSS3_DELTA_Q16);
    rafaelia_pss3_step(&s, Q16(1.0), Q16(1.0), Q16(0.75), 2); assert(rafaelia_pss3_delta(&s) > PSS3_DELTA_Q16);

    rafaelia_pss3_state low, hi; rafaelia_pss3_init(&low); rafaelia_pss3_init(&hi);
    rafaelia_pss3_step(&low,Q16(0.2),Q16(0.5),Q16(0.7),1);
    rafaelia_pss3_step(&hi,Q16(0.8),Q16(0.5),Q16(0.7),1);
    assert(hi.delta_q16 > low.delta_q16);

    rafaelia_pss3_step(&hi,Q16(0.8),Q16(0.9),Q16(0.7),1); assert(hi.delta_q16 > low.delta_q16);
    uint32_t before = hi.delta_q16; rafaelia_pss3_step(&hi,Q16(0.8),Q16(0.9),Q16(0.95),1); assert(hi.delta_q16 < before);

    rafaelia_pss3_step(&hi,Q16(0.8),Q16(0.9),Q16(0.7),3); assert(!rafaelia_pss3_is_stable(&hi,1));

    rafaelia_pss3_state d1, d2; rafaelia_pss3_init(&d1); rafaelia_pss3_init(&d2);
    rafaelia_pss3_step(&d1,Q16(0.4),Q16(0.4),Q16(0.6),2);
    rafaelia_pss3_step(&d2,Q16(0.4),Q16(0.4),Q16(0.6),2);
    assert(d1.delta_q16 == d2.delta_q16 && d1.crc32c == d2.crc32c);

    puts("rafaelia_pss3_selftest: OK");
    return 0;
}
