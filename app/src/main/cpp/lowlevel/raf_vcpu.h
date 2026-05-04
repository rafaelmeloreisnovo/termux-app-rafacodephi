#ifndef RAF_VCPU_H
#define RAF_VCPU_H
#include <stdint.h>
#include <stddef.h>

#define RAF_STATE_DIM 7
#define RAF_PERIOD 42
#define RAF_VCPU 8

typedef struct __attribute__((packed)) {
    uint32_t s[RAF_STATE_DIM];
    uint32_t coherence;
    uint32_t entropy;
    uint32_t phase;
    uint32_t step;
    uint32_t crc;
} raf_state_t;

typedef struct __attribute__((aligned(64))) {
    raf_state_t state;
    uint32_t id;
    uint32_t target_hz;
    uint32_t actual_hz_q16;
    uint64_t ticks;
    uint64_t last_ns;
    uint64_t jitter_ns;
    uint32_t flags;
    uint32_t crc;
} raf_vcpu_t;

void raf_vcpu_init(uint32_t target_hz);
void raf_vcpu_reset(uint32_t vcpu_id);
int raf_vcpu_step(uint32_t vcpu_id, uint64_t now_ns);
uint32_t raf_vcpu_crc(const raf_vcpu_t* v);
int raf_vcpu_validate(uint32_t vcpu_id);
const raf_state_t* raf_vcpu_get_state(uint32_t vcpu_id);
const raf_vcpu_t* raf_vcpu_get(uint32_t vcpu_id);
size_t raf_vcpu_count(void);

#endif
