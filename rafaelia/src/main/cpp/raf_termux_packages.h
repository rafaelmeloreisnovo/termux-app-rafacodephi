/* raf_termux_packages.h
   IDs deterministicos de pacotes Termux (sem libc, gerado)
*/
#ifndef RAF_TERMUX_PACKAGES_H
#define RAF_TERMUX_PACKAGES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef struct {
  u32 id;
  u16 name_len;
  u16 flags;
} raf_termux_pkg_id_t;

typedef struct {
  const raf_termux_pkg_id_t *entries;
  u32 count;
  u32 seed;
  u32 seal;
} raf_termux_pkg_table_t;

const raf_termux_pkg_table_t *RmR_termux_packages(void);

#endif
