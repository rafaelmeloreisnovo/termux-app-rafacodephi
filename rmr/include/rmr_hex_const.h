#pragma once
#ifndef RMR_HEX_CONST_H
#define RMR_HEX_CONST_H

#include <stdint.h>

/* Q16.16 canonical constants (single source of truth). */
#define RMR_Q16_ONE         UINT32_C(0x00010000)
#define RMR_Q16_HALF        UINT32_C(0x00008000)
#define RMR_Q16_SQRT3_2     UINT32_C(0x0000DDB4)
#define RMR_Q16_PHI         UINT32_C(0x00019DFD)
#define RMR_Q16_PI          UINT32_C(0x0003243F)
#define RMR_Q16_2PI         UINT32_C(0x0006487E)
#define RMR_Q16_INV6        UINT32_C(0x00002AAB)
#define RMR_Q16_INV120      UINT32_C(0x00000222)

#endif /* RMR_HEX_CONST_H */
