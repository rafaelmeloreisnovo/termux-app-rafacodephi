#include "../rafaelia/src/main/cpp/zero/include/rafz.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_q16.h"

static int check_u32(rafz_u32 actual, rafz_u32 expected) {
    return actual == expected ? 0 : 1;
}

int main(void) {
    int failures = 0;
    const rafz_u32 one = RAFZ_Q16_ONE;
    const rafz_u32 half = one >> 1u;
    const rafz_u32 quarter = one >> 2u;
    const rafz_u32 a8[8] = {half,half,half,half,half,half,half,half};
    const rafz_u32 b8[8] = {quarter,quarter,quarter,quarter,quarter,quarter,quarter,quarter};

    failures += check_u32(rafz_pure_q16_sub_sat(0x2000u, 0x1000u), 0x1000u);
    failures += check_u32(rafz_pure_q16_sub_sat(0x1000u, 0x2000u), 0u);
    failures += check_u32(rafz_pure_q16_avg_floor(1u, 1u), 1u);
    failures += check_u32(rafz_pure_q16_avg_floor(0xffffffffu, 0xffffffffu), 0xffffffffu);
    failures += check_u32(rafz_pure_q16_from_u8_frac((rafz_u8)255u), 0x0000ff00u);
    failures += check_u32((rafz_u32)rafz_pure_q16_to_u8_frac(0x0000ff00u), 255u);
    failures += check_u32((rafz_u32)rafz_pure_q16_to_u8_frac(one), 255u);
    failures += check_u32(rafz_pure_q16_mul(one, one), one);
    failures += check_u32(rafz_pure_q16_mul(half, half), quarter);
    failures += check_u32(rafz_pure_q16_phi(0u, one), one);
    failures += check_u32(rafz_pure_q16_phi(one, one), 0u);
    failures += check_u32(rafz_pure_q16_phi(quarter, half), 0x00006000u);
    failures += check_u32(rafz_pure_q16_dot8(a8, b8), one);

    return failures;
}
