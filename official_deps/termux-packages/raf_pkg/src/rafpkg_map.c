// file: rafpkg_map.c
// RAFAELIA :: tiny open-addressing map (name -> index)
// SPDX-License-Identifier: MIT

#include "rafpkg_map.h"
#include "rafpkg_util.h"
#include <stdlib.h>
#include <string.h>

static u32 next_pow2(u32 x) {
  if (x < 2) return 2;
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x++;
  return x;
}

int raf_map_init(raf_map_t *m, u32 cap_pow2) {
  if (!m) return 0;
  u32 cap = next_pow2(cap_pow2);
  m->tab = (raf_map_ent_t*)calloc((size_t)cap, sizeof(raf_map_ent_t));
  if (!m->tab) return 0;
  m->cap = cap;
  m->len = 0;
  return 1;
}

void raf_map_free(raf_map_t *m) {
  if (!m) return;
  free(m->tab);
  m->tab = NULL;
  m->cap = 0;
  m->len = 0;
}

static int keys_equal(const char *a, const char *b) {
  return raf_streq(a, b);
}

// returns position for insert or existing; -1 on full
static i32 probe(raf_map_t *m, const char *key, u32 hash, int *found) {
  u32 mask = m->cap - 1;
  u32 pos = hash & mask;
  for (u32 i = 0; i < m->cap; i++) {
    raf_map_ent_t *e = &m->tab[pos];
    if (!e->key) { *found = 0; return (i32)pos; }
    if (e->hash == hash && keys_equal(e->key, key)) { *found = 1; return (i32)pos; }
    pos = (pos + 1) & mask;
  }
  return -1;
}

int raf_map_put(raf_map_t *m, const char *key, u32 idx) {
  if (!m || !m->tab || !key) return 0;
  // keep load factor <= ~0.70
  if (m->len * 10u >= m->cap * 7u) {
    // no resize in this minimal impl; fail to avoid infinite probe
    return 0;
  }
  u32 h = raf_hash_fnv1a(key, (u32)strlen(key));
  int found = 0;
  i32 pos = probe(m, key, h, &found);
  if (pos < 0) return 0;
  raf_map_ent_t *e = &m->tab[(u32)pos];
  if (!found) m->len++;
  e->hash = h;
  e->idx  = idx;
  e->key  = key;
  return 1;
}

int raf_map_get(const raf_map_t *m, const char *key, u32 *out_idx) {
  if (!m || !m->tab || !key) return 0;
  u32 mask = m->cap - 1;
  u32 h = raf_hash_fnv1a(key, (u32)strlen(key));
  u32 pos = h & mask;
  for (u32 i = 0; i < m->cap; i++) {
    const raf_map_ent_t *e = &m->tab[pos];
    if (!e->key) return 0;
    if (e->hash == h && keys_equal(e->key, key)) {
      if (out_idx) *out_idx = e->idx;
      return 1;
    }
    pos = (pos + 1) & mask;
  }
  return 0;
}
