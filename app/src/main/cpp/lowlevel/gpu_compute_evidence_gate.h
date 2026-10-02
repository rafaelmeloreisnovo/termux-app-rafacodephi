#ifndef RAFAELIA_GPU_COMPUTE_EVIDENCE_GATE_H
#define RAFAELIA_GPU_COMPUTE_EVIDENCE_GATE_H

#include "rafaelia_gpu_orchestrator.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fail-closed evidence boundary.
 *
 * Runtime/API discovery (OpenCL/Vulkan present) is not evidence that a real
 * compute dispatch executed the measured workload. This remains 0 until a
 * producer that performs real dispatch+wait+readback and binds correctness
 * evidence is integrated and tested in the same authority path.
 */
int rgpu_compute_dispatch_proven(rgpu_backend_t backend);

#ifdef __cplusplus
}
#endif

#endif
