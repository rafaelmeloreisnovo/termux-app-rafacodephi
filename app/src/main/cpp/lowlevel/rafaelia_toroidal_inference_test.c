#include "rafaelia_toroidal_inference.h"

#include <assert.h>
#include <math.h>

#define RAFAELIA_PI 3.14159265358979323846
#include <stdio.h>

int main(void) {
    double sphere = rafaelia_sphere_volume(3.0);
    assert(fabs(sphere - 113.0973355292) < 1e-6);

    double torus = rafaelia_torus_volume(4.0, 1.0);
    assert(fabs(torus - (8.0 * RAFAELIA_PI * RAFAELIA_PI)) < 1e-9);

    rafaelia_state7_t s = rafaelia_toroidal_map(0.2, 0.1, 0.3, 0.4);
    assert(s.u >= 0.0 && s.u < 1.0);
    assert(s.sigma >= 0.0 && s.sigma < 1.0);

    double c_next = 0.0, h_next = 0.0;
    rafaelia_update_coherence_entropy(0.8, 0.2, 1.0, 0.5, 0.25, &c_next, &h_next);
    assert(fabs(c_next - 0.85) < 1e-9);
    assert(fabs(h_next - 0.275) < 1e-9);

    double pulse[] = {1.0, 2.0, 3.0, 4.0};
    rafaelia_pulse_stats_t stats;
    assert(rafaelia_pulse_stats(pulse, 4, &stats) == 0);
    assert(stats.min == 1.0 && stats.max == 4.0 && stats.median == 2.5);

    printf("rafaelia_toroidal_inference_test OK\n");
    return 0;
}
