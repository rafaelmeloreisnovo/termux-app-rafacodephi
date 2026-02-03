#ifndef RMR_COMPAT_H
#define RMR_COMPAT_H

typedef unsigned char      rmr_u8;
typedef unsigned short     rmr_u16;
typedef unsigned int       rmr_u32;
typedef unsigned long long rmr_u64;

#ifdef __cplusplus
extern "C" {
#endif

struct RMR_API {
  rmr_u32 (*write)(void *ctx, const rmr_u8 *buf, rmr_u32 len);
  void    (*panic)(void *ctx, const char *msg);
  void    *ctx;
};

void rmr_bind_api(const struct RMR_API *api);

void    rmr_memcpy(void *dst, const void *src, rmr_u32 n);
void    rmr_memset(void *dst, rmr_u8 v, rmr_u32 n);
rmr_u32 rmr_strlen(const char *s);
int     rmr_memeq(const rmr_u8 *a, const rmr_u8 *b, rmr_u32 n);

rmr_u32 rmr_write(const rmr_u8 *buf, rmr_u32 len);
void    rmr_puts(const char *s);

void    rmr_u32_to_hex(char out8[8], rmr_u32 v);
void    rmr_assert(int cond, const char *msg);

#ifdef __cplusplus
}
#endif
#endif
