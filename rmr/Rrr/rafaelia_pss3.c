#include "rafaelia_pss3.h"

static uint32_t mix(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }

uint32_t rafaelia_pss3_crc(const rafaelia_pss3_state *s) {
    uint32_t c = 0x1EDC6F41u;
    c ^= mix(s->recurrence_q16); c ^= mix(s->severity_q16); c ^= mix(s->stability_q16); c ^= mix(s->delta_q16); c ^= mix(s->gate);
    return c;
}

void rafaelia_pss3_init(rafaelia_pss3_state *s) {
    s->recurrence_q16 = 0; s->severity_q16 = 0; s->stability_q16 = 0; s->delta_q16 = 0; s->gate = 0;
    s->crc32c = rafaelia_pss3_crc(s);
}

void rafaelia_pss3_step(rafaelia_pss3_state *s, uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16, uint32_t gate) {
    if (stability_q16 > 65535u) stability_q16 = 65535u;
    s->recurrence_q16 = recurrence_q16;
    s->severity_q16 = severity_q16;
    s->stability_q16 = stability_q16;
    s->gate = gate;

    uint64_t risk = ((uint64_t)recurrence_q16 * (uint64_t)severity_q16) >> 16;
    uint32_t unstability = 65535u - stability_q16;
    s->delta_q16 = (uint32_t)((risk * unstability) >> 16);
    s->crc32c = rafaelia_pss3_crc(s);
}

uint32_t rafaelia_pss3_delta(const rafaelia_pss3_state *s) { return s->delta_q16; }

int rafaelia_pss3_is_stable(const rafaelia_pss3_state *s, int beta_mode) {
    int base = s->delta_q16 <= PSS3_DELTA_Q16;
    if (!beta_mode) return base;
    return base && s->gate != 3u && s->crc32c == rafaelia_pss3_crc(s);
}
