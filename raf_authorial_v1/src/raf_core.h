/* SPDX-License-Identifier: MIT */
#ifndef RAFCODEPHI_AUTHORIAL_CORE_V1_H
#define RAFCODEPHI_AUTHORIAL_CORE_V1_H
/* Pure C11 ABI: 8-bit bytes, 32-bit unsigned int, no hosted includes. */
_Static_assert(sizeof(unsigned int) == 4u, "requires a 32-bit unsigned int");
_Static_assert(((unsigned char)256u) == 0u, "requires 8-bit bytes");
#ifndef RAF_UNROLL
#define RAF_UNROLL 1
#endif
#if RAF_UNROLL != 1 && RAF_UNROLL != 4
#error RAF_UNROLL must be 1 or 4
#endif
/* Non-cryptographic FNV-1a32. NEVER use for security integrity.
 * data may be null only for count 0; out must not be null. */
int raf_fold32(const unsigned char *data, unsigned int count,
               unsigned int *out);
/* Strict 1..10 decimal digits, accepts UINT32_MAX, rejects overflow.
 * On invalid input, out is unchanged. */
int raf_decimal_u32(const unsigned char *data, unsigned int count,
                    unsigned int *out);
#endif
