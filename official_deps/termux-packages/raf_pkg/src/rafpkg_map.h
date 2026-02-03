// file: rafpkg_map.h
// RAFAELIA :: tiny open-addressing map (name -> index)
// SPDX-License-Identifier: MIT

#ifndef RAFPKG_MAP_H
#define RAFPKG_MAP_H

#include "../include/raf_pkg.h"

typedef struct {
  u32 hash;
  u32 idx;
  const char *key;
} raf_map_ent_t;

typedef struct {
  raf_map_ent_t *tab;
  u32 cap;   // power of two
  u32 len;
} raf_map_t;

int  raf_map_init(raf_map_t *m, u32 cap_pow2);
void raf_map_free(raf_map_t *m);

int  raf_map_put(raf_map_t *m, const char *key, u32 idx);
int  raf_map_get(const raf_map_t *m, const char *key, u32 *out_idx);

#endif
