#include <stdio.h>
#include "rmr_compat.h"

static rmr_u32 host_write(void *ctx, const rmr_u8 *buf, rmr_u32 len){
  (void)ctx;
  for (rmr_u32 i=0;i<len;i++) putchar((int)buf[i]);
  return len;
}
static void host_panic(void *ctx, const char *msg){
  (void)ctx;
  fprintf(stderr, "PANIC: %s\n", msg ? msg : "(null)");
}

int main(void){
  struct RMR_API api = { host_write, host_panic, 0 };
  rmr_bind_api(&api);
  rmr_puts("RMR_COMPAT_V2 OK\n");
  char hx8[8];
  rmr_u32_to_hex(hx8, 0xB2F14A7Fu);
  rmr_write((const rmr_u8*)hx8, 8);
  rmr_puts("\n");
  rmr_assert(rmr_strlen("abc")==3u, "strlen fail");
  return 0;
}
