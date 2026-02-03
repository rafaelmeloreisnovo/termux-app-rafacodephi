// file: rafpkg_util.c
// RAFAELIA :: tiny util (hosted)
// SPDX-License-Identifier: MIT

#include "rafpkg_util.h"

u32 raf_hash_fnv1a(const char *s, u32 len) {
  // 32-bit FNV-1a
  u32 h = 2166136261u;
  for (u32 i = 0; i < len; i++) {
    h ^= (u8)s[i];
    h *= 16777619u;
  }
  return h;
}

int raf_streq(const char *a, const char *b) {
  if (!a || !b) return 0;
  while (*a && *b) {
    if (*a != *b) return 0;
    a++; b++;
  }
  return (*a == 0 && *b == 0);
}

char *raf_trim(char *s) {
  if (!s) return s;
  // left
  while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
  // right
  char *e = s;
  while (*e) e++;
  while (e > s) {
    char c = e[-1];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { e--; continue; }
    break;
  }
  *e = 0;
  return s;
}
