/* raf_termux_toolset.h
   Tabela deterministica de 100 ferramentas Termux (sem libc, sem dependencias)
*/
#ifndef RAF_TERMUX_TOOLSET_H
#define RAF_TERMUX_TOOLSET_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;

enum {
  RAF_TOOLSET_MAX = 100u
};

enum {
  RAF_TOOL_FLAG_SHELL = 1u,
  RAF_TOOL_FLAG_SYSTEM = 2u,
  RAF_TOOL_FLAG_NET = 4u,
  RAF_TOOL_FLAG_BUILD = 8u,
  RAF_TOOL_FLAG_LANG = 16u,
  RAF_TOOL_FLAG_APP = 32u,
  RAF_TOOL_FLAG_LIB = 64u
};

void RmR_toolset_init(void);
u32 RmR_toolset_count(void);
const char *RmR_toolset_name_at(u32 idx, u32 *len);
u16 RmR_toolset_flags_at(u32 idx);
u32 RmR_toolset_id_at(u32 idx);
u32 RmR_toolset_find(const u8 *name, u32 len, u32 *out_idx);

#endif
