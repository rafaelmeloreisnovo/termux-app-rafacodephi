// file: rafpkg_store.c
// RAFAELIA :: rafstore (tiny persistent cache)
// SPDX-License-Identifier: MIT

#include "rafpkg_store.h"

#include <fcntl.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef RAFSTORE_MAGIC
#define RAFSTORE_MAGIC 0x31534652u /* 'RFS1' little endian */
#endif

static u32 next_pow2(u32 x) {
  if (x < 2) return 2;
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

u64 raf_fnv1a64(const char *s) {
  const u64 FNV_OFF = 1469598103934665603ULL;
  const u64 FNV_PR  = 1099511628211ULL;
  u64 h = FNV_OFF;
  const unsigned char *p = (const unsigned char*)s;
  while (*p) {
    h ^= (u64)(*p++);
    h *= FNV_PR;
  }
  return h;
}

void raf_store_init(raf_store_t *st) {
  memset(st, 0, sizeof(*st));
  // small default table so we can cache even on first run (no existing file)
  st->slots_cap = 1024;
  st->slots = (u32*)malloc(st->slots_cap * sizeof(u32));
  if (st->slots) {
    for (u32 i = 0; i < st->slots_cap; i++) st->slots[i] = 0xFFFFFFFFu;
  } else {
    st->slots_cap = 0;
  }
}

static void free_rec(raf_store_rec_t *r) {
  if (r && r->path) free(r->path);
  if (r) memset(r, 0, sizeof(*r));
}

void raf_store_free(raf_store_t *st) {
  if (!st) return;
  for (u32 i = 0; i < st->cnt; i++) free_rec(&st->recs[i]);
  free(st->recs);
  free(st->slots);
  memset(st, 0, sizeof(*st));
}

static void ensure_slots(raf_store_t *st, u32 want_cnt) {
  if (!st) return;
  u32 need = next_pow2(want_cnt * 2u + 1u);
  if (need < 256u) need = 256u;
  if (st->slots && st->slots_cap >= need) return;

  u32 *new_slots = (u32*)calloc((size_t)need, sizeof(u32));
  if (!new_slots) { fprintf(stderr, "rafstore: OOM slots\n"); exit(2); }

  // rehash existing
  if (st->slots) {
    free(st->slots);
    st->slots = NULL;
    st->slots_cap = 0;
  }

  st->slots = new_slots;
  st->slots_cap = need;

  for (u32 i = 0; i < st->cnt; i++) {
    raf_store_rec_t *r = &st->recs[i];
    if (!r->path) continue;
    u32 mask = st->slots_cap - 1u;
    u32 pos = (u32)r->hash & mask;
    while (st->slots[pos] != 0u) pos = (pos + 1u) & mask;
    st->slots[pos] = i + 1u;
  }
}

static raf_store_rec_t* find_slot(const raf_store_t *st, const char *path, u64 hash, u32 *slot_out) {
  if (!st || !st->slots || st->slots_cap == 0) return NULL;
  u32 mask = st->slots_cap - 1u;
  u32 pos = (u32)hash & mask;
  for (;;) {
    u32 v = st->slots[pos];
    if (v == 0u) { if (slot_out) *slot_out = pos; return NULL; }
    u32 idx = v - 1u;
    if (idx < st->cnt) {
      raf_store_rec_t *r = &st->recs[idx];
      if (r->hash == hash && r->path && strcmp(r->path, path) == 0) {
        if (slot_out) *slot_out = pos;
        return r;
      }
    }
    pos = (pos + 1u) & mask;
  }
}

int raf_store_get(const raf_store_t *st, const char *path, u64 mtime_ns, u64 size, u8 *kind_out) {
  if (!st || !path || !st->slots) return 0;
  u64 h = raf_fnv1a64(path);
  raf_store_rec_t *r = find_slot(st, path, h, NULL);
  if (!r) return 0;
  if (r->mtime_ns != mtime_ns) return 0;
  if (r->size != size) return 0;
  if (kind_out) *kind_out = r->kind;
  return 1;
}

int raf_store_put(raf_store_t *st, const char *path, u64 mtime_ns, u64 size, u8 kind) {
  if (!st || !path) return 0;
  u64 h = raf_fnv1a64(path);

  ensure_slots(st, st->cnt + 1u);

  u32 slot = 0;
  raf_store_rec_t *r = find_slot(st, path, h, &slot);
  if (r) {
    r->mtime_ns = mtime_ns;
    r->size = size;
    r->kind = kind;
    return 1;
  }

  if (st->cnt == st->cap) {
    u32 ncap = st->cap ? st->cap * 2u : 256u;
    st->recs = (raf_store_rec_t*)raf_xrealloc(st->recs, (size_t)ncap * sizeof(raf_store_rec_t));
    memset(st->recs + st->cap, 0, (size_t)(ncap - st->cap) * sizeof(raf_store_rec_t));
    st->cap = ncap;
    // slots might need rebuild because recs moved
    ensure_slots(st, st->cnt + 1u);
    // recompute slot for this insert
    slot = 0;
    find_slot(st, path, h, &slot);
  }

  raf_store_rec_t *nr = &st->recs[st->cnt];
  nr->hash = h;
  nr->mtime_ns = mtime_ns;
  nr->size = size;
  nr->kind = kind;
  nr->path = raf_xstrdup(path);
  nr->path_len = (u16)strlen(path);

  // place in hash table
  if (st->slots && st->slots_cap) st->slots[slot] = st->cnt + 1u;
  st->cnt++;
  return 1;
}

static int read_full(int fd, void *buf, size_t n) {
  unsigned char *p = (unsigned char*)buf;
  size_t off = 0;
  while (off < n) {
    ssize_t r = read(fd, p + off, n - off);
    if (r == 0) return 0;
    if (r < 0) return 0;
    off += (size_t)r;
  }
  return 1;
}

static int write_full(int fd, const void *buf, size_t n) {
  const unsigned char *p = (const unsigned char*)buf;
  size_t off = 0;
  while (off < n) {
    ssize_t w = write(fd, p + off, n - off);
    if (w < 0) return 0;
    off += (size_t)w;
  }
  return 1;
}

int raf_store_load(raf_store_t *st, const char *path) {
  if (!st) return 0;
  raf_store_free(st);
  raf_store_init(st);
  if (!path || !path[0]) return 1; // no cache path

  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) return 1; // cache miss is not fatal

  struct {
    u32 magic;
    u32 version;
    u32 count;
    u32 reserved;
  } hdr;

  if (!read_full(fd, &hdr, sizeof(hdr))) { close(fd); return 0; }
  if (hdr.magic != RAFSTORE_MAGIC || hdr.version != 1u) { close(fd); return 0; }

  if (hdr.count > 2000000u) { close(fd); return 0; } // sanity cap

  st->recs = (raf_store_rec_t*)calloc((size_t)hdr.count, sizeof(raf_store_rec_t));
  if (!st->recs) { close(fd); return 0; }
  st->cap = hdr.count;

  for (u32 i = 0; i < hdr.count; i++) {
    raf_store_rec_t *r = &st->recs[i];
    if (!read_full(fd, &r->hash, sizeof(u64))) { close(fd); raf_store_free(st); return 0; }
    if (!read_full(fd, &r->mtime_ns, sizeof(u64))) { close(fd); raf_store_free(st); return 0; }
    if (!read_full(fd, &r->size, sizeof(u64))) { close(fd); raf_store_free(st); return 0; }
    if (!read_full(fd, &r->kind, sizeof(u8))) { close(fd); raf_store_free(st); return 0; }
    if (!read_full(fd, &r->path_len, sizeof(u16))) { close(fd); raf_store_free(st); return 0; }
    if (r->path_len > 8192u) { close(fd); raf_store_free(st); return 0; }
    r->path = (char*)malloc((size_t)r->path_len + 1u);
    if (!r->path) { close(fd); raf_store_free(st); return 0; }
    if (!read_full(fd, r->path, r->path_len)) { close(fd); raf_store_free(st); return 0; }
    r->path[r->path_len] = 0;
    st->cnt++;
  }

  close(fd);
  ensure_slots(st, st->cnt + 1u);
  return 1;
}

int raf_store_save(const raf_store_t *st, const char *path) {
  if (!st || !path || !path[0]) return 0;

  // write to tmp then rename
  char tmp[4096];
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);

  int fd = open(tmp, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0644);
  if (fd < 0) return 0;

  struct {
    u32 magic;
    u32 version;
    u32 count;
    u32 reserved;
  } hdr;
  hdr.magic = RAFSTORE_MAGIC;
  hdr.version = 1u;
  hdr.count = st->cnt;
  hdr.reserved = 0u;

  if (!write_full(fd, &hdr, sizeof(hdr))) { close(fd); return 0; }

  for (u32 i = 0; i < st->cnt; i++) {
    const raf_store_rec_t *r = &st->recs[i];
    if (!r->path) continue;
    u16 len = (u16)strlen(r->path);
    if (!write_full(fd, &r->hash, sizeof(u64))) { close(fd); return 0; }
    if (!write_full(fd, &r->mtime_ns, sizeof(u64))) { close(fd); return 0; }
    if (!write_full(fd, &r->size, sizeof(u64))) { close(fd); return 0; }
    if (!write_full(fd, &r->kind, sizeof(u8))) { close(fd); return 0; }
    if (!write_full(fd, &len, sizeof(u16))) { close(fd); return 0; }
    if (!write_full(fd, r->path, len)) { close(fd); return 0; }
  }

  fsync(fd);
  close(fd);

  if (rename(tmp, path) != 0) return 0;
  return 1;
}
