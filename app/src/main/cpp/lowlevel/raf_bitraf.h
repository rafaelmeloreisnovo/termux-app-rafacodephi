#ifndef RAF_BITRAF_H
#define RAF_BITRAF_H
#include <stdint.h>

uint64_t bitraf_encode(uint8_t opcode, uint8_t dir, uint16_t layer, uint16_t imm, uint16_t flags);
void bitraf_decode(uint64_t insn, uint8_t* opcode, uint8_t* dir, uint16_t* layer, uint16_t* imm, uint16_t* flags);
int bitraf_validate(uint64_t insn);
#if !defined(RMR_NO_DEBUG_STRING)
int bitraf_to_string(uint64_t insn, char* out, int cap);
#endif

#endif
