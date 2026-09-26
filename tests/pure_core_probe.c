#include "../rafaelia/src/main/cpp/zero/include/rafz.h"
#include "../rafaelia/src/main/cpp/zero/include/rafz_pure_primitives.h"

RAFZ_EXPORT rafz_u32 rafz_pure_core_probe(rafz_u32 x) {
    const rafz_u32 a = (rafz_u32)rafz_pure_bagua_rol3((rafz_u8)x);
    const rafz_u32 b = (rafz_u32)rafz_pure_bagua_ror3((rafz_u8)(x >> 3u));
    const rafz_u32 c = (rafz_u32)rafz_pure_q16_step((rafz_s32)x);
    const rafz_u32 m = 0u - (x & 1u);
    return rafz_pure_select_u32(m, a ^ c, b ^ c);
}
