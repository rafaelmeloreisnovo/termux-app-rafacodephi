#include "rafaelia_gpu_orchestrator.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

#define Q16_ONE 0x10000u
#define RGO_CRC32_POLY 0xEDB88320u

#define RGO_ARCH_ARM32  (1u << 0)
#define RGO_ARCH_ARM64  (1u << 1)
#define RGO_ARCH_X86_64 (1u << 2)
#define RGO_ARCH_X86    (1u << 3)

typedef int (*clGetPlatformIDs_fn)(uint32_t, void*, uint32_t*);

static const uint32_t g_core_freq_q16[MAX_CORES] = {
    78643u, 78643u, 78643u, 78643u,
    78643u, 78643u, 78643u, 78643u,
    78643u, 78643u, 78643u, 78643u,
    78643u, 78643u, 78643u, 78643u
};

static atomic_uint g_core_load[MAX_CORES];
static rgpu_state_t g_gpu_state = GPU_UNKNOWN;
static void* g_opencl_handle = 0;
static pthread_once_t g_gpu_probe_once = PTHREAD_ONCE_INIT;

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

static uint32_t rgo_absdiff_u32(uint32_t a, uint32_t b) {
    return (a > b) ? (a - b) : (b - a);
}

static void crc32_init_table(void) {
    uint32_t i;
    for (i = 0; i < 256u; ++i) {
        uint32_t c = i;
        uint32_t j;
        for (j = 0; j < 8u; ++j) {
            c = (c & 1u) ? (RGO_CRC32_POLY ^ (c >> 1u)) : (c >> 1u);
        }
        g_crc32_tbl[i] = c;
    }
}

static void rgpu_probe_opencl_internal(void) {
    const char* libs[] = { "libOpenCL.so", "libOpenCL.so.1" };
    int i;
    for (i = 0; i < 2; ++i) {
        void* h = dlopen(libs[i], RTLD_NOW | RTLD_LOCAL);
        if (!h) continue;

        clGetPlatformIDs_fn fn = (clGetPlatformIDs_fn)dlsym(h, "clGetPlatformIDs");
        if (!fn) {
            dlclose(h);
            continue;
        }

        {
            uint32_t platforms = 0u;
            int ret = fn(0u, 0, &platforms);
            if (ret == 0 && platforms > 0u) {
                g_opencl_handle = h;
                g_gpu_state = GPU_PRESENT;
                return;
            }
            g_gpu_state = GPU_FAIL_RUNTIME;
        }
        dlclose(h);
    }

    if (g_gpu_state == GPU_UNKNOWN) g_gpu_state = GPU_NO_DRIVER;
}

int rgpu_probe_opencl(void) {
    pthread_once(&g_gpu_probe_once, rgpu_probe_opencl_internal);
    return (g_gpu_state == GPU_PRESENT) ? RGO_OK : RGO_ERR_GPU_RUNTIME;
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

rgpu_state_t rgpu_get_state(void) {
    return g_gpu_state;
}

uint32_t rgpu_get_core_count(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n <= 0) return 4u;
    if (n > (long)MAX_CORES) return MAX_CORES;
    return (uint32_t)n;
}

void rcpu_map_toroidal(uint32_t* zones, uint32_t n) {
    uint32_t i;
    uint32_t cores = rgpu_get_core_count();
    if (!zones || n == 0u) return;

    for (i = 0u; i < n; ++i) {
        zones[i] = (i * 3u + 1u) % cores;
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

uint32_t rscheduler_pick_core(uint32_t task_hz_q16, uint32_t intensity) {
    uint32_t i;
    uint32_t cores = rgpu_get_core_count();
    uint32_t best = 0u;
    uint32_t best_cost = 0xFFFFFFFFu;

    for (i = 0u; i < cores; ++i) {
        uint32_t load = atomic_load_explicit(&g_core_load[i], memory_order_relaxed);
        uint32_t freq_error = rgo_absdiff_u32(task_hz_q16, g_core_freq_q16[i]);
        uint32_t cost = (freq_error >> 10u) + (load * 3u) + (intensity * 2u);

        if (cost < best_cost) {
            best_cost = cost;
            best = i;
        }
    }

    atomic_fetch_add_explicit(&g_core_load[best], 1u, memory_order_relaxed);
    rgo_mem_barrier();
    return best;
}

void rscheduler_set_load(uint32_t core_idx, uint32_t load_q16) {
    if (core_idx >= MAX_CORES) return;
    atomic_store_explicit(&g_core_load[core_idx], load_q16, memory_order_relaxed);
    rgo_mem_barrier();
}

void rscheduler_reset(void) {
    uint32_t i;
    for (i = 0u; i < MAX_CORES; ++i) {
        atomic_store_explicit(&g_core_load[i], 0u, memory_order_relaxed);
    }
    rgo_mem_barrier();
}

uint32_t rgpu_runtime_caps(void) {
    uint32_t caps = rgo_arch_mask();
    if (rgpu_probe_opencl() == RGO_OK) caps |= RGO_CAP_OPENCL;
    if (rgpu_probe_vulkan() == RGO_OK) caps |= RGO_CAP_VULKAN;
    return caps;
}
