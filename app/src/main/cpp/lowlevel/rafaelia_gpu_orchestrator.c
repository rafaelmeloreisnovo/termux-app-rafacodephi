#include "rafaelia_gpu_orchestrator.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stddef.h>

#define RGO_MAX_CORES 8u
#define Q16_ONE 65536u
#define RGO_CRC32_POLY 0xEDB88320u

#define RGO_ARCH_ARM32  (1u << 0)
#define RGO_ARCH_ARM64  (1u << 1)
#define RGO_ARCH_X86_64 (1u << 2)
#define RGO_ARCH_X86    (1u << 3)

static const uint32_t g_core_freq_q16[RGO_MAX_CORES] = {
    78643u, 78643u, 78643u, 78643u, 78643u, 78643u, 78643u, 78643u
};

static uint32_t g_core_load_q16[RGO_MAX_CORES];

static uint32_t g_crc32_tbl[256];
static pthread_once_t g_crc32_once = PTHREAD_ONCE_INIT;

static void rgo_mem_barrier(void) {
#if defined(__aarch64__)
    __asm__ volatile("dmb ish" ::: "memory");
#elif defined(__arm__)
    __asm__ volatile("dmb" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("mfence" ::: "memory");
#else
    __sync_synchronize();
#endif
}

static uint32_t rgo_arch_mask(void) {
#if defined(__aarch64__)
    return RGO_ARCH_ARM64;
#elif defined(__arm__)
    return RGO_ARCH_ARM32;
#elif defined(__x86_64__)
    return RGO_ARCH_X86_64;
#elif defined(__i386__)
    return RGO_ARCH_X86;
#else
    return 0u;
#endif
}

static void crc32_init_table(void) {
    uint32_t i = 0;
    for (i = 0; i < 256u; ++i) {
        uint32_t c = i;
        uint32_t j = 0;
        for (j = 0; j < 8u; ++j) {
            c = (c & 1u) ? (RGO_CRC32_POLY ^ (c >> 1u)) : (c >> 1u);
        }
        g_crc32_tbl[i] = c;
    }
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
    uint32_t i;
    const uint32_t r = 4u;
    const uint32_t c = 2u;
    const uint32_t dr = 1u;
    const uint32_t dc = 1u;

    if (!zones || zone_cap == 0u) return;
    if (cpu_count > zone_cap) cpu_count = zone_cap;

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
    return crc ^ 0xFFFFFFFFu;
}

uint32_t rscheduler_pick_core(uint32_t task_hz_q16) {
    uint32_t i;
    uint32_t best_i = 0u;
    uint32_t best_err = 0xFFFFFFFFu;
    uint32_t best_load = 0xFFFFFFFFu;

    for (i = 0u; i < RGO_MAX_CORES; ++i) {
        uint32_t target = g_core_freq_q16[i];
        uint32_t err = (task_hz_q16 > target) ? (task_hz_q16 - target) : (target - task_hz_q16);
        uint32_t load = g_core_load_q16[i];

        if (err < best_err || (err == best_err && load < best_load)) {
            best_err = err;
            best_load = load;
            best_i = i;
        }
    }

    (void)__atomic_fetch_add(&g_core_load_q16[best_i], 1u, __ATOMIC_RELAXED);
    rgo_mem_barrier();
    return best_i;
}

void rscheduler_set_load(uint32_t core_idx, uint32_t load_q16) {
    if (core_idx >= RGO_MAX_CORES) return;
    g_core_load_q16[core_idx] = load_q16;
    rgo_mem_barrier();
}

void rscheduler_reset(void) {
    uint32_t i;
    for (i = 0u; i < RGO_MAX_CORES; ++i) g_core_load_q16[i] = 0u;
    rgo_mem_barrier();
}

uint32_t rgpu_runtime_caps(void) {
    uint32_t caps = rgo_arch_mask();
    if (rgpu_probe_opencl() == RGO_OK) caps |= RGO_CAP_OPENCL;
    if (rgpu_probe_vulkan() == RGO_OK) caps |= RGO_CAP_VULKAN;
    return caps;
}
