#include "rafaelia_gpu_orchestrator.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stddef.h>

#define RGO_MAX_CORES 8u
#define Q16_ONE 65536u

static const uint32_t g_core_ratio_q16[RGO_MAX_CORES] = {
    65536u, 56755u, 49152u, 42598u, 36864u, 31949u, 27648u, 23961u
};

static uint32_t g_core_load_q16[RGO_MAX_CORES];

static uint32_t g_crc32_tbl[256];
static pthread_once_t g_crc32_once = PTHREAD_ONCE_INIT;

static void crc32_init_table(void) {
    uint32_t i = 0;
    for (i = 0; i < 256u; ++i) {
        uint32_t c = i;
        uint32_t j = 0;
        for (j = 0; j < 8u; ++j) {
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
        }
        g_crc32_tbl[i] = c;
    }
}

static uint32_t gcd_u32(uint32_t a, uint32_t b) {
    while (b != 0u) {
        uint32_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

int rgpu_probe_opencl(void) {
    void* h = dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    if (!h) h = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!h) return RGO_ERR_DLOPEN;

    if (!dlsym(h, "clGetPlatformIDs") || !dlsym(h, "clGetDeviceIDs")) {
        dlclose(h);
        return RGO_ERR_DLSYM;
    }

    dlclose(h);
    return RGO_OK;
}

int rgpu_probe_vulkan(void) {
    void* h = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!h) h = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!h) return RGO_ERR_DLOPEN;

    if (!dlsym(h, "vkGetInstanceProcAddr")) {
        dlclose(h);
        return RGO_ERR_DLSYM;
    }

    dlclose(h);
    return RGO_OK;
}

void rcpu_map_toroidal(uint32_t cpu_count, uint32_t* zones, uint32_t zone_cap) {
    uint32_t r = 3u;
    uint32_t c = 3u;
    uint32_t dr = 1u;
    uint32_t dc = 2u;
    uint32_t i;
    if (!zones || zone_cap == 0u) return;
    if (cpu_count > zone_cap) cpu_count = zone_cap;

    if (gcd_u32(dr, r) != 1u) dr = 1u;
    if (gcd_u32(dc, c) != 1u) dc = 1u;

    for (i = 0u; i < cpu_count; ++i) {
        uint32_t rr = (i * dr) % r;
        uint32_t cc = (i * dc) % c;
        zones[i] = rr * c + cc;
    }
}

uint32_t rcrc32_sw(const uint8_t* data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    if (!data) return 0u;
    (void)pthread_once(&g_crc32_once, crc32_init_table);

    for (i = 0u; i < len; ++i) {
        uint32_t idx = (crc ^ (uint32_t)data[i]) & 0xFFu;
        crc = g_crc32_tbl[idx] ^ (crc >> 8u);
    }
    return ~crc;
}

uint32_t rscheduler_pick_core(uint32_t task_hz_q16) {
    uint32_t i;
    uint32_t best_i = 0u;
    uint32_t best_err = 0xFFFFFFFFu;
    uint32_t best_load = 0xFFFFFFFFu;

    for (i = 0u; i < RGO_MAX_CORES; ++i) {
        uint32_t target = (uint32_t)(((uint64_t)Q16_ONE * (uint64_t)g_core_ratio_q16[i]) >> 16u);
        uint32_t err = (task_hz_q16 > target) ? (task_hz_q16 - target) : (target - task_hz_q16);
        uint32_t load = g_core_load_q16[i];

        if (err < best_err || (err == best_err && load < best_load)) {
            best_err = err;
            best_load = load;
            best_i = i;
        }
    }
    return best_i;
}

void rscheduler_set_load(uint32_t core_idx, uint32_t load_q16) {
    if (core_idx >= RGO_MAX_CORES) return;
    g_core_load_q16[core_idx] = load_q16;
}

void rscheduler_reset(void) {
    uint32_t i;
    for (i = 0u; i < RGO_MAX_CORES; ++i) g_core_load_q16[i] = 0u;
}
