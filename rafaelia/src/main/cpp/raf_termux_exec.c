/* raf_termux_exec.c
   Execucao real low-level (syscall direto, sem libc)
*/

#include "raf_termux_exec.h"

#if defined(__x86_64__)
#define RMR_NR_VFORK 58
#define RMR_NR_EXECVE 59
#define RMR_NR_EXIT 60
#define RMR_NR_WAIT4 61
#elif defined(__i386__)
#define RMR_NR_VFORK 190
#define RMR_NR_EXECVE 11
#define RMR_NR_EXIT 1
#define RMR_NR_WAIT4 114
#elif defined(__aarch64__)
#define RMR_NR_VFORK 190
#define RMR_NR_EXECVE 221
#define RMR_NR_EXIT 93
#define RMR_NR_WAIT4 260
#elif defined(__arm__)
#define RMR_NR_VFORK 190
#define RMR_NR_EXECVE 11
#define RMR_NR_EXIT 1
#define RMR_NR_WAIT4 114
#elif defined(__riscv)
#define RMR_NR_VFORK 220
#define RMR_NR_EXECVE 221
#define RMR_NR_EXIT 93
#define RMR_NR_WAIT4 260
#else
#define RMR_NR_VFORK 0
#define RMR_NR_EXECVE 0
#define RMR_NR_EXIT 0
#define RMR_NR_WAIT4 0
#endif

#if defined(__x86_64__)
static long RmR_syscall0(long n){
  long r;
  __asm__ volatile("syscall" : "=a"(r) : "a"(n) : "rcx", "r11", "memory");
  return r;
}
static long RmR_syscall1(long n, long a){
  long r;
  __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a) : "rcx", "r11", "memory");
  return r;
}
static long RmR_syscall3(long n, long a, long b, long c){
  long r;
  __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
  return r;
}
static long RmR_syscall4(long n, long a, long b, long c, long d){
  long r;
  register long r10 __asm__("r10") = d;
  __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10) : "rcx", "r11", "memory");
  return r;
}
#elif defined(__aarch64__)
static long RmR_syscall0(long n){
  register long x8 __asm__("x8") = n;
  register long x0 __asm__("x0");
  __asm__ volatile("svc 0" : "=r"(x0) : "r"(x8) : "memory");
  return x0;
}
static long RmR_syscall1(long n, long a){
  register long x8 __asm__("x8") = n;
  register long x0 __asm__("x0") = a;
  __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8) : "memory");
  return x0;
}
static long RmR_syscall3(long n, long a, long b, long c){
  register long x8 __asm__("x8") = n;
  register long x0 __asm__("x0") = a;
  register long x1 __asm__("x1") = b;
  register long x2 __asm__("x2") = c;
  __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
  return x0;
}
static long RmR_syscall4(long n, long a, long b, long c, long d){
  register long x8 __asm__("x8") = n;
  register long x0 __asm__("x0") = a;
  register long x1 __asm__("x1") = b;
  register long x2 __asm__("x2") = c;
  register long x3 __asm__("x3") = d;
  __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
  return x0;
}
#elif defined(__riscv)
static long RmR_syscall0(long n){
  register long a7 __asm__("a7") = n;
  register long a0 __asm__("a0");
  __asm__ volatile("ecall" : "=r"(a0) : "r"(a7) : "memory");
  return a0;
}
static long RmR_syscall1(long n, long a){
  register long a7 __asm__("a7") = n;
  register long a0 __asm__("a0") = a;
  __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
  return a0;
}
static long RmR_syscall3(long n, long a, long b, long c){
  register long a7 __asm__("a7") = n;
  register long a0 __asm__("a0") = a;
  register long a1 __asm__("a1") = b;
  register long a2 __asm__("a2") = c;
  __asm__ volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2) : "memory");
  return a0;
}
static long RmR_syscall4(long n, long a, long b, long c, long d){
  register long a7 __asm__("a7") = n;
  register long a0 __asm__("a0") = a;
  register long a1 __asm__("a1") = b;
  register long a2 __asm__("a2") = c;
  register long a3 __asm__("a3") = d;
  __asm__ volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2), "r"(a3) : "memory");
  return a0;
}
#else
static long RmR_syscall0(long n){ (void)n; return -1; }
static long RmR_syscall1(long n, long a){ (void)n; (void)a; return -1; }
static long RmR_syscall3(long n, long a, long b, long c){ (void)n; (void)a; (void)b; (void)c; return -1; }
static long RmR_syscall4(long n, long a, long b, long c, long d){ (void)n; (void)a; (void)b; (void)c; (void)d; return -1; }
#endif

static u32 RmR_copy_path(const char *prefix, u32 prefix_len, const u8 *name, u32 name_len, char *out, u32 cap){
  u32 i = 0u;
  if(prefix_len + name_len + 1u > cap) return 0u;
  for(i=0u;i<prefix_len;i++) out[i] = prefix[i];
  for(u32 j=0u;j<name_len;j++) out[i++] = (char)name[j];
  out[i] = 0;
  return i;
}

static long RmR_execve_try(const char *path, const char **argv){
  const char *envp[1];
  envp[0] = 0;
  return RmR_syscall3(RMR_NR_EXECVE, (long)path, (long)argv, (long)envp);
}

u32 RmR_exec_host_termux(const u8 *name, u32 len, raf_exec_result_t *out){
#if defined(__x86_64__) || defined(__aarch64__) || defined(__i386__) || defined(__arm__) || defined(__riscv)
  char path[256];
  const char *argv[2];
  const char *prefixes[6];
  u32 prefix_lens[6];
  u32 prefix_count = 0u;
  prefixes[prefix_count] = "/data/data/com.termux.rafacodephi/files/usr/bin/";
  prefix_lens[prefix_count++] = (u32)(sizeof("/data/data/com.termux.rafacodephi/files/usr/bin/") - 1u);
  prefixes[prefix_count] = "/data/user/0/com.termux.rafacodephi/files/usr/bin/";
  prefix_lens[prefix_count++] = (u32)(sizeof("/data/user/0/com.termux.rafacodephi/files/usr/bin/") - 1u);
  prefixes[prefix_count] = "/data/data/com.termux/files/usr/bin/";
  prefix_lens[prefix_count++] = (u32)(sizeof("/data/data/com.termux/files/usr/bin/") - 1u);
  prefixes[prefix_count] = "/data/user/0/com.termux/files/usr/bin/";
  prefix_lens[prefix_count++] = (u32)(sizeof("/data/user/0/com.termux/files/usr/bin/") - 1u);
  prefixes[prefix_count] = "/system/bin/";
  prefix_lens[prefix_count++] = (u32)(sizeof("/system/bin/") - 1u);
  prefixes[prefix_count] = "/system/xbin/";
  prefix_lens[prefix_count++] = (u32)(sizeof("/system/xbin/") - 1u);
  long pid = RmR_syscall0(RMR_NR_VFORK);
  if(pid == 0){
    argv[0] = (const char*)name;
    argv[1] = 0;
    for(u32 i = 0u; i < prefix_count; i++){
      if(RmR_copy_path(prefixes[i], prefix_lens[i], name, len, path, 256u)){
        (void)RmR_execve_try(path, argv);
      }
    }
    (void)RmR_syscall1(RMR_NR_EXIT, 127);
  }
  if(pid < 0){
    if(out) out->status = 127u;
    return 127u;
  }
  {
    int status = 0;
    (void)RmR_syscall4(RMR_NR_WAIT4, pid, (long)&status, 0, 0);
    if(out) out->status = (u32)status;
    return (u32)status;
  }
#else
  if(out) out->status = 127u;
  (void)name;
  (void)len;
  return 127u;
#endif
}
