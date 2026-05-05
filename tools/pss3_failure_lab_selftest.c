#include <assert.h>
#include <stdio.h>
#include "../rmr/Rrr/rafaelia_pss3.h"

int main(void) {
    rafaelia_pss3_state s;
    rafaelia_pss3_init(&s);

    rafaelia_pss3_step(&s, 0, 0, 65535, 0); assert(rafaelia_pss3_delta(&s) == 0); assert(rafaelia_pss3_is_stable(&s, 0));
    rafaelia_pss3_step(&s, 65535, 65535, 58982, 1); assert(rafaelia_pss3_delta(&s) <= PSS3_DELTA_Q16);
    rafaelia_pss3_step(&s, 65535, 65535, 53739, 1); assert(rafaelia_pss3_delta(&s) <= PSS3_DELTA_Q16 + 1);
    rafaelia_pss3_step(&s, 65535, 65535, 49152, 1); assert(rafaelia_pss3_delta(&s) > PSS3_DELTA_Q16);

    rafaelia_pss3_step(&s, 40000, 45000, 40000, 1); uint32_t r1 = rafaelia_pss3_delta(&s);
    rafaelia_pss3_step(&s, 60000, 45000, 40000, 1); uint32_t r2 = rafaelia_pss3_delta(&s); assert(r2 > r1);
    rafaelia_pss3_step(&s, 60000, 60000, 40000, 1); uint32_t r3 = rafaelia_pss3_delta(&s); assert(r3 > r2);
    rafaelia_pss3_step(&s, 60000, 60000, 50000, 1); uint32_t r4 = rafaelia_pss3_delta(&s); assert(r4 < r3);
    rafaelia_pss3_step(&s, 20000, 20000, 20000, 3); assert(!rafaelia_pss3_is_stable(&s, 1));
    rafaelia_pss3_step(&s, 20000, 20000, 20000, 2); uint32_t a = rafaelia_pss3_delta(&s); rafaelia_pss3_step(&s, 20000, 20000, 20000, 2); uint32_t b = rafaelia_pss3_delta(&s); assert(a == b);

    puts("pss3 selftest ok");
    return 0;
}
