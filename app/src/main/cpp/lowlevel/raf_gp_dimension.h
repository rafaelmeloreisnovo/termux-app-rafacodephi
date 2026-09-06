#ifndef RAF_GP_DIMENSION_H
#define RAF_GP_DIMENSION_H
#include <stddef.h>
#define RAF_GP_EINVAL (-1)
#define RAF_GP_UNIMPLEMENTED (-2)
/* Negative return means no JSON result. The output is cleared when writable. */
int raf_gp_dimension_json(const float* samples, size_t n, char* out, int cap);
#endif
