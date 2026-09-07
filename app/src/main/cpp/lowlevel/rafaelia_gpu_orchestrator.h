#ifndef RAFAELIA_GPU_ORCHESTRATOR_H
#define RAFAELIA_GPU_ORCHESTRATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RGO_OK 0
#define RGO_ERR_ARGS -1
#define RGO_ERR_DLOPEN -2
#define RGO_ERR_DLSYM -3
#define RGO_ERR_GPU_RUNTIME -4
#define RGO_ERR_QUEUE_FULL -5
#define RGO_ERR_QUEUE_EMPTY -6
#define RGO_ERR_NOT_QUALIFIED -7

#define MAX_CORES 16u
#define WSQ_SIZE 64u

#define RGO_CAP_OPENCL        (1u << 8)
#define RGO_CAP_VULKAN        (1u << 9)
#define RGO_CAP_GPU_QUALIFIED (1u << 10)

#define RGO_DEFAULT_MIN_SAMPLES 7u
#define RGO_DEFAULT_MIN_WORK_BYTES (64u * 1024u)
#define RGO_DEFAULT_MAX_THERMAL 80u
#define RGO_DEFAULT_MIN_GAIN_PERMILLE 50u

typedef enum {
    GPU_UNKNOWN = 0,
    GPU_PRESENT,
    GPU_NO_DRIVER,
    GPU_FAIL_RUNTIME
} rgpu_state_t;

typedef enum {
    RGO_GPU_BACKEND_NONE = 0,
    RGO_GPU_BACKEND_OPENCL = 1,
    RGO_GPU_BACKEND_VULKAN = 2
} rgpu_backend_t;

typedef enum {
    RGO_ROUTE_CPU_DEFAULT = 0,
    RGO_ROUTE_GPU_NOT_CAPABLE,
    RGO_ROUTE_GPU_NOT_QUALIFIED,
    RGO_ROUTE_TASK_NOT_CANDIDATE,
    RGO_ROUTE_BELOW_MIN_WORK,
    RGO_ROUTE_THERMAL_GUARD,
    RGO_ROUTE_GPU_TOTAL_COST_BETTER
} rgpu_route_reason_t;

typedef struct {
    uint32_t min_samples;
    uint32_t min_work_bytes;
    uint32_t max_thermal;
    uint32_t min_gain_permille;
} rgpu_policy_t;

typedef struct {
    uint64_t cpu_total_ns;
    uint64_t gpu_total_ns;
    uint32_t sample_count;
    uint32_t work_bytes;
    uint8_t correctness_pass;
    uint8_t stable_environment;
    uint8_t reserved[2];
} rgpu_measurement_t;

typedef struct {
    rgpu_state_t state;
    rgpu_backend_t qualified_backend;
    uint32_t runtime_caps;
    uint32_t qualified_caps;
    rgpu_policy_t policy;
    rgpu_measurement_t opencl_measurement;
    rgpu_measurement_t vulkan_measurement;
    uint32_t route_count_cpu;
    uint32_t route_count_gpu;
    uint32_t last_route_reason;
    uint32_t core_count;
    uint32_t thermal_0_100;
} rgpu_snapshot_t;

typedef struct {
    uint32_t id;
    uint32_t task_hz_q16;
    uint32_t intensity;
    uint64_t deadline_ns;
    uint64_t submit_time_ns;
    uint32_t priority;
    uint8_t gpu_candidate;
    uint8_t reserved[3];
    uint32_t work_bytes;
} rtask_t;

int rgpu_probe_opencl(void);
int rgpu_probe_vulkan(void);
rgpu_state_t rgpu_get_state(void);
uint32_t rgpu_get_core_count(void);
void rcpu_map_toroidal(uint32_t* zones, uint32_t n);
uint32_t rcrc32_sw(const uint8_t* data, uint32_t len);
uint32_t rscheduler_pick_core(uint32_t task_hz_q16, uint32_t intensity);
void rscheduler_set_load(uint32_t core_idx, uint32_t load_q16);
void rscheduler_reset(void);
uint32_t rgpu_runtime_caps(void);

void rgpu_set_policy(const rgpu_policy_t* policy);
void rgpu_get_policy(rgpu_policy_t* policy);
int rgpu_measurement_qualifies(const rgpu_policy_t* policy,
                               const rgpu_measurement_t* measurement);
int rgpu_record_total_cost(rgpu_backend_t backend,
                           const rgpu_measurement_t* measurement);
int rgpu_backend_qualified(rgpu_backend_t backend);
int rgpu_get_snapshot(rgpu_snapshot_t* snapshot);

void remk_set_thermal(uint32_t thermal_0_100);
uint32_t remk_get_thermal(void);
int remk_enqueue_task(const rtask_t* task);
int remk_dequeue_task(rtask_t* task);
int remk_run_once_ex(uint32_t* selected_core,
                     rgpu_backend_t* selected_backend,
                     uint32_t* route_reason);
int remk_run_once(uint32_t* selected_core, uint32_t* used_gpu);

#ifdef __cplusplus
}
#endif

#endif
