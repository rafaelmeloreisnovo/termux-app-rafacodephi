#include "../rafaelia/src/main/cpp/zero/include/rafz.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_bitraf42.h"
#include "../app/src/main/cpp/lowlevel/raf_bitraf.h"

static int check_u64(rafz_bitraf_u64 actual, rafz_bitraf_u64 expected) {
    return actual == expected ? 0 : 1;
}

static int check_u32(rafz_u32 actual, rafz_u32 expected) {
    return actual == expected ? 0 : 1;
}

int main(void) {
    int failures = 0;
    const rafz_bitraf_u64 pure = rafz_pure_bitraf42_encode(1u, 2u, 3u, 4u, 5u);
    const rafz_bitraf_u64 legacy = (rafz_bitraf_u64)bitraf_encode(1u, 2u, 3u, 4u, 5u);
    const rafz_bitraf_u64 maxword =
        rafz_pure_bitraf42_encode(0xFFu, 0xFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu);

    failures += check_u64(pure, (rafz_bitraf_u64)0x0000001401802005ULL);
    failures += check_u64(pure, legacy);
    failures += check_u64(maxword, RAFZ_BITRAF42_WORD_MASK);

    failures += check_u32((rafz_u32)rafz_pure_bitraf42_opcode(maxword), 0x3Fu);
    failures += check_u32((rafz_u32)rafz_pure_bitraf42_dir(maxword), 0x07u);
    failures += check_u32((rafz_u32)rafz_pure_bitraf42_layer(maxword), 0x03FFu);
    failures += check_u32((rafz_u32)rafz_pure_bitraf42_imm(maxword), 0x0FFFu);
    failures += check_u32((rafz_u32)rafz_pure_bitraf42_flags(maxword), 0x07FFu);

    failures += check_u32(rafz_pure_bitraf42_is_valid(maxword), 1u);
    failures += check_u32(
        rafz_pure_bitraf42_is_valid(maxword | ((rafz_bitraf_u64)1u << 42u)),
        0u);

    return failures;
}
