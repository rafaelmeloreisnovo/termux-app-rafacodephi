#include "../rafaelia/src/main/cpp/zero/include/rafz.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_crc32c_fixed.h"

static int check(rafz_u32 actual, rafz_u32 expected) {
    return actual == expected ? 0 : 1;
}

int main(void) {
    static const rafz_u8 v8[8] = {'1','2','3','4','5','6','7','8'};
    static const rafz_u8 v9[9] = {'1','2','3','4','5','6','7','8','9'};
    static const rafz_u8 v40[40] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u, 32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u};
    static const rafz_u8 v64[64] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u, 32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u, 40u, 41u, 42u, 43u, 44u, 45u, 46u, 47u, 48u, 49u, 50u, 51u, 52u, 53u, 54u, 55u, 56u, 57u, 58u, 59u, 60u, 61u, 62u, 63u};
    int failures = 0;

    failures += check(
        rafz_pure_crc32c_fixed9(v9),
        0xE3069283u);
    failures += check(
        rafz_pure_crc32c_fixed8(v8),
        rafz_crc32c(v8, 8u));
    failures += check(
        rafz_pure_crc32c_fixed9(v9),
        rafz_crc32c(v9, 9u));
    failures += check(
        rafz_pure_crc32c_fixed40(v40),
        rafz_crc32c(v40, 40u));
    failures += check(
        rafz_pure_crc32c_fixed64(v64),
        rafz_crc32c(v64, 64u));

    return failures;
}
