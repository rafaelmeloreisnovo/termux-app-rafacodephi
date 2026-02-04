/* raf_termux_emul.h
   Emulacao deterministica de ferramentas Termux (sem libc, sem dependencias)
*/
#ifndef RAF_TERMUX_EMUL_H
#define RAF_TERMUX_EMUL_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

struct RAF_EMU_IO {
  u32 (*write)(void *ctx, const u8 *buf, u32 len);
  void *ctx;
};

typedef struct {
  struct RAF_EMU_IO io;
  u32 arch_id;
  u8  bus_bits;
  u8  _pad0;
  u32 seed;
  u32 last_status;
  u32 packages[16];
  u32 package_count;
} raf_termux_emu_t;

void RmR_emul_init(raf_termux_emu_t *emu, const struct RAF_EMU_IO *io, u32 arch_id, u8 bus_bits);
u32 RmR_emul_exec(raf_termux_emu_t *emu, const u8 *cmd, u32 len);

#endif
