#include "raf_phase_release_gate.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void complete_cycle(
    raf_phase_release_gate_t *gate,
    uint32_t cycle,
    raf_phase_release_event_t *event,
    int expect_release) {
  for (uint8_t phase = 0u; phase < RAF_PHASE_GATE_PHASES_PER_CYCLE; ++phase) {
    raf_phase_gate_status_t status = raf_phase_release_gate_step(
        gate,
        phase,
        1u,
        0xA5000000u ^ (cycle << 8) ^ phase,
        event);
    if (phase == 3u && expect_release != 0) {
      assert(status == RAF_PHASE_GATE_RELEASED);
      assert(event->release_ready == 1u);
    } else {
      assert(status == RAF_PHASE_GATE_OK);
      assert(event->release_ready == 0u);
    }
  }
}

int main(void) {
  raf_phase_release_gate_t gate;
  raf_phase_release_event_t event;
  raf_phase_gate_config_t config = raf_phase_gate_default_config();

  assert(raf_phase_release_gate_init(&gate, &config) == RAF_PHASE_GATE_OK);
  assert(gate.frequency_mhz == 10000u);
  assert(gate.release_epoch == 0u);

  for (uint32_t cycle = 0u; cycle < 7u; ++cycle) {
    complete_cycle(&gate, cycle, &event, 0);
    assert(gate.release_epoch == 0u);
    assert(gate.released_digest == 0u);
  }

  complete_cycle(&gate, 7u, &event, 1);
  assert(gate.release_epoch == 1u);
  assert(gate.frequency_mhz == 10100u);
  assert(gate.completed_mask == 0u);
  assert(gate.window_cycle == 0u);
  assert((event.flags & RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR) != 0u);

  complete_cycle(&gate, 8u, &event, 0);
  assert(gate.phase_q32 == 42949672u);
  assert((gate.flags & RAF_PHASE_GATE_FLAG_ALIAS_ANCHOR) == 0u);

  assert(raf_phase_release_gate_step(&gate, 2u, 1u, 1u, &event) ==
         RAF_PHASE_GATE_ERR_PHASE_ORDER);
  assert((gate.flags & RAF_PHASE_GATE_FLAG_FAULT_LATCHED) != 0u);
  assert(raf_phase_release_gate_step(&gate, 0u, 1u, 1u, &event) ==
         RAF_PHASE_GATE_ERR_FAULT_LATCHED);
  assert(raf_phase_release_gate_reset_fault(&gate) == RAF_PHASE_GATE_OK);
  assert(gate.expected_phase == 0u);

  assert(raf_phase_release_gate_step(&gate, 0u, 0u, 0u, &event) ==
         RAF_PHASE_GATE_ERR_EVIDENCE);
  assert(gate.release_epoch == 1u);

  puts("raf_phase_release_gate: PASS");
  return 0;
}
