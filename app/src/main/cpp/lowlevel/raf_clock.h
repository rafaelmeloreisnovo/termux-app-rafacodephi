#ifndef RAF_CLOCK_H
#define RAF_CLOCK_H
#include <stdint.h>

typedef struct {
    uint32_t target_hz;
    uint64_t period_ns;
    uint64_t last_tick_ns;
    uint64_t actual_delta_ns;
    uint64_t jitter_ns;
    uint64_t missed_ticks;
    uint64_t total_ticks;
} raf_clock_t;

void raf_clock_init(raf_clock_t* c, uint32_t target_hz);
uint64_t raf_clock_now_ns(void);
int raf_clock_should_tick(const raf_clock_t* c, uint64_t now_ns);
void raf_clock_mark_tick(raf_clock_t* c, uint64_t now_ns);
uint32_t raf_clock_actual_hz_q16(const raf_clock_t* c);
uint64_t raf_clock_get_jitter_ns(const raf_clock_t* c);

#endif
