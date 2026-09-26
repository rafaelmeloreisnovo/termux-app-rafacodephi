#include "../rafaelia/src/main/cpp/zero/include/rafz.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_primitives.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_q16.h"

RAFZ_EXPORT rafz_u32 rafz_pure_core_probe(rafz_u32 x) {
    const rafz_u32 a = (rafz_u32)rafz_pure_bagua_rol3((rafz_u8)x);
    const rafz_u32 b = (rafz_u32)rafz_pure_bagua_ror3((rafz_u8)(x >> 3u));
    const rafz_u32 c = (rafz_u32)rafz_pure_q16_step((rafz_s32)x);
    const rafz_u32 d = rafz_pure_q16_sub_sat(
        rafz_pure_q16_clamp01(x),
        rafz_pure_q16_from_u8_frac((rafz_u8)x));
    const rafz_u32 e = rafz_pure_q16_phi(
        rafz_pure_q16_clamp01(x),
        RAFZ_Q16_ONE >> 1u);
    const rafz_u32 m = 0u - (x & 1u);
    return rafz_pure_select_u32(m, a ^ c ^ d, b ^ c ^ e);
}
