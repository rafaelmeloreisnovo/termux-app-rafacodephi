/* raf_termux_catalog.c
   Catalogo deterministico para absorver listas de pacotes Termux (sem libc)
*/

#include "raf_termux_catalog.h"

static u8 RmR_is_space(u8 v){
  return (v == (u8)'\n' || v == (u8)'\r' || v == (u8)'\t' || v == (u8)' ') ? 1u : 0u;
}

static u32 RmR_hash_token(const u8 *buf, u32 len, u32 seed){
  u32 h = 2166136261u ^ seed;
  for(u32 i=0u;i<len;i++){
    h ^= (u32)buf[i];
    h *= 16777619u;
  }
  return h;
}

void RmR_catalog_init(raf_pkg_catalog_t *c, u32 seed){
  if(!c) return;
  for(u32 i=0u;i<RAF_CATALOG_MAX;i++){
    c->entries[i].id = 0u;
    c->entries[i].name_len = 0u;
    c->entries[i].flags = 0u;
  }
  c->count = 0u;
  c->seed = seed;
  c->seal = 0x52414643u; /* "RAFC" */
}

u32 RmR_catalog_load(raf_pkg_catalog_t *c, const u8 *buf, u32 len){
  if(!c || !buf || len == 0u) return 0u;
  u32 pos = 0u;
  u32 added = 0u;
  while(pos < len && c->count < RAF_CATALOG_MAX){
    while(pos < len && RmR_is_space(buf[pos])) pos++;
    if(pos >= len) break;
    u32 start = pos;
    while(pos < len && !RmR_is_space(buf[pos])) pos++;
    u32 tok_len = pos - start;
    if(tok_len > 0u){
      u32 id = RmR_hash_token(&buf[start], tok_len, c->seed);
      raf_pkg_entry_t *e = &c->entries[c->count];
      e->id = id;
      e->name_len = (u16)tok_len;
      e->flags = 0u;
      c->count++;
      added++;
    }
  }
  return added;
}
