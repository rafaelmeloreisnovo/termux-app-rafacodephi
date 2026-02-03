// file: rafpkg_store.h
// RAFAELIA :: rafstore (tiny persistent cache)
// SPDX-License-Identifier: MIT
//
// Goal: keep "logical" state (indexes / audit metadata) using less physical IO.
// Design: simple binary store, open-addressing hash table in memory, full rewrite on save.
// No external deps.

#pragma once

#include "rafpkg_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  u64 hash;      // fnv1a64(path)
  u64 mtime_ns;  // best-effort (seconds->ns)
  u64 size;
  u8  kind;      // 0=unk, 1=elf, 2=script, 3=other
  u16 path_len;
  char *path;    // NUL-terminated
} raf_store_rec_t;

typedef struct {
  raf_store_rec_t *recs;
  u32 cnt;
  u32 cap;

  // hash table of indices into recs: slots[i] = rec_index+1, 0 means empty
  u32 *slots;
  u32 slots_cap;
} raf_store_t;

// initialize empty store
void raf_store_init(raf_store_t *st);
void raf_store_free(raf_store_t *st);

// load/save store from/to path; returns 1 on success, 0 on failure
int  raf_store_load(raf_store_t *st, const char *path);
int  raf_store_save(const raf_store_t *st, const char *path);

// lookup by full path, verifying mtime/size; returns 1 hit else 0
int  raf_store_get(const raf_store_t *st, const char *path, u64 mtime_ns, u64 size, u8 *kind_out);

// upsert record
int  raf_store_put(raf_store_t *st, const char *path, u64 mtime_ns, u64 size, u8 kind);

// stable hash
u64  raf_fnv1a64(const char *s);

#ifdef __cplusplus
}
#endif
