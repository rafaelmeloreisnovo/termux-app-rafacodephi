#include "raf_gp_dimension.h"

int raf_gp_dimension_json(const float *samples, size_t n, char *out, int cap) {
    if (out && cap > 0) out[0] = '\0';
    if (!samples || n < 4 || !out || cap < 64) return RAF_GP_EINVAL;

    /* TOKEN_VAZIO: the previous implementation never completed for valid n
     * and its JSON contained fixed correlation/slope values. Preserve the
     * API error channel until a bounded estimator and its numerical gate
     * exist; an unavailable dimension is not a measured zero. */
    return RAF_GP_UNIMPLEMENTED;
}
