#include "../rafaelia/src/main/cpp/zero/include/rafz.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_bitraf42.h"
#include "../app/src/main/cpp/lowlevel/raf_bitraf.h"

static int check_u64(rafz_bitraf42_u64 a, rafz_bitraf42_u64 b) {
    return a == b ? 0 : 1;
}

static int check_u32(rafz_u32 a, rafz_u32 b) {
    return a == b ? 0 : 1;
}

static int check_s32(rafz_s32 a, rafz_s32 b) {
    return a == b ? 0 : 1;
}

static int run_case(
    rafz_u8 opcode,
    rafz_u8 dir,
    rafz_u16 layer,
    rafz_u16 imm,
    rafz_u16 flags) {
    const rafz_bitraf42_u64 pure =
        rafz_pure_bitraf42_encode(opcode, dir, layer, imm, flags);
    const uint64_t lowlevel = bitraf_encode(opcode, dir, layer, imm, flags);
    uint8_t o = 0u;
    uint8_t d = 0u;
    uint16_t l = 0u;
    uint16_t m = 0u;
    uint16_t f = 0u;
    int failures = 0;

    bitraf_decode(lowlevel, &o, &d, &l, &m, &f);
    failures += check_u64(pure, (rafz_bitraf42_u64)lowlevel);
    failures += check_u32(rafz_pure_bitraf42_opcode(pure), o);
    failures += check_u32(rafz_pure_bitraf42_dir(pure), d);
    failures += check_u32(rafz_pure_bitraf42_layer(pure), l);
    failures += check_u32(rafz_pure_bitraf42_imm(pure), m);
    failures += check_u32(rafz_pure_bitraf42_flags(pure), f);
    failures += check_s32(
        rafz_pure_bitraf42_validate(pure),
        (rafz_s32)bitraf_validate(lowlevel));
    return failures;
}

int main(void) {
    int failures = 0;
    const rafz_bitraf42_u64 overflow =
        ((rafz_bitraf42_u64)1u << 42u) | 0x12345u;

    failures += run_case(1u, 2u, 3u, 4u, 5u);
    failures += run_case(63u, 7u, 1023u, 4095u, 2047u);
    failures += run_case(0xFFu, 0xFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu);
    failures += check_u64(
        rafz_pure_bitraf42_encode(63u, 7u, 1023u, 4095u, 2047u),
        RAFZ_BITRAF42_MASK);
    failures += check_u32(
        rafz_pure_bitraf42_is_valid(RAFZ_BITRAF42_MASK),
        1u);
    failures += check_s32(
        rafz_pure_bitraf42_validate(overflow),
        -1);
    failures += check_u64(
        rafz_pure_bitraf42_canonicalize(overflow),
        0x12345u);

    return failures;
}
