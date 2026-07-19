#include "raf_ecc32_masked.h"

#include <stdint.h>
#include <stdio.h>

static uint8_t raf_ecc32_reference(uint32_t v) {
    uint8_t ecc = 0u;
    for (uint8_t bit = 0u; bit < 6u; ++bit) {
        uint32_t parity = 0u;
        for (uint8_t i = 0u; i < 32u; ++i) {
            uint32_t position = (uint32_t)i + 1u;
            if ((position & (1u << bit)) != 0u) {
                parity ^= (v >> i) & 1u;
            }
        }
        ecc |= (uint8_t)((parity & 1u) << bit);
    }
    return ecc;
}

static int expect_equal(uint32_t value) {
    uint8_t reference = raf_ecc32_reference(value);
    uint8_t masked = raf_ecc32_masked(value);
    if (reference == masked) return 0;
    fprintf(
        stderr,
        "ecc32 mismatch value=0x%08x reference=%u masked=%u\n",
        value,
        (unsigned int)reference,
        (unsigned int)masked
    );
    return 1;
}

int main(void) {
    static const uint32_t fixed[] = {
        0x00000000u,
        0x00000001u,
        0x00000002u,
        0x00000003u,
        0xFFFFFFFFu,
        0x80000000u,
        0x55555555u,
        0xAAAAAAAAu,
        0x12345678u
    };
    int failed = 0;

    /*
     * Both transforms are linear over GF(2). Equality on the complete
     * 32-element standard basis therefore proves equality for all 2^32 words.
     */
    for (uint32_t bit = 0u; bit < 32u; ++bit) {
        failed += expect_equal(1u << bit);
    }

    for (uint32_t i = 0u; i < (uint32_t)(sizeof(fixed) / sizeof(fixed[0])); ++i) {
        failed += expect_equal(fixed[i]);
    }

    /* Additional deterministic implementation guard: one million states. */
    uint32_t state = 0x12345678u;
    uint32_t checksum = 0u;
    for (uint32_t i = 0u; i < 1000000u; ++i) {
        state = state * 1664525u + 1013904223u;
        failed += expect_equal(state);
        checksum = (checksum << 5u) ^ (checksum >> 2u) ^ raf_ecc32_masked(state);
        if (failed != 0) break;
    }

    if (failed != 0) return 1;
    printf("raf_ecc32 masked equivalence: PASS checksum=0x%08x\n", checksum);
    return 0;
}
