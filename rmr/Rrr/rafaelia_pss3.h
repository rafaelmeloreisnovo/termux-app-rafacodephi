#ifndef RAFAELIA_PSS3_H
#define RAFAELIA_PSS3_H
#include <stdint.h>
#define PSS3_DELTA_Q16 11796u

typedef struct {
    uint32_t recurrence_q16;
    uint32_t severity_q16;
    uint32_t stability_q16;
    uint32_t delta_q16;
    uint32_t gate;
    uint32_t token_invariant_q16;
    uint32_t crc32c;
} rafaelia_pss3_state;

void rafaelia_pss3_init(rafaelia_pss3_state *s);
void rafaelia_pss3_step(rafaelia_pss3_state *s, uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16, uint32_t gate);
void rafaelia_pss3_step_asm(rafaelia_pss3_state *s, uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16, uint32_t gate);
uint32_t rafaelia_pss3_delta(const rafaelia_pss3_state *s);
uint32_t rafaelia_pss3_delta_q16_asm(uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16);
uint32_t rafaelia_pss3_token_invariant_q16_asm(uint32_t delta_q16);
uint32_t rafaelia_pss3_token_invariant(const rafaelia_pss3_state *s);
int rafaelia_pss3_is_stable(const rafaelia_pss3_state *s, int beta_mode);
uint32_t rafaelia_pss3_crc(const rafaelia_pss3_state *s);
#endif
