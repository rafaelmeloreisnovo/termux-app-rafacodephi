/* Hosted test harness for freestanding raf_im_ecc_v1.h.
 * Build: cc -std=c99 -O2 -Wall -Wextra -Werror -pedantic
 *        tests/internetmount/test_raf_im_ecc_v1.c -o /tmp/raf-im-ecc-test
 *        && /tmp/raf-im-ecc-test
 * NOT evidence of Android device execution. No cloud data involved.
 */
#include <stdio.h>
#include "../../native/internetmount/raf_im_ecc_v1.h"
#define N RAF_IM_MAX_CHUNK
static uint8_t d0[N], d1[N], parity[N], recovered[N];
static uint32_t expected[3];
static unsigned failures;
#define CHECK(expr) do { if (!(expr)) { ++failures; \
    printf("FAIL line %d: %s\n", __LINE__, #expr); } } while (0)
int main(void) {
    uint32_t i;
    for (i = 0; i < N; ++i) {
        d0[i] = (uint8_t)(i * 13u + 11u);
        d1[i] = (uint8_t)(i * 7u + 29u);
    }
    CHECK(raf_im_crc32((const uint8_t*)"123456789", 9u) == 0xcbf43926u);
    CHECK(raf_im_xor_parity(d0, d1, parity, N) == RAF_IM_INTACT);
    expected[0] = raf_im_crc32(d0, N);
    expected[1] = raf_im_crc32(d1, N);
    expected[2] = raf_im_crc32(parity, N);
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,N) == RAF_IM_INTACT);

    d0[17] ^= 0x80u;
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,N) == RAF_IM_RECOVER_DATA0);
    CHECK(recovered[17] == (uint8_t)(d0[17] ^ 0x80u));
    CHECK(raf_im_crc32(recovered, N) == expected[0]);
    d0[17] ^= 0x80u;

    d1[77] ^= 0x01u;
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,N) == RAF_IM_RECOVER_DATA1);
    CHECK(raf_im_crc32(recovered, N) == expected[1]);
    d1[77] ^= 0x01u;

    parity[200] ^= 0x40u;
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,N) == RAF_IM_RECOVER_PARITY);
    CHECK(raf_im_crc32(recovered, N) == expected[2]);
    parity[200] ^= 0x40u;

    d0[2] ^= 1u;
    d1[3] ^= 1u;
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,N) == RAF_IM_UNCORRECTABLE);
    d0[2] ^= 1u;
    d1[3] ^= 1u;
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,0u) == RAF_IM_INVALID);
    CHECK(raf_im_recover_one(d0,d1,parity,expected,recovered,N+1u) == RAF_IM_INVALID);
    CHECK(raf_im_xor_parity(d0,d1,d0,N) == RAF_IM_INVALID);
    CHECK(raf_im_recover_one(d0,d1,parity,expected,d0,N) == RAF_IM_INVALID);
    if (failures) { printf("FAIL %u checks\n", failures); return 1; }
    puts("PASS_SCOPED_HOSTED_ECC_TESTS_ONLY");
    return 0;
}
