#include "raf_vcpu.h"

static raf_vcpu_t g_vcpu[RAF_VCPU];

static uint32_t crc32_simple(const void* b, size_t n) {
    const uint8_t* p = (const uint8_t*)b;
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int i = 0; i < 8; i++) c = (c >> 1) ^ (0x82F63B78u & (uint32_t)(-(int32_t)(c & 1u)));
    }
    return ~c;
}

uint32_t raf_vcpu_crc(const raf_vcpu_t* v) {
    if (!v) return 0;
    raf_vcpu_t tmp = *v;
    tmp.crc = 0;
    tmp.state.crc = 0;
    return crc32_simple(&tmp, sizeof(tmp));
}

void raf_vcpu_reset(uint32_t id) {
    if (id >= RAF_VCPU) return;
    raf_vcpu_t* v = &g_vcpu[id];
    v->id = id;
    for (int i = 0; i < RAF_STATE_DIM; i++) v->state.s[i] = (uint32_t)((id + 1u) * (uint32_t)(i + 3) * 733u) & 0xFFFFu;
    v->state.coherence = 0x8000u;
    v->state.entropy = 0x8000u;
    v->state.phase = 0;
    v->state.step = 0;
    v->ticks = 0;
    v->last_ns = 0;
    v->jitter_ns = 0;
    v->flags = 0;
    v->actual_hz_q16 = 0;
    v->state.crc = crc32_simple(&v->state, sizeof(v->state) - sizeof(uint32_t));
    v->crc = raf_vcpu_crc(v);
}

void raf_vcpu_init(uint32_t target_hz) {
    for (uint32_t i = 0; i < RAF_VCPU; i++) {
        g_vcpu[i].target_hz = target_hz;
        raf_vcpu_reset(i);
    }
}

int raf_vcpu_step(uint32_t id, uint64_t now_ns) {
    if (id >= RAF_VCPU) return -1;
    raf_vcpu_t* v = &g_vcpu[id];
    if (raf_vcpu_validate(id) != 0) raf_vcpu_reset(id);
    uint32_t cin = (uint32_t)(((id + 1u) * 56755u + (uint32_t)v->ticks) & 0xFFFFu);
    uint32_t hin = 65535u - cin;
    v->state.coherence = (uint32_t)(((uint64_t)v->state.coherence * 49152u + (uint64_t)cin * 16384u) >> 16);
    v->state.entropy = (uint32_t)(((uint64_t)v->state.entropy * 49152u + (uint64_t)hin * 16384u) >> 16);
    v->state.phase = (v->state.phase + 1u) % RAF_PERIOD;
    v->state.step++;
    v->ticks++;
    v->last_ns = now_ns;
    v->state.crc = crc32_simple(&v->state, sizeof(v->state) - sizeof(uint32_t));
    v->crc = raf_vcpu_crc(v);
    return 0;
}

int raf_vcpu_validate(uint32_t id) {
    if (id >= RAF_VCPU) return -1;
    raf_vcpu_t* v = &g_vcpu[id];
    uint32_t scrc = crc32_simple(&v->state, sizeof(v->state) - sizeof(uint32_t));
    if (scrc != v->state.crc) return -2;
    if (raf_vcpu_crc(v) != v->crc) return -3;
    return 0;
}

const raf_state_t* raf_vcpu_get_state(uint32_t id) { return (id < RAF_VCPU) ? &g_vcpu[id].state : 0; }
const raf_vcpu_t* raf_vcpu_get(uint32_t id) { return (id < RAF_VCPU) ? &g_vcpu[id] : 0; }
size_t raf_vcpu_count(void) { return RAF_VCPU; }
