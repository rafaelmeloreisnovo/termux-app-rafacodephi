#include "gpu_compute_evidence_gate.h"

int rgpu_compute_dispatch_proven(rgpu_backend_t backend) {
    /*
     * CURRENT_STATE: runtime probes only establish API/device presence.
     * No active OpenCL NDRange or Vulkan compute dispatch provider is bound
     * to this orchestrator authority path yet, so promotion must fail closed.
     */
    (void)backend;
    return 0;
}
