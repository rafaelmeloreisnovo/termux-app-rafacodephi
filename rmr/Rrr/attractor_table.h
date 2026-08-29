/**
 * attractor_table.h — 41-state toroid attractor phase space (header)
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#ifndef ATTRACTOR_TABLE_H
#define ATTRACTOR_TABLE_H

#include <stdint.h>

/**
 * Attractor lookup: O(1) deterministic access to 41-state phase space
 * @param idx Phase index [0..40]
 * @return Q16.16 coherence value, or 0 on out-of-bounds
 */
uint32_t attractor_lookup(uint32_t idx);

/**
 * Attractor validation: verify coprimality and bounds
 * @return 0 on success, negative on validation failure
 */
int attractor_validate(void);

/**
 * Attractor table statistics
 */
void attractor_stats(uint32_t *out_min, uint32_t *out_max, uint32_t *out_avg);

/**
 * Attractor metadata
 */
struct attractor_metadata {
    uint32_t count;      /* 41 */
    uint32_t period;     /* 41 (prime) */
    uint32_t dim;        /* 7 (toroid dimension) */
    uint32_t sha256[8];  /* SHA-256 hash */
};

const struct attractor_metadata* attractor_get_metadata(void);

#endif /* ATTRACTOR_TABLE_H */
