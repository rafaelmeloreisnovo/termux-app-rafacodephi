#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../app/src/main/cpp/lowlevel/raf_bitraf.h"
#include "../app/src/main/cpp/lowlevel/raf_vcpu.h"
#include "../rmr/include/rmr_hex_const.h"

static inline uint32_t rmr_mask_if_u32(uint32_t c){ return (uint32_t)-(int32_t)(c!=0u); }
static inline uint32_t rmr_select_u32(uint32_t m,uint32_t a,uint32_t b){ return (a&m)|(b&~m); }
static inline uint32_t rmr_absdiff_u32(uint32_t a,uint32_t b){ uint32_t m=(uint32_t)-((int32_t)(a<b)); return (a-b+m)^m; }
static inline uint32_t rmr_crc32c_step(uint32_t c){ return (c>>1) ^ (RMR_CRC32C_POLY_REV & (uint32_t)(-(int32_t)(c&1u))); }

static uint32_t crc32c(const void* b,size_t n){ const uint8_t* p=b; uint32_t c=~0u; while(n--){ c^=*p++; for(int i=0;i<8;i++) c=rmr_crc32c_step(c);} return ~c; }

int main(void){
 uint64_t x=bitraf_encode(1,2,3,4,5); if(bitraf_validate(x)!=0) return 11;
 uint8_t o,d; uint16_t l,m,f; bitraf_decode(x,&o,&d,&l,&m,&f); if(o!=1||d!=2||l!=3||m!=4||f!=5) return 12;
 if(RMR_PSS3_DELTA_Q16!=0x00002E14u) return 13;
 if(RMR_Q16_SQRT3_2!=0x0000DDB4u) return 14;
 uint32_t d3=rmr_absdiff_u32(0xDDB3u,56756u), d4=rmr_absdiff_u32(0xDDB4u,56756u); if(!(d4<d3)) return 15;
 raf_vcpu_init(60); if(raf_vcpu_step(0,1000)!=0) return 16; if(raf_vcpu_validate(0)!=0) return 17;
 uint32_t c = crc32c("RMR",3); if(c==0) return 18;
 uint32_t mask=rmr_mask_if_u32(1); if(rmr_select_u32(mask,7,9)!=7) return 19;
 puts("RMR pure core selftest OK");
 return 0;
}
