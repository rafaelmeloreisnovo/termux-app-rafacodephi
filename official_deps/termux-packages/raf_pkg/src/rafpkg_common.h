#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#ifndef RAF_MAX_LINE
#define RAF_MAX_LINE 4096
#endif

static inline int raf_is_dir(const char *path){
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  return S_ISDIR(st.st_mode);
}

static inline int raf_is_file(const char *path){
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  return S_ISREG(st.st_mode);
}

static inline void *raf_xmalloc(size_t n){
  void *p = malloc(n);
  if(!p){
    fprintf(stderr, "rafpkg: OOM (%zu)\n", n);
    exit(2);
  }
  return p;
}

static inline void *raf_xrealloc(void *p, size_t n){
  void *q = realloc(p, n);
  if(!q){
    fprintf(stderr, "rafpkg: OOM realloc (%zu)\n", n);
    exit(2);
  }
  return q;
}

static inline char *raf_xstrdup(const char *s){
  size_t n = strlen(s);
  char *p = (char*)raf_xmalloc(n+1);
  memcpy(p,s,n+1);
  return p;
}
