/* raf_termux_registry.h
   Registro deterministico de ferramentas principais (sem libc, sem dependencias)
*/
#ifndef RAF_TERMUX_REGISTRY_H
#define RAF_TERMUX_REGISTRY_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef struct {
  u32 id;
  u8  name_len;
  u8  class_id;
  u16 version;
} raf_tool_entry_t;

typedef struct {
  const raf_tool_entry_t *entries;
  u32 count;
  u32 seal;
} raf_tool_registry_t;

const raf_tool_registry_t *RmR_termux_registry(void);

#endif
