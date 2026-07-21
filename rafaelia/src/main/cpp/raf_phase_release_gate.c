#include "raf_phase_release_gate.h"

#define RAF_PHASE_GATE_PHASE_DENOMINATOR 1000000000ULL
#define RAF_PHASE_GATE_Q32_TURN 4294967296ULL

_Static_assert(RAF_PHASE_GATE_PHASES_PER_CYCLE == 4u, "BITRAF requires four ordered phases");
_Static_assert(RAF_PHASE_GATE_WINDOW_CYCLES == 8u, "release window requires eight cycles");
_Static_assert(RAF_PHASE_GATE_WINDOW_CYCLES <= 32u, "completed mask width exceeded");

static void raf_phase_gate_zero(void *dst, uint32_t size) {
  uint8_t *cursor = (uint8_t *)dst;
  while (size-- != 0u) {
    *cursor++ = 0u;
  }
}

static uint32_t raf_phase_gate_rotl32(uint32_t value, uint8_t shift) {
  uint8_t amount = (uint8_t)(shift & 31u);
  if (amount == 0u) {
    return value;
  }
  return (value << amount) | (value >> (32u - amount));
}

static uint32_t raf_phase_gate_mix(
    uint32_t accumulator,
    uint32_t evidence_digest,
    uint8_t phase,
    uint8_t cycle) {
  uint32_t lane = evidence_digest ^ ((uint32_t)phase << 24) ^ ((uint32_t)cycle << 16);
  accumulator ^= lane + 0x9E3779B9u + (accumulator << 6) + (accumulator >> 2);
  return raf_phase_gate_rotl32(accumulator, (uint8_t)(5u + phase));
}

static void raf_phase_gate_fill_event(
    const raf_phase_release_gate_t *gate,
    uint8_t release_ready,
    raf_phase_release_event_t *event) {
  if (event == 0) {
    return;
  }
  event->frequency_mhz = gate->frequency_mhz;
  event->phase_q32 = gate->phase_q32;
  event->logical_cycle = gate->logical_cycle;
  event->release_epoch = gate->release_epoch;
  event->released_digest = gate->released_digest;
  event->flags = gate->flags;
  event->fault_code = gate->fault_code;
  event->expected_phase = gate->expected_phase;
  event->window_cycle = gate->window_cycle;
  event->release_ready = release_ready;
}

static void raf_phase_gate_advance_phase(raf_phase_release_gate_t *gate) {
  uint64_t cycle_numerator =
      (uint64_t)gate->frequency_mhz * (uint64_t)gate->config.logical_period_us;
  uint64_t fractional = cycle_numerator % RAF_PHASE_GATE_PHASE_DENOMINATOR;
  uint64_t q32_numerator =
      fractional * RAF_PHASE_GATE_Q32_TURN + (uint64_t)gate->phase_remainder;

  gate->phase_q32 += (uint32_t)(q32_numerator / RAF_PHASE_GATE_PHASE_DENOMINATOR);
  gate->phase_remainder = (uint32_t)(q32_numerator % RAF_PHASE_GATE_PHASE_DENOMINATOR);

  if (fractional == 0u) {
    gate->flags |= RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR;
  } else {
    gate->flags &= ~RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR;
  }
}

static void raf_phase_gate_advance_frequency(raf_phase_release_gate_t *gate) {
  uint32_t remaining = gate->config.max_frequency_mhz - gate->frequency_mhz;
  if (gate->config.frequency_step_mhz >= remaining) {
    gate->frequency_mhz = gate->config.max_frequency_mhz;
    gate->flags |= RAF_PHASE_GATE_FLAG_FREQUENCY_SATURATED;
    return;
  }
  gate->frequency_mhz += gate->config.frequency_step_mhz;
}

static raf_phase_gate_status_t raf_phase_gate_latch_fault(
    raf_phase_release_gate_t *gate,
    raf_phase_gate_status_t code,
    raf_phase_release_event_t *event) {
  gate->fault_code = (int32_t)code;
  gate->flags |= RAF_PHASE_GATE_FLAG_FAULT_LATCHED;
  raf_phase_gate_fill_event(gate, 0u, event);
  return code;
}

raf_phase_gate_config_t raf_phase_gate_default_config(void) {
  raf_phase_gate_config_t config;
  config.logical_period_us = RAF_PHASE_GATE_LOGICAL_PERIOD_US;
  config.base_frequency_mhz = RAF_PHASE_GATE_BASE_FREQUENCY_MHZ;
  config.frequency_step_mhz = RAF_PHASE_GATE_STEP_MHZ;
  config.max_frequency_mhz = RAF_PHASE_GATE_MAX_FREQUENCY_MHZ;
  config.phases_per_cycle = RAF_PHASE_GATE_PHASES_PER_CYCLE;
  config.window_cycles = RAF_PHASE_GATE_WINDOW_CYCLES;
  return config;
}

raf_phase_gate_status_t raf_phase_gate_validate_config(const raf_phase_gate_config_t *config) {
  if (config == 0) {
    return RAF_PHASE_GATE_ERR_NULL;
  }
  if (config->logical_period_us == 0u ||
      config->base_frequency_mhz == 0u ||
      config->frequency_step_mhz == 0u ||
      config->base_frequency_mhz > config->max_frequency_mhz ||
      config->phases_per_cycle != RAF_PHASE_GATE_PHASES_PER_CYCLE ||
      config->window_cycles != RAF_PHASE_GATE_WINDOW_CYCLES) {
    return RAF_PHASE_GATE_ERR_CONFIG;
  }
  return RAF_PHASE_GATE_OK;
}

raf_phase_gate_status_t raf_phase_release_gate_init(
    raf_phase_release_gate_t *gate,
    const raf_phase_gate_config_t *config) {
  raf_phase_gate_config_t resolved;
  raf_phase_gate_status_t status;

  if (gate == 0) {
    return RAF_PHASE_GATE_ERR_NULL;
  }
  resolved = config == 0 ? raf_phase_gate_default_config() : *config;
  status = raf_phase_gate_validate_config(&resolved);
  if (status != RAF_PHASE_GATE_OK) {
    return status;
  }

  raf_phase_gate_zero(gate, (uint32_t)sizeof(*gate));
  gate->config = resolved;
  gate->frequency_mhz = resolved.base_frequency_mhz;
  gate->staged_digest = 0x52414638u;
  return RAF_PHASE_GATE_OK;
}

raf_phase_gate_status_t raf_phase_release_gate_step(
    raf_phase_release_gate_t *gate,
    uint8_t observed_phase,
    uint8_t evidence_valid,
    uint32_t evidence_digest,
    raf_phase_release_event_t *event) {
  uint8_t release_ready = 0u;

  if (gate == 0) {
    return RAF_PHASE_GATE_ERR_NULL;
  }
  if ((gate->flags & RAF_PHASE_GATE_FLAG_FAULT_LATCHED) != 0u) {
    raf_phase_gate_fill_event(gate, 0u, event);
    return RAF_PHASE_GATE_ERR_FAULT_LATCHED;
  }
  if (observed_phase != gate->expected_phase) {
    return raf_phase_gate_latch_fault(gate, RAF_PHASE_GATE_ERR_PHASE_ORDER, event);
  }
  if (evidence_valid == 0u) {
    return raf_phase_gate_latch_fault(gate, RAF_PHASE_GATE_ERR_EVIDENCE, event);
  }

  gate->staged_digest = raf_phase_gate_mix(
      gate->staged_digest,
      evidence_digest,
      observed_phase,
      gate->window_cycle);

  gate->expected_phase++;
  if (gate->expected_phase == gate->config.phases_per_cycle) {
    gate->expected_phase = 0u;
    gate->completed_mask |= (uint32_t)1u << gate->window_cycle;
    gate->logical_cycle++;
    raf_phase_gate_advance_phase(gate);
    gate->window_cycle++;

    if (gate->window_cycle == gate->config.window_cycles) {
      if (gate->completed_mask != RAF_PHASE_GATE_WINDOW_MASK) {
        return raf_phase_gate_latch_fault(gate, RAF_PHASE_GATE_ERR_EVIDENCE, event);
      }
      gate->released_digest = gate->staged_digest;
      gate->release_epoch++;
      gate->completed_mask = 0u;
      gate->window_cycle = 0u;
      gate->staged_digest = 0x52414638u ^ gate->release_epoch;
      raf_phase_gate_advance_frequency(gate);
      release_ready = 1u;
    }
  }

  raf_phase_gate_fill_event(gate, release_ready, event);
  return release_ready != 0u ? RAF_PHASE_GATE_RELEASED : RAF_PHASE_GATE_OK;
}

raf_phase_gate_status_t raf_phase_release_gate_reset_fault(
    raf_phase_release_gate_t *gate) {
  if (gate == 0) {
    return RAF_PHASE_GATE_ERR_NULL;
  }
  gate->completed_mask = 0u;
  gate->window_cycle = 0u;
  gate->expected_phase = 0u;
  gate->staged_digest = 0x52414638u ^ gate->release_epoch;
  gate->fault_code = 0;
  gate->flags &= ~RAF_PHASE_GATE_FLAG_FAULT_LATCHED;
  return RAF_PHASE_GATE_OK;
}
