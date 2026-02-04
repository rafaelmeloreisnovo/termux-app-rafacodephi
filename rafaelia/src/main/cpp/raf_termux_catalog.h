/* raf_termux_catalog.h
   Catalogo deterministico para absorver listas de pacotes Termux (sem libc)
*/
#ifndef RAF_TERMUX_CATALOG_H
#define RAF_TERMUX_CATALOG_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

#ifndef RAF_CATALOG_MAX
#define RAF_CATALOG_MAX 2048u
#endif

typedef struct {
  u32 id;
  u16 name_len;
  u16 flags;
} raf_pkg_entry_t;

typedef struct {
  raf_pkg_entry_t entries[RAF_CATALOG_MAX];
  u32 count;
  u32 seed;
  u32 seal;
} raf_pkg_catalog_t;

void RmR_catalog_init(raf_pkg_catalog_t *c, u32 seed);
u32 RmR_catalog_load(raf_pkg_catalog_t *c, const u8 *buf, u32 len);

#endif
