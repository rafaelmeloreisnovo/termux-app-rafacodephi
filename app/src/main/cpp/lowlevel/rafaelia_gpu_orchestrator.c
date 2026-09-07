#define _POSIX_C_SOURCE 200809L
#include "rafaelia_gpu_orchestrator.h"

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <dlfcn.h>
#include <unistd.h>

#define Q16_ONE 0x10000u
#define RGO_CRC32_POLY 0xEDB88320u

#define RGO_ARCH_ARM32  (1u << 0)
#define RGO_ARCH_ARM64  (1u << 1)
#define RGO_ARCH_X86_64 (1u << 2)
#define RGO_ARCH_X86    (1u << 3)

#define RGO_CL_DEVICE_TYPE_GPU (1ull << 2)
#define RGO_VK_STRUCTURE_TYPE_APPLICATION_INFO 0u
#define RGO_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO 1u
#define RGO_VK_QUEUE_COMPUTE_BIT 0x00000002u
#define RGO_VK_API_VERSION_1_0 (1u << 22)
#define RGO_MAX_GPU_HANDLES 8u

typedef int32_t (*rgo_clGetPlatformIDs_fn)(uint32_t, void**, uint32_t*);
typedef int32_t (*rgo_clGetDeviceIDs_fn)(void*, uint64_t, uint32_t, void**, uint32_t*);

typedef void* rgo_VkInstance;
typedef void* rgo_VkPhysicalDevice;
typedef int32_t rgo_VkResult;
typedef struct {
    uint32_t sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
} rgo_VkApplicationInfo;
typedef struct {
    uint32_t sType;
    const void* pNext;
    uint32_t flags;
    const rgo_VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
} rgo_VkInstanceCreateInfo;
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} rgo_VkExtent3D;
typedef struct {
    uint32_t queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    rgo_VkExtent3D minImageTransferGranularity;
} rgo_VkQueueFamilyProperties;
typedef rgo_VkResult (*rgo_vkCreateInstance_fn)(const rgo_VkInstanceCreateInfo*, const void*, rgo_VkInstance*);
typedef void (*rgo_vkDestroyInstance_fn)(rgo_VkInstance, const void*);
typedef rgo_VkResult (*rgo_vkEnumeratePhysicalDevices_fn)(rgo_VkInstance, uint32_t*, rgo_VkPhysicalDevice*);
typedef void (*rgo_vkGetPhysicalDeviceQueueFamilyProperties_fn)(rgo_VkPhysicalDevice, uint32_t*, rgo_VkQueueFamilyProperties*);

typedef struct {
    rtask_t buffer[WSQ_SIZE];
    atomic_uint head;
    atomic_uint tail;
} wsq_t;

static const char* const g_opencl_paths[] = {
    "/vendor/lib/libOpenCL.so",
    "/vendor/lib/libOpenCL.so.1",
    "/vendor/lib/libPVROCL.so",
    "/system/lib/libOpenCL.so",
    "/system/vendor/lib/libOpenCL.so",
    "/vendor/lib64/libOpenCL.so",
    "/vendor/lib64/libOpenCL.so.1",
    "/vendor/lib64/libPVROCL.so",
    "/system/lib64/libOpenCL.so",
    "libOpenCL.so.1",
    "libOpenCL.so",
    NULL
};

static const char* const g_vulkan_paths[] = {
    "/vendor/lib/libvulkan.so",
    "/system/lib/libvulkan.so",
    "/vendor/lib64/libvulkan.so",
    "/system/lib64/libvulkan.so",
    "libvulkan.so.1",
    "libvulkan.so",
    NULL
};

static const uint32_t g_core_freq_q16[MAX_CORES] = {
    78643u, 78643u, 78643u, 78643u, 78643u, 78643u, 78643u, 78643u,
    78643u, 78643u, 78643u, 78643u, 78643u, 78643u, 78643u, 78643u
};

static atomic_uint g_core_load[MAX_CORES];
static atomic_uint g_thermal_state;
static atomic_uint g_runtime_caps;
static atomic_uint g_qualified_caps;
static atomic_uint g_route_count_cpu;
static atomic_uint g_route_count_gpu;
static atomic_uint g_last_route_reason;
static atomic_int g_gpu_state = ATOMIC_VAR_INIT(GPU_UNKNOWN);
static atomic_flag g_gpu_probe_lock = ATOMIC_FLAG_INIT;
static atomic_flag g_policy_lock = ATOMIC_FLAG_INIT;
static atomic_int g_probe_done = ATOMIC_VAR_INIT(0);
static wsq_t g_wsq;

static rgpu_policy_t g_policy = {
    RGO_DEFAULT_MIN_SAMPLES,
    RGO_DEFAULT_MIN_WORK_BYTES,
    RGO_DEFAULT_MAX_THERMAL,
    RGO_DEFAULT_MIN_GAIN_PERMILLE
};
static rgpu_measurement_t g_measurement[3];
static rgpu_backend_t g_qualified_backend = RGO_GPU_BACKEND_NONE;

static uint32_t g_crc32_tbl[256];
static atomic_int g_crc32_initialized = ATOMIC_VAR_INIT(0);
static atomic_flag g_crc_lock = ATOMIC_FLAG_INIT;

static void rgo_lock(atomic_flag* lock) {
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) { }
}

static void rgo_unlock(atomic_flag* lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}

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

static uint64_t remk_now_ns(void) {
    static atomic_ullong g_clock_counter = ATOMIC_VAR_INIT(0ull);
    return atomic_fetch_add_explicit(&g_clock_counter, 1ull, memory_order_relaxed);
}

static uint32_t rgo_absdiff_u32(uint32_t a, uint32_t b) {
    return (a > b) ? (a - b) : (b - a);
}

static uint32_t remk_thermal_penalty(void) {
    return atomic_load_explicit(&g_thermal_state, memory_order_relaxed) / 10u;
}

static uint64_t remk_cost_fn(uint64_t latency, uint32_t load, uint32_t intensity, uint32_t thermal_penalty) {
    return (latency >> 8u) + ((uint64_t)load * 40u) +
           ((uint64_t)intensity * 25u) + ((uint64_t)thermal_penalty * 60u);
}

static uint32_t rgo_backend_cap(rgpu_backend_t backend) {
    if (backend == RGO_GPU_BACKEND_OPENCL) return RGO_CAP_OPENCL;
    if (backend == RGO_GPU_BACKEND_VULKAN) return RGO_CAP_VULKAN;
    return 0u;
}

static int rgo_probe_opencl_once(void) {
    uint32_t path_i;
    for (path_i = 0u; g_opencl_paths[path_i] != NULL; ++path_i) {
        void* lib = dlopen(g_opencl_paths[path_i], RTLD_NOW | RTLD_LOCAL);
        if (lib == NULL) continue;

        rgo_clGetPlatformIDs_fn get_platforms =
            (rgo_clGetPlatformIDs_fn)dlsym(lib, "clGetPlatformIDs");
        rgo_clGetDeviceIDs_fn get_devices =
            (rgo_clGetDeviceIDs_fn)dlsym(lib, "clGetDeviceIDs");
        if (get_platforms != NULL && get_devices != NULL) {
            void* platforms[RGO_MAX_GPU_HANDLES];
            uint32_t platform_count = 0u;
            if (get_platforms(RGO_MAX_GPU_HANDLES, platforms, &platform_count) == 0 && platform_count > 0u) {
                uint32_t i;
                if (platform_count > RGO_MAX_GPU_HANDLES) platform_count = RGO_MAX_GPU_HANDLES;
                for (i = 0u; i < platform_count; ++i) {
                    uint32_t gpu_count = 0u;
                    if (get_devices(platforms[i], RGO_CL_DEVICE_TYPE_GPU, 0u, NULL, &gpu_count) == 0 && gpu_count > 0u) {
                        dlclose(lib);
                        return RGO_OK;
                    }
                }
            }
        }
        dlclose(lib);
    }
    return RGO_ERR_GPU_RUNTIME;
}

static int rgo_probe_vulkan_once(void) {
    uint32_t path_i;
    for (path_i = 0u; g_vulkan_paths[path_i] != NULL; ++path_i) {
        void* lib = dlopen(g_vulkan_paths[path_i], RTLD_NOW | RTLD_LOCAL);
        if (lib == NULL) continue;

        rgo_vkCreateInstance_fn create_instance =
            (rgo_vkCreateInstance_fn)dlsym(lib, "vkCreateInstance");
        rgo_vkDestroyInstance_fn destroy_instance =
            (rgo_vkDestroyInstance_fn)dlsym(lib, "vkDestroyInstance");
        rgo_vkEnumeratePhysicalDevices_fn enumerate_devices =
            (rgo_vkEnumeratePhysicalDevices_fn)dlsym(lib, "vkEnumeratePhysicalDevices");
        rgo_vkGetPhysicalDeviceQueueFamilyProperties_fn get_queue_props =
            (rgo_vkGetPhysicalDeviceQueueFamilyProperties_fn)dlsym(lib, "vkGetPhysicalDeviceQueueFamilyProperties");

        if (create_instance != NULL && destroy_instance != NULL &&
            enumerate_devices != NULL && get_queue_props != NULL) {
            rgo_VkApplicationInfo app_info = {
                RGO_VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL,
                "RAFAELIA-GPU-Probe", 1u, "RAFAELIA", 1u, RGO_VK_API_VERSION_1_0
            };
            rgo_VkInstanceCreateInfo create_info = {
                RGO_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL, 0u,
                &app_info, 0u, NULL, 0u, NULL
            };
            rgo_VkInstance instance = NULL;
            if (create_instance(&create_info, NULL, &instance) == 0 && instance != NULL) {
                rgo_VkPhysicalDevice devices[RGO_MAX_GPU_HANDLES];
                uint32_t device_count = RGO_MAX_GPU_HANDLES;
                if (enumerate_devices(instance, &device_count, devices) == 0 && device_count > 0u) {
                    uint32_t d;
                    if (device_count > RGO_MAX_GPU_HANDLES) device_count = RGO_MAX_GPU_HANDLES;
                    for (d = 0u; d < device_count; ++d) {
                        rgo_VkQueueFamilyProperties props[16];
                        uint32_t prop_count = 16u;
                        uint32_t q;
                        get_queue_props(devices[d], &prop_count, props);
                        if (prop_count > 16u) prop_count = 16u;
                        for (q = 0u; q < prop_count; ++q) {
                            if (props[q].queueCount > 0u &&
                                (props[q].queueFlags & RGO_VK_QUEUE_COMPUTE_BIT) != 0u) {
                                destroy_instance(instance, NULL);
                                dlclose(lib);
                                return RGO_OK;
                            }
                        }
                    }
                }
                destroy_instance(instance, NULL);
            }
        }
        dlclose(lib);
    }
    return RGO_ERR_GPU_RUNTIME;
}

static void rgo_probe_all(void) {
    if (atomic_load_explicit(&g_probe_done, memory_order_acquire)) return;
    rgo_lock(&g_gpu_probe_lock);
    if (!atomic_load_explicit(&g_probe_done, memory_order_relaxed)) {
        uint32_t caps = rgo_arch_mask();
        if (rgo_probe_opencl_once() == RGO_OK) caps |= RGO_CAP_OPENCL;
        if (rgo_probe_vulkan_once() == RGO_OK) caps |= RGO_CAP_VULKAN;
        atomic_store_explicit(&g_runtime_caps, caps, memory_order_release);
        atomic_store_explicit(&g_gpu_state,
            (caps & (RGO_CAP_OPENCL | RGO_CAP_VULKAN)) ? GPU_PRESENT : GPU_NO_DRIVER,
            memory_order_release);
        atomic_store_explicit(&g_probe_done, 1, memory_order_release);
    }
    rgo_unlock(&g_gpu_probe_lock);
}

int rgpu_probe_opencl(void) {
    rgo_probe_all();
    return (atomic_load_explicit(&g_runtime_caps, memory_order_acquire) & RGO_CAP_OPENCL)
        ? RGO_OK : RGO_ERR_GPU_RUNTIME;
}

int rgpu_probe_vulkan(void) {
    rgo_probe_all();
    return (atomic_load_explicit(&g_runtime_caps, memory_order_acquire) & RGO_CAP_VULKAN)
        ? RGO_OK : RGO_ERR_GPU_RUNTIME;
}

rgpu_state_t rgpu_get_state(void) {
    rgo_probe_all();
    return (rgpu_state_t)atomic_load_explicit(&g_gpu_state, memory_order_acquire);
}

uint32_t rgpu_get_core_count(void) {
    long online = sysconf(_SC_NPROCESSORS_ONLN);
    if (online > 0) {
        if (online > (long)MAX_CORES) online = (long)MAX_CORES;
        return (uint32_t)online;
    }
    {
        uint32_t arch = rgo_arch_mask();
        if (arch & (RGO_ARCH_ARM64 | RGO_ARCH_X86_64)) return 8u;
        if (arch & (RGO_ARCH_ARM32 | RGO_ARCH_X86)) return 4u;
    }
    return 1u;
}

void rcpu_map_toroidal(uint32_t* zones, uint32_t n) {
    uint32_t i;
    uint32_t cores = rgpu_get_core_count();
    if (!zones || n == 0u || cores == 0u) return;
    for (i = 0u; i < n; ++i) zones[i] = (i * 3u + 1u) % cores;
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

uint32_t rcrc32_sw(const uint8_t* data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    if (!data) return 0u;
    if (!atomic_load_explicit(&g_crc32_initialized, memory_order_acquire)) {
        rgo_lock(&g_crc_lock);
        if (!atomic_load_explicit(&g_crc32_initialized, memory_order_relaxed)) {
            crc32_init_table();
            atomic_store_explicit(&g_crc32_initialized, 1, memory_order_release);
        }
        rgo_unlock(&g_crc_lock);
    }
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
    uint64_t best_cost = UINT64_MAX;
    uint32_t thermal = remk_thermal_penalty();

    if (cores == 0u) cores = 1u;
    for (i = 0u; i < cores; ++i) {
        uint32_t load = atomic_load_explicit(&g_core_load[i], memory_order_relaxed);
        uint32_t freq_error = rgo_absdiff_u32(task_hz_q16 ? task_hz_q16 : Q16_ONE, g_core_freq_q16[i]);
        uint64_t cost = remk_cost_fn((uint64_t)(freq_error << 4u), load, intensity, thermal);
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
    for (i = 0u; i < MAX_CORES; ++i)
        atomic_store_explicit(&g_core_load[i], 0u, memory_order_relaxed);
    atomic_store_explicit(&g_thermal_state, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_wsq.head, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_wsq.tail, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_route_count_cpu, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_route_count_gpu, 0u, memory_order_relaxed);
    atomic_store_explicit(&g_last_route_reason, RGO_ROUTE_CPU_DEFAULT, memory_order_relaxed);
    rgo_mem_barrier();
}

uint32_t rgpu_runtime_caps(void) {
    rgo_probe_all();
    return atomic_load_explicit(&g_runtime_caps, memory_order_acquire) |
           atomic_load_explicit(&g_qualified_caps, memory_order_acquire);
}

void rgpu_set_policy(const rgpu_policy_t* policy) {
    rgpu_policy_t p;
    if (!policy) return;
    p = *policy;
    if (p.min_samples == 0u) p.min_samples = RGO_DEFAULT_MIN_SAMPLES;
    if (p.min_work_bytes == 0u) p.min_work_bytes = RGO_DEFAULT_MIN_WORK_BYTES;
    if (p.max_thermal > 100u) p.max_thermal = 100u;
    if (p.min_gain_permille > 1000u) p.min_gain_permille = 1000u;
    rgo_lock(&g_policy_lock);
    g_policy = p;
    rgo_unlock(&g_policy_lock);
}

void rgpu_get_policy(rgpu_policy_t* policy) {
    if (!policy) return;
    rgo_lock(&g_policy_lock);
    *policy = g_policy;
    rgo_unlock(&g_policy_lock);
}

int rgpu_measurement_qualifies(const rgpu_policy_t* policy,
                               const rgpu_measurement_t* measurement) {
    uint64_t saved;
    uint64_t scaled_saved;
    if (!policy || !measurement) return 0;
    if (!measurement->correctness_pass || !measurement->stable_environment) return 0;
    if (measurement->sample_count < policy->min_samples) return 0;
    if (measurement->work_bytes < policy->min_work_bytes) return 0;
    if (measurement->cpu_total_ns == 0u || measurement->gpu_total_ns == 0u) return 0;
    if (measurement->gpu_total_ns >= measurement->cpu_total_ns) return 0;
    saved = measurement->cpu_total_ns - measurement->gpu_total_ns;
    if (policy->min_gain_permille == 0u) return 1;
    if (saved > UINT64_MAX / 1000u) return 1;
    scaled_saved = saved * 1000u;
    return (scaled_saved / measurement->cpu_total_ns) >= policy->min_gain_permille;
}

int rgpu_record_total_cost(rgpu_backend_t backend,
                           const rgpu_measurement_t* measurement) {
    rgpu_policy_t policy;
    uint32_t cap;
    uint32_t qualified;
    if (!measurement || backend <= RGO_GPU_BACKEND_NONE || backend > RGO_GPU_BACKEND_VULKAN)
        return RGO_ERR_ARGS;
    rgo_probe_all();
    cap = rgo_backend_cap(backend);
    if ((atomic_load_explicit(&g_runtime_caps, memory_order_acquire) & cap) == 0u)
        return RGO_ERR_GPU_RUNTIME;
    rgpu_get_policy(&policy);
    qualified = (uint32_t)rgpu_measurement_qualifies(&policy, measurement);

    rgo_lock(&g_policy_lock);
    g_measurement[(uint32_t)backend] = *measurement;
    if (qualified) {
        g_qualified_backend = backend;
        atomic_fetch_or_explicit(&g_qualified_caps, cap | RGO_CAP_GPU_QUALIFIED, memory_order_release);
    } else {
        atomic_fetch_and_explicit(&g_qualified_caps, ~cap, memory_order_release);
        if ((atomic_load_explicit(&g_qualified_caps, memory_order_relaxed) &
             (RGO_CAP_OPENCL | RGO_CAP_VULKAN)) == 0u) {
            atomic_fetch_and_explicit(&g_qualified_caps, ~RGO_CAP_GPU_QUALIFIED, memory_order_release);
            g_qualified_backend = RGO_GPU_BACKEND_NONE;
        } else if (g_qualified_backend == backend) {
            g_qualified_backend = (atomic_load_explicit(&g_qualified_caps, memory_order_relaxed) & RGO_CAP_VULKAN)
                ? RGO_GPU_BACKEND_VULKAN : RGO_GPU_BACKEND_OPENCL;
        }
    }
    rgo_unlock(&g_policy_lock);
    return qualified ? RGO_OK : RGO_ERR_NOT_QUALIFIED;
}

int rgpu_backend_qualified(rgpu_backend_t backend) {
    uint32_t cap = rgo_backend_cap(backend);
    if (cap == 0u) return 0;
    return (atomic_load_explicit(&g_qualified_caps, memory_order_acquire) & cap) != 0u;
}

int rgpu_get_snapshot(rgpu_snapshot_t* snapshot) {
    if (!snapshot) return RGO_ERR_ARGS;
    rgo_probe_all();
    rgo_lock(&g_policy_lock);
    snapshot->state = (rgpu_state_t)atomic_load_explicit(&g_gpu_state, memory_order_acquire);
    snapshot->qualified_backend = g_qualified_backend;
    snapshot->runtime_caps = atomic_load_explicit(&g_runtime_caps, memory_order_acquire);
    snapshot->qualified_caps = atomic_load_explicit(&g_qualified_caps, memory_order_acquire);
    snapshot->policy = g_policy;
    snapshot->opencl_measurement = g_measurement[RGO_GPU_BACKEND_OPENCL];
    snapshot->vulkan_measurement = g_measurement[RGO_GPU_BACKEND_VULKAN];
    rgo_unlock(&g_policy_lock);
    snapshot->route_count_cpu = atomic_load_explicit(&g_route_count_cpu, memory_order_relaxed);
    snapshot->route_count_gpu = atomic_load_explicit(&g_route_count_gpu, memory_order_relaxed);
    snapshot->last_route_reason = atomic_load_explicit(&g_last_route_reason, memory_order_relaxed);
    snapshot->core_count = rgpu_get_core_count();
    snapshot->thermal_0_100 = remk_get_thermal();
    return RGO_OK;
}

void remk_set_thermal(uint32_t thermal_0_100) {
    if (thermal_0_100 > 100u) thermal_0_100 = 100u;
    atomic_store_explicit(&g_thermal_state, thermal_0_100, memory_order_relaxed);
}

uint32_t remk_get_thermal(void) {
    return atomic_load_explicit(&g_thermal_state, memory_order_relaxed);
}

int remk_enqueue_task(const rtask_t* task) {
    uint32_t head;
    uint32_t tail;
    if (!task) return RGO_ERR_ARGS;

    tail = atomic_load_explicit(&g_wsq.tail, memory_order_relaxed);
    head = atomic_load_explicit(&g_wsq.head, memory_order_acquire);
    if (((tail + 1u) % WSQ_SIZE) == (head % WSQ_SIZE)) return RGO_ERR_QUEUE_FULL;

    g_wsq.buffer[tail % WSQ_SIZE] = *task;
    atomic_store_explicit(&g_wsq.tail, tail + 1u, memory_order_release);
    return RGO_OK;
}

int remk_dequeue_task(rtask_t* task) {
    uint32_t head;
    uint32_t tail;
    if (!task) return RGO_ERR_ARGS;

    head = atomic_load_explicit(&g_wsq.head, memory_order_relaxed);
    tail = atomic_load_explicit(&g_wsq.tail, memory_order_acquire);
    if (head == tail) return RGO_ERR_QUEUE_EMPTY;

    *task = g_wsq.buffer[head % WSQ_SIZE];
    atomic_store_explicit(&g_wsq.head, head + 1u, memory_order_release);
    return RGO_OK;
}

static rgpu_backend_t remk_choose_gpu_backend(const rtask_t* task, uint32_t* reason) {
    rgpu_policy_t policy;
    uint32_t caps;
    if (!task) {
        if (reason) *reason = RGO_ROUTE_CPU_DEFAULT;
        return RGO_GPU_BACKEND_NONE;
    }
    caps = rgpu_runtime_caps();
    if ((caps & (RGO_CAP_OPENCL | RGO_CAP_VULKAN)) == 0u) {
        if (reason) *reason = RGO_ROUTE_GPU_NOT_CAPABLE;
        return RGO_GPU_BACKEND_NONE;
    }
    if (!task->gpu_candidate || task->intensity <= 50u) {
        if (reason) *reason = RGO_ROUTE_TASK_NOT_CANDIDATE;
        return RGO_GPU_BACKEND_NONE;
    }
    rgpu_get_policy(&policy);
    if (task->work_bytes < policy.min_work_bytes) {
        if (reason) *reason = RGO_ROUTE_BELOW_MIN_WORK;
        return RGO_GPU_BACKEND_NONE;
    }
    if (remk_get_thermal() > policy.max_thermal) {
        if (reason) *reason = RGO_ROUTE_THERMAL_GUARD;
        return RGO_GPU_BACKEND_NONE;
    }
    if (rgpu_backend_qualified(RGO_GPU_BACKEND_VULKAN)) {
        if (reason) *reason = RGO_ROUTE_GPU_TOTAL_COST_BETTER;
        return RGO_GPU_BACKEND_VULKAN;
    }
    if (rgpu_backend_qualified(RGO_GPU_BACKEND_OPENCL)) {
        if (reason) *reason = RGO_ROUTE_GPU_TOTAL_COST_BETTER;
        return RGO_GPU_BACKEND_OPENCL;
    }
    if (reason) *reason = RGO_ROUTE_GPU_NOT_QUALIFIED;
    return RGO_GPU_BACKEND_NONE;
}

int remk_run_once_ex(uint32_t* selected_core,
                     rgpu_backend_t* selected_backend,
                     uint32_t* route_reason) {
    rtask_t task;
    uint32_t core;
    uint32_t reason = RGO_ROUTE_CPU_DEFAULT;
    rgpu_backend_t backend;
    int rc = remk_dequeue_task(&task);
    if (rc != RGO_OK) return rc;

    if (task.submit_time_ns == 0u) task.submit_time_ns = remk_now_ns();
    backend = remk_choose_gpu_backend(&task, &reason);
    core = rscheduler_pick_core(task.task_hz_q16, task.intensity);

    if (selected_core) *selected_core = core;
    if (selected_backend) *selected_backend = backend;
    if (route_reason) *route_reason = reason;

    if (backend == RGO_GPU_BACKEND_NONE)
        atomic_fetch_add_explicit(&g_route_count_cpu, 1u, memory_order_relaxed);
    else
        atomic_fetch_add_explicit(&g_route_count_gpu, 1u, memory_order_relaxed);
    atomic_store_explicit(&g_last_route_reason, reason, memory_order_relaxed);

    atomic_fetch_sub_explicit(&g_core_load[core], 1u, memory_order_relaxed);
    return RGO_OK;
}

int remk_run_once(uint32_t* selected_core, uint32_t* used_gpu) {
    rgpu_backend_t backend = RGO_GPU_BACKEND_NONE;
    int rc = remk_run_once_ex(selected_core, &backend, NULL);
    if (used_gpu) *used_gpu = (backend == RGO_GPU_BACKEND_NONE) ? 0u : 1u;
    return rc;
}
