/* raf_termux_emul.c
   Emulacao deterministica de ferramentas Termux (sem libc, sem dependencias)
*/

#include "raf_termux_emul.h"

static u32 RmR_write(const struct RAF_EMU_IO *io, const u8 *buf, u32 len){
  if(io && io->write) return io->write(io->ctx, buf, len);
  return 0u;
}

static u8 RmR_is_space(u8 v){
  return (v == (u8)' ' || v == (u8)'\t' || v == (u8)'\n' || v == (u8)'\r') ? 1u : 0u;
}

static u32 RmR_skip_space(const u8 *buf, u32 len, u32 pos){
  while(pos < len && RmR_is_space(buf[pos])) pos++;
  return pos;
}

static u32 RmR_token_len(const u8 *buf, u32 len, u32 pos){
  u32 start = pos;
  while(pos < len && !RmR_is_space(buf[pos])) pos++;
  return pos - start;
}

static u8 RmR_str_eq_n(const u8 *a, u32 a_len, const u8 *b, u32 b_len){
  if(a_len != b_len) return 0u;
  for(u32 i=0u;i<a_len;i++){
    if(a[i] != b[i]) return 0u;
  }
  return 1u;
}

static void RmR_write_literal(const struct RAF_EMU_IO *io, const char *lit){
  const u8 *p = (const u8*)lit;
  u32 len = 0u;
  while(p[len] != 0) len++;
  (void)RmR_write(io, p, len);
}

static u32 RmR_hash_token(const u8 *buf, u32 len){
  u32 h = 2166136261u;
  for(u32 i=0u;i<len;i++){
    h ^= (u32)buf[i];
    h *= 16777619u;
  }
  return h;
}

void RmR_emul_init(raf_termux_emu_t *emu, const struct RAF_EMU_IO *io, u32 arch_id, u8 bus_bits){
  if(!emu) return;
  emu->io = *io;
  emu->arch_id = arch_id;
  emu->bus_bits = bus_bits;
  emu->_pad0 = 0u;
  emu->seed = 0xB17RAFu;
  emu->last_status = 0u;
  emu->package_count = 6u;
  emu->packages[0] = RmR_hash_token((const u8*)"coreutils", 9u);
  emu->packages[1] = RmR_hash_token((const u8*)"curl", 4u);
  emu->packages[2] = RmR_hash_token((const u8*)"openssl", 7u);
  emu->packages[3] = RmR_hash_token((const u8*)"busybox", 7u);
  emu->packages[4] = RmR_hash_token((const u8*)"vim", 3u);
  emu->packages[5] = RmR_hash_token((const u8*)"git", 3u);
}

static void RmR_emit_help(const struct RAF_EMU_IO *io){
  RmR_write_literal(io, "raf_termux_emu: comandos: help, echo, pkg, uname, stat\n");
  RmR_write_literal(io, "pkg list | pkg install <nome>\n");
}

static void RmR_emit_uname(const raf_termux_emu_t *emu){
  const struct RAF_EMU_IO *io = &emu->io;
  RmR_write_literal(io, "raf_os ");
  if(emu->arch_id == 1u) RmR_write_literal(io, "x86_64 ");
  else if(emu->arch_id == 2u) RmR_write_literal(io, "x86 ");
  else if(emu->arch_id == 3u) RmR_write_literal(io, "arm64 ");
  else if(emu->arch_id == 4u) RmR_write_literal(io, "arm ");
  else if(emu->arch_id == 5u) RmR_write_literal(io, "riscv64 ");
  else if(emu->arch_id == 6u) RmR_write_literal(io, "riscv32 ");
  else RmR_write_literal(io, "unknown ");
  RmR_write_literal(io, "bus=");
  if(emu->bus_bits == 64u) RmR_write_literal(io, "64\n");
  else if(emu->bus_bits == 32u) RmR_write_literal(io, "32\n");
  else RmR_write_literal(io, "?\n");
}

static void RmR_emit_stat(const raf_termux_emu_t *emu){
  const struct RAF_EMU_IO *io = &emu->io;
  RmR_write_literal(io, "status=");
  {
    u32 v = emu->last_status;
    u8 buf[10];
    u32 n = 0u;
    if(v == 0u){
      buf[n++] = (u8)'0';
    } else {
      u8 tmp[10];
      u32 t = 0u;
      while(v > 0u && t < 10u){
        tmp[t++] = (u8)('0' + (v % 10u));
        v /= 10u;
      }
      while(t > 0u) buf[n++] = tmp[--t];
    }
    (void)RmR_write(io, buf, n);
  }
  RmR_write_literal(io, " pkg=");
  {
    u32 v = emu->package_count;
    u8 buf[10];
    u32 n = 0u;
    if(v == 0u){
      buf[n++] = (u8)'0';
    } else {
      u8 tmp[10];
      u32 t = 0u;
      while(v > 0u && t < 10u){
        tmp[t++] = (u8)('0' + (v % 10u));
        v /= 10u;
      }
      while(t > 0u) buf[n++] = tmp[--t];
    }
    (void)RmR_write(io, buf, n);
  }
  RmR_write_literal(io, "\n");
}

static void RmR_emit_pkg_list(const raf_termux_emu_t *emu){
  const struct RAF_EMU_IO *io = &emu->io;
  for(u32 i=0u;i<emu->package_count;i++){
    u32 v = emu->packages[i];
    u8 buf[10];
    u32 n = 0u;
    if(v == 0u){
      buf[n++] = (u8)'0';
    } else {
      u8 tmp[10];
      u32 t = 0u;
      while(v > 0u && t < 10u){
        tmp[t++] = (u8)('0' + (v % 10u));
        v /= 10u;
      }
      while(t > 0u) buf[n++] = tmp[--t];
    }
    (void)RmR_write(io, buf, n);
    RmR_write_literal(io, "\n");
  }
}

static void RmR_pkg_install(raf_termux_emu_t *emu, const u8 *name, u32 name_len){
  u32 h = RmR_hash_token(name, name_len);
  if(emu->package_count < 16u){
    emu->packages[emu->package_count++] = h;
    emu->last_status = 0u;
    RmR_write_literal(&emu->io, "ok\n");
  } else {
    emu->last_status = 2u;
    RmR_write_literal(&emu->io, "full\n");
  }
}

u32 RmR_emul_exec(raf_termux_emu_t *emu, const u8 *cmd, u32 len){
  if(!emu || !cmd || len == 0u) return 1u;
  u32 pos = RmR_skip_space(cmd, len, 0u);
  if(pos >= len) return 1u;
  u32 t0_len = RmR_token_len(cmd, len, pos);
  const u8 *t0 = &cmd[pos];
  pos = RmR_skip_space(cmd, len, pos + t0_len);
  u32 t1_len = RmR_token_len(cmd, len, pos);
  const u8 *t1 = (t1_len > 0u) ? &cmd[pos] : (const u8*)0;

  if(RmR_str_eq_n(t0, t0_len, (const u8*)"help", 4u)){
    RmR_emit_help(&emu->io);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"echo", 4u)){
    pos = RmR_skip_space(cmd, len, pos);
    if(pos < len){
      (void)RmR_write(&emu->io, &cmd[pos], len - pos);
    }
    RmR_write_literal(&emu->io, "\n");
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"uname", 5u)){
    RmR_emit_uname(emu);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"stat", 4u)){
    RmR_emit_stat(emu);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"pkg", 3u)){
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"list", 4u)){
      RmR_emit_pkg_list(emu);
      emu->last_status = 0u;
      return 0u;
    }
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"install", 7u)){
      pos = RmR_skip_space(cmd, len, pos + t1_len);
      u32 name_len = RmR_token_len(cmd, len, pos);
      if(name_len == 0u){
        emu->last_status = 1u;
        RmR_write_literal(&emu->io, "missing\n");
        return 1u;
      }
      RmR_pkg_install(emu, &cmd[pos], name_len);
      return emu->last_status;
    }
    emu->last_status = 1u;
    RmR_write_literal(&emu->io, "pkg?\n");
    return 1u;
  }

  emu->last_status = 127u;
  RmR_write_literal(&emu->io, "unknown\n");
  return 127u;
}
