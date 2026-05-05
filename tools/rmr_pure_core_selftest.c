#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../rmr/include/rmr_hex_const.h"

static uint32_t ref_q16_sqrt3_2(void) {
    const double ref = sqrt(3.0) / 2.0;
    return (uint32_t)llround(ref * 65536.0);
}

int main(void) {
    const uint32_t candidate_a = UINT32_C(0x0000DDB3);
    const uint32_t candidate_b = UINT32_C(0x0000DDB4);
    const uint32_t ref = ref_q16_sqrt3_2();

    const uint32_t err_a = (candidate_a > ref) ? (candidate_a - ref) : (ref - candidate_a);
    const uint32_t err_b = (candidate_b > ref) ? (candidate_b - ref) : (ref - candidate_b);
    const uint32_t official = RMR_Q16_SQRT3_2;

    printf("RMR pure-core Q16 sqrt(3)/2 decision test\n");
    printf("ref_q16=0x%08X\n", ref);
    printf("candidate 0x0000DDB3 abs_err=%u\n", err_a);
    printf("candidate 0x0000DDB4 abs_err=%u\n", err_b);
    printf("official RMR_Q16_SQRT3_2=0x%08X\n", official);

    if (err_b < err_a && official == candidate_b) {
        printf("PASS: selected 0x0000DDB4 as single official constant.\n");
        return 0;
    }

    fprintf(stderr, "FAIL: official constant is not aligned with minimum abs error criterion.\n");
    return 1;
}
