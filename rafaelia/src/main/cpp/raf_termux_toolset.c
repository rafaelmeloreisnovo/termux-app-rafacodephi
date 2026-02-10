/* raf_termux_toolset.c
   Tabela deterministica de 100 ferramentas Termux (sem libc, sem dependencias)
*/

#include "raf_termux_toolset.h"

static const char *g_tool_names[RAF_TOOLSET_MAX] = {
  "bash",
  "sh",
  "dash",
  "zsh",
  "fish",
  "coreutils",
  "busybox",
  "toybox",
  "curl",
  "wget",
  "openssl",
  "ca-certificates",
  "git",
  "vim",
  "nano",
  "neovim",
  "less",
  "grep",
  "sed",
  "gawk",
  "diffutils",
  "findutils",
  "tar",
  "gzip",
  "bzip2",
  "xz",
  "zstd",
  "zip",
  "unzip",
  "p7zip",
  "rsync",
  "openssh",
  "openssh-sftp-server",
  "tmux",
  "screen",
  "htop",
  "procps",
  "psmisc",
  "lsof",
  "strace",
  "ltrace",
  "gdb",
  "clang",
  "gcc",
  "make",
  "cmake",
  "ninja",
  "pkg-config",
  "python",
  "python-pip",
  "perl",
  "ruby",
  "nodejs",
  "lua",
  "sqlite",
  "sqlite-dev",
  "libffi",
  "zlib",
  "liblz4",
  "liblzma",
  "libpng",
  "libjpeg-turbo",
  "libtiff",
  "libwebp",
  "libxml2",
  "libxslt",
  "ncurses",
  "readline",
  "termux-tools",
  "inetutils",
  "net-tools",
  "iproute2",
  "iptables",
  "tcpdump",
  "nmap",
  "dnsutils",
  "bind-tools",
  "dropbear",
  "aria2",
  "libcurl",
  "ffmpeg",
  "imagemagick",
  "sox",
  "pulseaudio",
  "termux-exec",
  "termux-api",
  "termux-elf-cleaner",
  "termux-apt-repo",
  "termux-chroot",
  "busybox-static",
  "proot",
  "proot-distro",
  "tsu",
  "clang-tools",
  "llvm",
  "libc++",
  "libc++-dev",
  "libandroid-support",
  "debootstrap",
  "qemu-user-static"
};

static const u16 g_tool_lens[RAF_TOOLSET_MAX] = {
  4u, 2u, 4u, 3u, 4u, 9u, 7u, 6u, 4u, 4u,
  7u, 15u, 3u, 3u, 4u, 6u, 4u, 4u, 3u, 4u,
  9u, 9u, 3u, 4u, 5u, 2u, 4u, 3u, 5u, 5u,
  5u, 7u, 19u, 4u, 6u, 4u, 6u, 6u, 4u, 6u,
  6u, 3u, 5u, 3u, 4u, 5u, 5u, 10u, 6u, 10u,
  4u, 4u, 6u, 3u, 6u, 10u, 6u, 4u, 6u, 7u,
  6u, 13u, 7u, 7u, 7u, 7u, 7u, 8u, 12u, 9u,
  9u, 8u, 8u, 7u, 4u, 8u, 10u, 8u, 5u, 7u,
  6u, 11u, 3u, 10u, 11u, 10u, 18u, 15u, 13u, 14u,
  5u, 12u, 3u, 11u, 4u, 6u, 10u, 18u, 11u, 16u
};

static const u16 g_tool_flags[RAF_TOOLSET_MAX] = {
  RAF_TOOL_FLAG_SHELL,
  RAF_TOOL_FLAG_SHELL,
  RAF_TOOL_FLAG_SHELL,
  RAF_TOOL_FLAG_SHELL,
  RAF_TOOL_FLAG_SHELL,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_LANG,
  RAF_TOOL_FLAG_LANG,
  RAF_TOOL_FLAG_LANG,
  RAF_TOOL_FLAG_LANG,
  RAF_TOOL_FLAG_LANG,
  RAF_TOOL_FLAG_LANG,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_NET,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_APP,
  RAF_TOOL_FLAG_APP,
  RAF_TOOL_FLAG_APP,
  RAF_TOOL_FLAG_APP,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_APP,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_BUILD,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_LIB,
  RAF_TOOL_FLAG_SYSTEM,
  RAF_TOOL_FLAG_SYSTEM
};

static u32 g_tool_ids[RAF_TOOLSET_MAX];
static u8 g_tool_init = 0u;

static u32 RmR_hash_token(const u8 *buf, u32 len){
  u32 h = 2166136261u;
  for(u32 i=0u;i<len;i++){
    h ^= (u32)buf[i];
    h *= 16777619u;
  }
  return h;
}

static u8 RmR_str_eq_n(const u8 *a, u32 a_len, const u8 *b, u32 b_len){
  if(a_len != b_len) return 0u;
  for(u32 i=0u;i<a_len;i++){
    if(a[i] != b[i]) return 0u;
  }
  return 1u;
}

void RmR_toolset_init(void){
  if(g_tool_init) return;
  for(u32 i=0u;i<RAF_TOOLSET_MAX;i++){
    g_tool_ids[i] = RmR_hash_token((const u8*)g_tool_names[i], (u32)g_tool_lens[i]);
  }
  g_tool_init = 1u;
}

u32 RmR_toolset_count(void){
  RmR_toolset_init();
  return RAF_TOOLSET_MAX;
}

const char *RmR_toolset_name_at(u32 idx, u32 *len){
  RmR_toolset_init();
  if(idx >= RAF_TOOLSET_MAX) return (const char*)0;
  if(len) *len = (u32)g_tool_lens[idx];
  return g_tool_names[idx];
}

u16 RmR_toolset_flags_at(u32 idx){
  RmR_toolset_init();
  if(idx >= RAF_TOOLSET_MAX) return 0u;
  return g_tool_flags[idx];
}

u32 RmR_toolset_id_at(u32 idx){
  RmR_toolset_init();
  if(idx >= RAF_TOOLSET_MAX) return 0u;
  return g_tool_ids[idx];
}

u32 RmR_toolset_find(const u8 *name, u32 len, u32 *out_idx){
  RmR_toolset_init();
  if(!name || len == 0u) return 0u;
  for(u32 i=0u;i<RAF_TOOLSET_MAX;i++){
    if(RmR_str_eq_n(name, len, (const u8*)g_tool_names[i], (u32)g_tool_lens[i])){
      if(out_idx) *out_idx = i;
      return 1u;
    }
  }
  return 0u;
}
