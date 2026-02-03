#include "rmr_compat.h"

static struct RMR_API g_api;

void rmr_bind_api(const struct RMR_API *api){ if(api) g_api=*api; }

void rmr_memcpy(void *dst, const void *src, rmr_u32 n){
  rmr_u8 *d=(rmr_u8*)dst; const rmr_u8 *s=(const rmr_u8*)src;
  for (rmr_u32 i=0;i<n;i++) d[i]=s[i];
}

void rmr_memset(void *dst, rmr_u8 v, rmr_u32 n){
  rmr_u8 *d=(rmr_u8*)dst;
  for (rmr_u32 i=0;i<n;i++) d[i]=v;
}

rmr_u32 rmr_strlen(const char *s){
  if (!s) return 0u;
  rmr_u32 n=0; while(s[n]) n++; return n;
}

int rmr_memeq(const rmr_u8 *a, const rmr_u8 *b, rmr_u32 n){
  for (rmr_u32 i=0;i<n;i++) if (a[i]!=b[i]) return 0;
  return 1;
}

rmr_u32 rmr_write(const rmr_u8 *buf, rmr_u32 len){
  if (!g_api.write) return 0u;
  return g_api.write(g_api.ctx, buf, len);
}

void rmr_puts(const char *s){ rmr_write((const rmr_u8*)s, rmr_strlen(s)); }

static rmr_u8 hx(rmr_u32 v){ v&=0xFu; return (rmr_u8)(v<10u?('0'+v):('a'+(v-10u))); }

void rmr_u32_to_hex(char out8[8], rmr_u32 v){
  out8[0]=hx(v>>28); out8[1]=hx(v>>24);
  out8[2]=hx(v>>20); out8[3]=hx(v>>16);
  out8[4]=hx(v>>12); out8[5]=hx(v>>8);
  out8[6]=hx(v>>4);  out8[7]=hx(v>>0);
}

void rmr_assert(int cond, const char *msg){
  if (cond) return;
  if (g_api.panic) g_api.panic(g_api.ctx, msg?msg:"RMR_ASSERT");
  for(;;){}
}
