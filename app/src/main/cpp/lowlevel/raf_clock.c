#include "raf_clock.h"
#include <time.h>

void raf_clock_init(raf_clock_t* c, uint32_t target_hz) {
    if (!c) return;
    if (target_hz == 0) target_hz = 1;
    c->target_hz = target_hz;
    c->period_ns = 1000000000ULL / (uint64_t)target_hz;
    c->last_tick_ns = 0;
    c->actual_delta_ns = 0;
    c->jitter_ns = 0;
    c->missed_ticks = 0;
    c->total_ticks = 0;
}

uint64_t raf_clock_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int raf_clock_should_tick(const raf_clock_t* c, uint64_t now_ns) {
    if (!c || c->period_ns == 0) return 0;
    if (c->last_tick_ns == 0) return 1;
    return (now_ns - c->last_tick_ns) >= c->period_ns;
}

void raf_clock_mark_tick(raf_clock_t* c, uint64_t now_ns) {
    if (!c) return;
    if (c->last_tick_ns != 0) {
        c->actual_delta_ns = now_ns - c->last_tick_ns;
        uint64_t expected = c->period_ns;
        c->jitter_ns = (c->actual_delta_ns > expected) ? (c->actual_delta_ns - expected) : (expected - c->actual_delta_ns);
        if (expected && c->actual_delta_ns > expected * 2ULL) c->missed_ticks++;
    }
    c->last_tick_ns = now_ns;
    c->total_ticks++;
}

uint32_t raf_clock_actual_hz_q16(const raf_clock_t* c) {
    if (!c || c->actual_delta_ns == 0) return 0;
    uint64_t hz_q16 = (1000000000ULL << 16) / c->actual_delta_ns;
    return (uint32_t)hz_q16;
}

uint64_t raf_clock_get_jitter_ns(const raf_clock_t* c) { return c ? c->jitter_ns : 0; }
