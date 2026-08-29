#include "rafaelia_toroidal_inference.h"

#define RAFAELIA_PI 3.14159265358979323846

static double freestanding_fabs(double x) { return x < 0 ? -x : x; }

int main(void) {
    double sphere = rafaelia_sphere_volume(3.0);
    if (!(freestanding_fabs(sphere - 113.0973355292) < 1e-6)) return 1;

    double torus = rafaelia_torus_volume(4.0, 1.0);
    if (!(freestanding_fabs(torus - (8.0 * RAFAELIA_PI * RAFAELIA_PI)) < 1e-9)) return 2;

    rafaelia_state7_t s = rafaelia_toroidal_map(0.2, 0.1, 0.3, 0.4);
    if (!(s.u >= 0.0 && s.u < 1.0)) return 3;
    if (!(s.sigma >= 0.0 && s.sigma < 1.0)) return 4;

    double c_next = 0.0, h_next = 0.0;
    rafaelia_update_coherence_entropy(0.8, 0.2, 1.0, 0.5, 0.25, &c_next, &h_next);
    if (!(freestanding_fabs(c_next - 0.85) < 1e-9)) return 5;
    if (!(freestanding_fabs(h_next - 0.275) < 1e-9)) return 6;

    double pulse[] = {1.0, 2.0, 3.0, 4.0};
    rafaelia_pulse_stats_t stats;
    if (!(rafaelia_pulse_stats(pulse, 4, &stats) == 0)) return 7;
    if (!(stats.min == 1.0 && stats.max == 4.0 && stats.median == 2.5)) return 8;

    return 0;
}
