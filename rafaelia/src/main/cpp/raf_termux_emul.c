/* raf_termux_emul.c
   Emulacao deterministica de ferramentas Termux (sem libc, sem dependencias)
*/

#include "raf_termux_emul.h"
#include "raf_termux_toolset.h"
#include "raf_termux_packages.h"
#include "raf_termux_exec.h"

static u32 RmR_write(const struct RAF_EMU_IO *io, const u8 *buf, u32 len){
  if(io && io->write) return io->write(io->ctx, buf, len);
  return 0u;
}

static void RmR_pkg_install(raf_termux_emu_t *emu, const u8 *name, u32 name_len);

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

static void RmR_write_u32(const struct RAF_EMU_IO *io, u32 v){
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

static u32 RmR_u32le_4(const u8 *p){
  return ((u32)p[0]) | ((u32)p[1] << 8u) | ((u32)p[2] << 16u) | ((u32)p[3] << 24u);
}

static u8 RmR_cmd_eq4(const u8 *t, u32 len, u32 le4){
  if(len != 4u || !t) return 0u;
  return (RmR_u32le_4(t) == le4) ? 1u : 0u;
}

static u8 RmR_cmd_eq5(const u8 *t, u32 len, const u8 a, const u8 b, const u8 c, const u8 d, const u8 e){
  if(len != 5u || !t) return 0u;
  return (t[0]==a && t[1]==b && t[2]==c && t[3]==d && t[4]==e) ? 1u : 0u;
}


static u32 RmR_hash_token(const u8 *buf, u32 len){
  u32 h = 0x811C9DC5u;
  for(u32 i=0u;i<len;i++){
    h ^= (u32)buf[i];
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __asm__ volatile(
      "imull $0x01000193, %0"
      : "+r"(h)
      :
      : "cc"
    );
#else
    h *= 0x01000193u;
#endif
  }
  return h;
}

static u8 RmR_pkg_exists_catalog(const u8 *name, u32 name_len, u32 *id_out){
  const raf_termux_pkg_table_t *tb = RmR_termux_packages();
  if(!tb || !name || name_len == 0u) return 0u;
  u32 id = RmR_hash_token(name, name_len);
  for(u32 i=0u;i<tb->count;i++){
    if(tb->entries[i].id == id && (u32)tb->entries[i].name_len == name_len){
      if(id_out) *id_out = id;
      return 1u;
    }
  }
  return 0u;
}

void RmR_emul_init(raf_termux_emu_t *emu, const struct RAF_EMU_IO *io, u32 arch_id, u8 bus_bits){
  if(!emu) return;
  RmR_toolset_init();
  emu->io = *io;
  emu->exec.exec = 0;
  emu->exec.ctx = 0;
  emu->arch_id = arch_id;
  emu->bus_bits = bus_bits;
  emu->_pad0 = 0u;
  emu->seed = 0x0B17AFu;
  emu->last_status = 0u;
  emu->package_count = 0u;
  {
    u32 count = RmR_toolset_count();
    for(u32 i=0u;i<count && i<128u;i++){
      emu->packages[i] = RmR_toolset_id_at(i);
      emu->package_count++;
    }
  }
}

void RmR_emul_bind_exec(raf_termux_emu_t *emu, const struct RAF_EMU_EXEC *ex){
  if(!emu) return;
  if(ex){
    emu->exec = *ex;
  } else {
    emu->exec.exec = 0;
    emu->exec.ctx = 0;
  }
}

static u32 RmR_emul_termux_exec_cb(void *ctx, const u8 *name, u32 len){
  raf_exec_result_t out;
  (void)ctx;
  return RmR_exec_host_termux(name, len, &out);
}

u32 RmR_emul_bind_termux_exec(raf_termux_emu_t *emu){
  struct RAF_EMU_EXEC ex;
  if(!emu) return 1u;
  ex.exec = RmR_emul_termux_exec_cb;
  ex.ctx = 0;
  emu->exec = ex;
  return 0u;
}

static void RmR_emit_help(const struct RAF_EMU_IO *io){
  RmR_write_literal(io, "raf_termux_emu: comandos: help, echo, pkg, uname, stat, hw, bootstrap, proot\n");
  RmR_write_literal(io, "pkg list | pkg install <nome> | pkg exec <nome>\n");
  RmR_write_literal(io, "tools list | tools info <nome>\n");
  RmR_write_literal(io, "bootstrap init | proot init\n");
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
  RmR_write_u32(io, emu->last_status);
  RmR_write_literal(io, " pkg=");
  RmR_write_u32(io, emu->package_count);
  RmR_write_literal(io, " tools=");
  RmR_write_u32(io, RmR_toolset_count());
  RmR_write_literal(io, "\n");
}


static void RmR_emit_hw(const raf_termux_emu_t *emu){
  const struct RAF_EMU_IO *io = &emu->io;
  RmR_write_literal(io, "arch=");
  if(emu->arch_id == 1u) RmR_write_literal(io, "x86_64");
  else if(emu->arch_id == 2u) RmR_write_literal(io, "x86");
  else if(emu->arch_id == 3u) RmR_write_literal(io, "arm64");
  else if(emu->arch_id == 4u) RmR_write_literal(io, "arm");
  else if(emu->arch_id == 5u) RmR_write_literal(io, "riscv64");
  else if(emu->arch_id == 6u) RmR_write_literal(io, "riscv32");
  else RmR_write_literal(io, "unknown");
  RmR_write_literal(io, " bus=");
  if(emu->bus_bits == 64u) RmR_write_literal(io, "64");
  else if(emu->bus_bits == 32u) RmR_write_literal(io, "32");
  else RmR_write_literal(io, "?");
  RmR_write_literal(io, " seed=");
  RmR_write_u32(io, emu->seed);
  RmR_write_literal(io, "\n");
}

static u32 RmR_emit_bootstrap(raf_termux_emu_t *emu){
  const u8 apt[] = { 'a','p','t' };
  const u8 deb[] = { 'd','e','b','o','o','t','s','t','r','a','p' };
  const u8 proot[] = { 'p','r','o','o','t' };
  RmR_pkg_install(emu, apt, 3u);
  if(emu->last_status != 0u) return emu->last_status;
  RmR_pkg_install(emu, deb, 11u);
  if(emu->last_status != 0u) return emu->last_status;
  RmR_pkg_install(emu, proot, 5u);
  if(emu->last_status != 0u) return emu->last_status;
  RmR_write_literal(&emu->io, "bootstrap=ready\n");
  return 0u;
}

static u32 RmR_emit_proot(raf_termux_emu_t *emu){
  const u8 proot[] = { 'p','r','o','o','t' };
  const u8 distro[] = { 'p','r','o','o','t','-','d','i','s','t','r','o' };
  const u8 qemu[] = { 'q','e','m','u','-','u','s','e','r','-','s','t','a','t','i','c' };
  RmR_pkg_install(emu, proot, 5u);
  if(emu->last_status != 0u) return emu->last_status;
  RmR_pkg_install(emu, distro, 12u);
  if(emu->last_status != 0u) return emu->last_status;
  RmR_pkg_install(emu, qemu, 16u);
  if(emu->last_status != 0u) return emu->last_status;
  RmR_write_literal(&emu->io, "proot=ready\n");
  return 0u;
}

static u8 RmR_tool_name_by_id(u32 id, const char **name, u32 *len){
  u32 count = RmR_toolset_count();
  for(u32 i=0u;i<count;i++){
    if(RmR_toolset_id_at(i) == id){
      if(name) *name = RmR_toolset_name_at(i, len);
      return 1u;
    }
  }
  return 0u;
}

static void RmR_emit_pkg_list(const raf_termux_emu_t *emu){
  const struct RAF_EMU_IO *io = &emu->io;
  for(u32 i=0u;i<emu->package_count;i++){
    u32 v = emu->packages[i];
    const char *name = 0;
    u32 name_len = 0u;
    if(RmR_tool_name_by_id(v, &name, &name_len) && name){
      (void)RmR_write(io, (const u8*)name, name_len);
      RmR_write_literal(io, " ");
    }
    RmR_write_u32(io, v);
    RmR_write_literal(io, "\n");
  }
}

static void RmR_emit_tool_list(const raf_termux_emu_t *emu){
  const struct RAF_EMU_IO *io = &emu->io;
  u32 count = RmR_toolset_count();
  for(u32 i=0u;i<count;i++){
    u32 name_len = 0u;
    const char *name = RmR_toolset_name_at(i, &name_len);
    if(name && name_len){
      (void)RmR_write(io, (const u8*)name, name_len);
      RmR_write_literal(io, "\n");
    }
  }
  (void)emu;
}

static void RmR_emit_tool_info(const raf_termux_emu_t *emu, const u8 *name, u32 name_len){
  u32 idx = 0u;
  if(!RmR_toolset_find(name, name_len, &idx)){
    RmR_write_literal(&emu->io, "noent\n");
    return;
  }
  {
    u32 nlen = 0u;
    const char *nm = RmR_toolset_name_at(idx, &nlen);
    if(nm){
      RmR_write_literal(&emu->io, "name=");
      (void)RmR_write(&emu->io, (const u8*)nm, nlen);
      RmR_write_literal(&emu->io, " id=");
      RmR_write_u32(&emu->io, RmR_toolset_id_at(idx));
      RmR_write_literal(&emu->io, " flags=");
      RmR_write_u32(&emu->io, (u32)RmR_toolset_flags_at(idx));
      RmR_write_literal(&emu->io, "\n");
    }
  }
}

static u32 RmR_emit_tool_run(raf_termux_emu_t *emu, u32 idx){
  u32 name_len = 0u;
  const char *name = RmR_toolset_name_at(idx, &name_len);
  if(!name) return 1u;
  if(emu->exec.exec){
    u32 st = emu->exec.exec(emu->exec.ctx, (const u8*)name, name_len);
    emu->last_status = st;
    return st;
  }
  RmR_write_literal(&emu->io, "run ");
  (void)RmR_write(&emu->io, (const u8*)name, name_len);
  RmR_write_literal(&emu->io, " id=");
  RmR_write_u32(&emu->io, RmR_toolset_id_at(idx));
  RmR_write_literal(&emu->io, "\n");
  emu->last_status = 0u;
  return 0u;
}

static u8 RmR_pkg_has(const raf_termux_emu_t *emu, u32 id){
  for(u32 i=0u;i<emu->package_count;i++){
    if(emu->packages[i] == id) return 1u;
  }
  return 0u;
}

static void RmR_pkg_install(raf_termux_emu_t *emu, const u8 *name, u32 name_len){
  u32 id = 0u;
  u32 idx = 0u;
  if(RmR_toolset_find(name, name_len, &idx)){
    id = RmR_toolset_id_at(idx);
  } else if(!RmR_pkg_exists_catalog(name, name_len, &id)){
    emu->last_status = 1u;
    RmR_write_literal(&emu->io, "noent\n");
    return;
  }

  if(RmR_pkg_has(emu, id)){
    emu->last_status = 0u;
    RmR_write_literal(&emu->io, "ok\n");
    return;
  }
  if(emu->package_count < 128u){
    emu->packages[emu->package_count++] = id;
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

  if((RmR_str_eq_n(t0, t0_len, (const u8*)"sh", 2u) || RmR_str_eq_n(t0, t0_len, (const u8*)"bash", 4u))
     && t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"-c", 2u)){
    pos = RmR_skip_space(cmd, len, pos + t1_len);
    if(pos >= len){
      emu->last_status = 1u;
      RmR_write_literal(&emu->io, "missing\n");
      return 1u;
    }
    return RmR_emul_exec(emu, &cmd[pos], len - pos);
  }

  if(RmR_cmd_eq4(t0, t0_len, 0x706c6568u)){
    RmR_emit_help(&emu->io);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_cmd_eq4(t0, t0_len, 0x6f686365u)){
    pos = RmR_skip_space(cmd, len, pos);
    if(pos < len){
      (void)RmR_write(&emu->io, &cmd[pos], len - pos);
    }
    RmR_write_literal(&emu->io, "\n");
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_cmd_eq5(t0, t0_len, (u8)'u', (u8)'n', (u8)'a', (u8)'m', (u8)'e')){
    RmR_emit_uname(emu);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_cmd_eq4(t0, t0_len, 0x74617473u)){
    RmR_emit_stat(emu);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"hw", 2u)){
    RmR_emit_hw(emu);
    emu->last_status = 0u;
    return 0u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"bootstrap", 9u)){
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"init", 4u)){
      u32 st = RmR_emit_bootstrap(emu);
      emu->last_status = st;
      return st;
    }
    emu->last_status = 1u;
    RmR_write_literal(&emu->io, "bootstrap?\n");
    return 1u;
  }
  if(RmR_cmd_eq5(t0, t0_len, (u8)'p', (u8)'r', (u8)'o', (u8)'o', (u8)'t')){
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"init", 4u)){
      u32 st = RmR_emit_proot(emu);
      emu->last_status = st;
      return st;
    }
    emu->last_status = 1u;
    RmR_write_literal(&emu->io, "proot?\n");
    return 1u;
  }
  if(RmR_str_eq_n(t0, t0_len, (const u8*)"pkg", 3u)){
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"list", 4u)){
      RmR_emit_pkg_list(emu);
      emu->last_status = 0u;
      return 0u;
    }
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"exec", 4u)){
      pos = RmR_skip_space(cmd, len, pos + t1_len);
      u32 name_len = RmR_token_len(cmd, len, pos);
      if(name_len == 0u){
        emu->last_status = 1u;
        RmR_write_literal(&emu->io, "missing\n");
        return 1u;
      }
      {
        u32 idx = 0u;
        if(!RmR_toolset_find(&cmd[pos], name_len, &idx)){
          emu->last_status = 1u;
          RmR_write_literal(&emu->io, "noent\n");
          return 1u;
        }
        return RmR_emit_tool_run(emu, idx);
      }
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

  if(RmR_str_eq_n(t0, t0_len, (const u8*)"tools", 5u)){
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"list", 4u)){
      RmR_emit_tool_list(emu);
      emu->last_status = 0u;
      return 0u;
    }
    if(t1 && RmR_str_eq_n(t1, t1_len, (const u8*)"info", 4u)){
      pos = RmR_skip_space(cmd, len, pos + t1_len);
      u32 name_len = RmR_token_len(cmd, len, pos);
      if(name_len == 0u){
        emu->last_status = 1u;
        RmR_write_literal(&emu->io, "missing\n");
        return 1u;
      }
      RmR_emit_tool_info(emu, &cmd[pos], name_len);
      emu->last_status = 0u;
      return 0u;
    }
    emu->last_status = 1u;
    RmR_write_literal(&emu->io, "tools?\n");
    return 1u;
  }

  {
    u32 idx = 0u;
    if(RmR_toolset_find(t0, t0_len, &idx)){
      return RmR_emit_tool_run(emu, idx);
    }
  }

  emu->last_status = 127u;
  RmR_write_literal(&emu->io, "unknown\n");
  return 127u;
}
