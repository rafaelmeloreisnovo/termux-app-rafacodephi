#ifndef RAF_PHASE_RELEASE_GATE_H
#define RAF_PHASE_RELEASE_GATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAF_PHASE_GATE_LOGICAL_PERIOD_US 100000u
#define RAF_PHASE_GATE_BASE_FREQUENCY_MHZ 10000u
#define RAF_PHASE_GATE_STEP_MHZ 100u
#define RAF_PHASE_GATE_MAX_FREQUENCY_MHZ 999000u
#define RAF_PHASE_GATE_PHASES_PER_CYCLE 4u
#define RAF_PHASE_GATE_WINDOW_CYCLES 8u
#define RAF_PHASE_GATE_WINDOW_MASK 0xFFu

#define RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR 0x00000001u
#define RAF_PHASE_GATE_FLAG_FREQUENCY_SATURATED 0x00000002u
#define RAF_PHASE_GATE_FLAG_FAULT_LATCHED 0x80000000u

typedef enum {
  RAF_PHASE_GATE_OK = 0,
  RAF_PHASE_GATE_RELEASED = 1,
  RAF_PHASE_GATE_ERR_NULL = -1,
  RAF_PHASE_GATE_ERR_CONFIG = -2,
  RAF_PHASE_GATE_ERR_FAULT_LATCHED = -3,
  RAF_PHASE_GATE_ERR_PHASE_ORDER = -4,
  RAF_PHASE_GATE_ERR_EVIDENCE = -5
} raf_phase_gate_status_t;

typedef struct {
  uint32_t logical_period_us;
  uint32_t base_frequency_mhz;
  uint32_t frequency_step_mhz;
  uint32_t max_frequency_mhz;
  uint8_t phases_per_cycle;
  uint8_t window_cycles;
} raf_phase_gate_config_t;

typedef struct {
  raf_phase_gate_config_t config;
  uint32_t frequency_mhz;
  uint32_t released_frequency_mhz;
  uint32_t phase_q32;
  uint32_t phase_remainder;
  uint32_t logical_cycle;
  uint32_t release_epoch;
  uint32_t completed_mask;
  uint32_t staged_digest;
  uint32_t released_digest;
  uint32_t flags;
  int32_t fault_code;
  uint8_t expected_phase;
  uint8_t window_cycle;
} raf_phase_release_gate_t;

typedef struct {
  uint32_t frequency_mhz;
  uint32_t released_frequency_mhz;
  uint32_t phase_q32;
  uint32_t logical_cycle;
  uint32_t release_epoch;
  uint32_t released_digest;
  uint32_t flags;
  int32_t fault_code;
  uint8_t expected_phase;
  uint8_t window_cycle;
  uint8_t release_ready;
} raf_phase_release_event_t;

raf_phase_gate_config_t raf_phase_gate_default_config(void);
raf_phase_gate_status_t raf_phase_gate_validate_config(const raf_phase_gate_config_t *config);
raf_phase_gate_status_t raf_phase_release_gate_init(
    raf_phase_release_gate_t *gate,
    const raf_phase_gate_config_t *config);
raf_phase_gate_status_t raf_phase_release_gate_step(
    raf_phase_release_gate_t *gate,
    uint8_t observed_phase,
    uint8_t evidence_valid,
    uint32_t evidence_digest,
    raf_phase_release_event_t *event);
raf_phase_gate_status_t raf_phase_release_gate_reset_fault(
    raf_phase_release_gate_t *gate);

#ifdef __cplusplus
}
#endif

#endif
