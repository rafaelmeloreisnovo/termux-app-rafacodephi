#include "raf_bitraf.h"
#include <stdio.h>

uint64_t bitraf_encode(uint8_t opcode, uint8_t dir, uint16_t layer, uint16_t imm, uint16_t flags){
 uint64_t v=0;
 v|=((uint64_t)(opcode&0x3Fu))<<36;
 v|=((uint64_t)(dir&0x07u))<<33;
 v|=((uint64_t)(layer&0x03FFu))<<23;
 v|=((uint64_t)(imm&0x0FFFu))<<11;
 v|=((uint64_t)(flags&0x07FFu));
 return v;
}
void bitraf_decode(uint64_t i,uint8_t*o,uint8_t*d,uint16_t*l,uint16_t*m,uint16_t*f){
 if(o)*o=(uint8_t)((i>>36)&0x3F); if(d)*d=(uint8_t)((i>>33)&0x7); if(l)*l=(uint16_t)((i>>23)&0x3FF); if(m)*m=(uint16_t)((i>>11)&0xFFF); if(f)*f=(uint16_t)(i&0x7FF);
}
int bitraf_validate(uint64_t i){ return (i>>42)==0 ? 0 : -1; }
int bitraf_to_string(uint64_t i,char*out,int cap){
 uint8_t o,d; uint16_t l,m,f; bitraf_decode(i,&o,&d,&l,&m,&f);
 return snprintf(out,(size_t)cap,"{\"opcode\":%u,\"dir\":%u,\"layer\":%u,\"imm\":%u,\"flags\":%u}",o,d,l,m,f);
}
