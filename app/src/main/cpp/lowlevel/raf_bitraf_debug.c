#include "raf_bitraf.h"

#if !defined(RMR_NO_DEBUG_STRING)
#include <stdio.h>

int bitraf_to_string(uint64_t i,char*out,int cap){
 uint8_t o,d; uint16_t l,m,f; bitraf_decode(i,&o,&d,&l,&m,&f);
 return snprintf(out,(size_t)cap,"{\"opcode\":%u,\"dir\":%u,\"layer\":%u,\"imm\":%u,\"flags\":%u}",o,d,l,m,f);
}
#endif
