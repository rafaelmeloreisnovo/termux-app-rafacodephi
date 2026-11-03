/* SPDX-License-Identifier: GPL-3.0-only
 * RAFAELIA RAFBOX v1 -- Linux/Android syscall adapter, ARMv7 + AArch64.
 * This is PLATFORM_GATE, not PURE_CORE and not a BusyBox replacement.
 */
#include "../proot_syscall_bridge.h"
#include "rafbox_core.h"

#define RAFBOX_STDIN 0
#define RAFBOX_STDOUT 1
#define RAFBOX_STDERR 2
static char rafbox_buffer[4096];

static int rafbox_write_all(int fd,const char *text,raf_word_t length) {
    while(length>0u) {
        raf_sysret_t sent=proot_sys_write(fd,text,length);
        if (sent==-4) continue; /* Linux EINTR */
        if (sent<=0 || (raf_word_t)sent>length) return 0;
        text+=(raf_word_t)sent;
        length-=(raf_word_t)sent;
    }
    return 1;
}
static int rafbox_out(const char *s) {
    unsigned int n=rafbox_strlen_bounded(s);
    return n<=RAFBOX_MAX_ARGUMENT_TEXT && rafbox_write_all(RAFBOX_STDOUT,s,n);
}
static int rafbox_err(const char *s) {
    unsigned int n=rafbox_strlen_bounded(s);
    return n<=RAFBOX_MAX_ARGUMENT_TEXT && rafbox_write_all(RAFBOX_STDERR,s,n);
}
static int rafbox_cat_fd(int fd) {
    for (;;) {
        raf_sysret_t got=proot_sys_read(fd,rafbox_buffer,sizeof(rafbox_buffer));
        if (got==-4) continue;
        if (got<0) return 0;
        if (got==0) return 1;
        if (!rafbox_write_all(RAFBOX_STDOUT,rafbox_buffer,(raf_word_t)got)) return 0;
    }
}
static int rafbox_main(int argc,char **argv) {
    int i;
    rafbox_span part;
    if (argc<2 || argc>128 || !argv) {rafbox_err("usage: rafbox --list | APPLET [args]\n");return 64;}
    if (rafbox_equals(argv[1],"--list")) {
        if (argc!=2) return 64;
        return rafbox_out("true\nfalse\necho\ncat\nbasename\ndirname\n")?0:74;
    }
    if (rafbox_equals(argv[1],"--help")) {
        if (argc!=2) return 64;
        return rafbox_out("RAFAELIA RAFBOX v1; subset only; no busybox or shell emulation\n")?0:74;
    }
    switch(rafbox_select(argv[1])) {
    case RAFBOX_TRUE: return argc==2?0:64;
    case RAFBOX_FALSE: return argc==2?1:64;
    case RAFBOX_ECHO:
        if (argc>2 && argv[2] && argv[2][0]=='-') return 64;
        for (i=2;i<argc;++i) {
            unsigned int n=rafbox_strlen_bounded(argv[i]);
            if(n>RAFBOX_MAX_ARGUMENT_TEXT) return 65;
            if(i>2 && !rafbox_write_all(RAFBOX_STDOUT," ",1u)) return 74;
            if(!rafbox_write_all(RAFBOX_STDOUT,argv[i],n)) return 74;
        }
        return rafbox_write_all(RAFBOX_STDOUT,"\n",1u)?0:74;
    case RAFBOX_CAT:
        if(argc==2) return rafbox_cat_fd(RAFBOX_STDIN)?0:74;
        for(i=2;i<argc;++i) {
            raf_sysret_t fd;
            if(rafbox_strlen_bounded(argv[i])>RAFBOX_MAX_ARGUMENT_TEXT) return 65;
            if(rafbox_equals(argv[i],"-")) {
                if(!rafbox_cat_fd(RAFBOX_STDIN)) return 74;
                continue;
            }
            fd=proot_sys_open(argv[i],0,0);
            if(fd<0) {rafbox_err("rafbox cat: cannot open input\n");return 66;}
            if(!rafbox_cat_fd((int)fd)) {proot_sys_close((int)fd);return 74;}
            if(proot_sys_close((int)fd)<0) return 74;
        }
        return 0;
    case RAFBOX_BASENAME:
        if(argc!=3 || !rafbox_basename_span(argv[2],&part)) return 64;
        if(!rafbox_write_all(RAFBOX_STDOUT,part.ptr,part.size)) return 74;
        return rafbox_write_all(RAFBOX_STDOUT,"\n",1u)?0:74;
    case RAFBOX_DIRNAME:
        if(argc!=3 || !rafbox_dirname_span(argv[2],&part)) return 64;
        if(!rafbox_write_all(RAFBOX_STDOUT,part.ptr,part.size)) return 74;
        return rafbox_write_all(RAFBOX_STDOUT,"\n",1u)?0:74;
    default:
        rafbox_err("RAFBOX_UNSUPPORTED_APPLET / TOKEN_VAZIO_NOT_IMPLEMENTED\n");
        return 64;
    }
}
__attribute__((noreturn,used,noinline))
void rafbox_entry(raf_word_t *sp) {
    int argc=(int)sp[0];
    char **argv=(char **)(sp+1);
    proot_sys_exit(rafbox_main(argc,argv));
}
#if defined(__aarch64__)
__attribute__((naked,noreturn,used)) void _start(void) {
    __asm__ volatile("mov x0, sp\n" "b rafbox_entry\n");
}
#elif defined(__arm__)
__attribute__((naked,noreturn,used)) void _start(void) {
    __asm__ volatile("mov r0, sp\n" "b rafbox_entry\n");
}
#endif
