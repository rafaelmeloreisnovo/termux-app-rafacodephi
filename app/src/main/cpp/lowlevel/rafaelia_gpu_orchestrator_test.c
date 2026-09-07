#include "rafaelia_gpu_orchestrator.h"

#include <assert.h>
#include <stdio.h>

static rgpu_policy_t default_policy(void) {
    rgpu_policy_t p = {
        7u,
        64u * 1024u,
        80u,
        50u
    };
    return p;
}

int main(void) {
    rgpu_policy_t p = default_policy();
    rgpu_measurement_t m = {
        1000000u,
        900000u,
        7u,
        65536u,
        1u,
        1u,
        {0u, 0u}
    };

    assert(rgpu_measurement_qualifies(&p, &m) == 1);

    m.sample_count = 6u;
    assert(rgpu_measurement_qualifies(&p, &m) == 0);

    m.sample_count = 7u;
    m.correctness_pass = 0u;
    assert(rgpu_measurement_qualifies(&p, &m) == 0);

    m.correctness_pass = 1u;
    m.stable_environment = 0u;
    assert(rgpu_measurement_qualifies(&p, &m) == 0);

    m.stable_environment = 1u;
    m.work_bytes = 32768u;
    assert(rgpu_measurement_qualifies(&p, &m) == 0);

    m.work_bytes = 65536u;
    m.gpu_total_ns = 970000u;
    assert(rgpu_measurement_qualifies(&p, &m) == 0);

    m.gpu_total_ns = 950000u;
    assert(rgpu_measurement_qualifies(&p, &m) == 1);

    m.gpu_total_ns = 1000000u;
    assert(rgpu_measurement_qualifies(&p, &m) == 0);

    printf("RGO_TEST PASS cores=%u caps=0x%x\n",
           rgpu_get_core_count(), rgpu_runtime_caps());
    return 0;
}
