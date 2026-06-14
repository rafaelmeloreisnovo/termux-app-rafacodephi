/* apkc.c — freestanding APK compiler, no libc, no heap, no abstractions */
#include "sys.h"
#include "mem.h"
#include "arch_arm64.h"
#include "arch_arm32.h"
#include "fmt_zip.h"
#include "fmt_dex.h"
#include "fmt_axml.h"
#include "fmt_elf.h"

/* ── token kinds ─────────────────────────────────────────────────── */
typedef enum {
    TK_EOF=0, TK_NL, TK_IDENT, TK_INT, TK_STR, TK_COMMA,
    TK_LBRK, TK_RBRK, TK_BANG, TK_HASH, TK_COLON, TK_PLUS, TK_MINUS
} TKind;

typedef struct { const char *p; sz len; u64 ival; TKind kind; } Tok;
typedef struct { const char *s; const char *e; Tok cur; } Lex;

static int _is_sp(char c)  { return c==' '||c=='\t'||c=='\r'; }
static int _is_al(char c)  { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'||c=='.'||c=='@'; }
static int _is_dg(char c)  { return c>='0'&&c<='9'; }
static int _is_hex(char c) {
    return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
}
static u64 _hv(char c) {
    u64 d=(u64)(c-'0'); u64 la=(u64)(c-'a'+10); u64 ua=(u64)(c-'A'+10);
    u64 sd=(u64)((i64)(-(c>='0'&&c<='9'))&d);
    u64 sl=(u64)((i64)(-(c>='a'&&c<='f'))&la);
    u64 su=(u64)((i64)(-(c>='A'&&c<='F'))&ua);
    return sd|sl|su;
}

static void lex_skip(Lex *l) {
    while (l->s < l->e && _is_sp(*l->s)) l->s++;
    if (l->s < l->e && *l->s == ';') { while (l->s<l->e && *l->s!='\n') l->s++; }
    if (l->s < l->e && *l->s == '/') {
        if (l->s+1<l->e && l->s[1]=='/') { while (l->s<l->e && *l->s!='\n') l->s++; }
    }
}

static void lex_next(Lex *l) {
    lex_skip(l);
    Tok t; t.p=l->s; t.len=0; t.ival=0;
    if (l->s>=l->e) { t.kind=TK_EOF; l->cur=t; return; }
    char c=*l->s;
    if (c=='\n') { t.kind=TK_NL; t.len=1; l->s++; l->cur=t; return; }
    if (c==',') { t.kind=TK_COMMA; t.len=1; l->s++; l->cur=t; return; }
    if (c=='[') { t.kind=TK_LBRK; t.len=1; l->s++; l->cur=t; return; }
    if (c==']') { t.kind=TK_RBRK; t.len=1; l->s++; l->cur=t; return; }
    if (c=='!') { t.kind=TK_BANG; t.len=1; l->s++; l->cur=t; return; }
    if (c=='#') { t.kind=TK_HASH; t.len=1; l->s++; l->cur=t; return; }
    if (c==':') { t.kind=TK_COLON; t.len=1; l->s++; l->cur=t; return; }
    if (c=='+') { t.kind=TK_PLUS; t.len=1; l->s++; l->cur=t; return; }
    if (c=='-') {
        if (l->s+1<l->e && _is_dg(l->s[1])) {
            l->s++; u64 v=0;
            while (l->s<l->e && _is_dg(*l->s)) { v=v*10+(u64)(*l->s-'0'); l->s++; }
            t.ival=(u64)(-(i64)v); t.kind=TK_INT; t.len=(sz)(l->s-t.p); l->cur=t; return;
        }
        t.kind=TK_MINUS; t.len=1; l->s++; l->cur=t; return;
    }
    if (c=='0' && l->s+1<l->e && (l->s[1]=='x'||l->s[1]=='X')) {
        l->s+=2; u64 v=0;
        while (l->s<l->e && _is_hex(*l->s)) { v=(v<<4)|_hv(*l->s); l->s++; }
        t.ival=v; t.kind=TK_INT; t.len=(sz)(l->s-t.p); l->cur=t; return;
    }
    if (_is_dg(c)) {
        u64 v=0; while (l->s<l->e && _is_dg(*l->s)) { v=v*10+(u64)(*l->s-'0'); l->s++; }
        t.ival=v; t.kind=TK_INT; t.len=(sz)(l->s-t.p); l->cur=t; return;
    }
    if (_is_al(c)) {
        while (l->s<l->e && (_is_al(*l->s)||_is_dg(*l->s))) l->s++;
        t.kind=TK_IDENT; t.len=(sz)(l->s-t.p); l->cur=t; return;
    }
    if (c=='"') {
        l->s++; t.p=l->s;
        while (l->s<l->e && *l->s!='"') l->s++;
        t.kind=TK_STR; t.len=(sz)(l->s-t.p);
        if (l->s<l->e) l->s++;
        l->cur=t; return;
    }
    /* skip unknown */ t.kind=TK_EOF; l->s++; l->cur=t;
}

static int tok_eq(Tok t, const char *s) {
    sz n=0; while(s[n]) n++;
    if (t.len!=n) return 0;
    for (sz i=0;i<n;i++) if (t.p[i]!=s[i]) return 0;
    return 1;
}

/* case-insensitive ident compare */
static int tok_eqi(Tok t, const char *s) {
    sz n=0; while(s[n]) n++;
    if (t.len!=n) return 0;
    for (sz i=0;i<n;i++) {
        char a=t.p[i]; char b=s[i];
        char al=(a>='A'&&a<='Z')?(char)(a+32):a;
        char bl=(b>='A'&&b<='Z')?(char)(b+32):b;
        if (al!=bl) return 0;
    }
    return 1;
}

/* ── label / backpatch tables ─────────────────────────────────────── */
#define MAX_LBL 256
#define MAX_PAT 256

typedef struct { char name[80]; u32 off; } Lbl;
typedef struct { u32 insn_off; char tgt[80]; int arch; } Pat; /* arch: 64 or 32 */

static Lbl _lbls[MAX_LBL];
static u32 _nlbl;
static Pat _pats[MAX_PAT];
static u32 _npat;

static void lbl_reset(void) { _nlbl=0; _npat=0; }

static void tok_copy(char *dst, Tok t) {
    sz n=t.len<79?t.len:79;
    m_cpy(dst,(const u8*)t.p,n); dst[n]=0;
}

static i32 lbl_find(const char *nm) {
    for (u32 i=0;i<_nlbl;i++) {
        sz j=0; while (_lbls[i].name[j]&&nm[j]&&_lbls[i].name[j]==nm[j]) j++;
        if (!_lbls[i].name[j]&&!nm[j]) return (i32)i;
    }
    return -1;
}

static void lbl_def(const char *nm, u32 off) {
    i32 idx=lbl_find(nm);
    if (idx>=0) { _lbls[idx].off=off; return; }
    if (_nlbl>=MAX_LBL) return;
    sz j=0; while (nm[j]&&j<79) { _lbls[_nlbl].name[j]=nm[j]; j++; }
    _lbls[_nlbl].name[j]=0;
    _lbls[_nlbl].off=off;
    _nlbl++;
}

static void pat_add(u32 ioff, Tok tgt, int arch) {
    if (_npat>=MAX_PAT) return;
    _pats[_npat].insn_off=ioff;
    tok_copy(_pats[_npat].tgt, tgt);
    _pats[_npat].arch=arch;
    _npat++;
}

/* ── register parsers ─────────────────────────────────────────────── */
static i32 reg64(Tok t) {
    if (t.kind!=TK_IDENT) return -1;
    /* xN or wN */
    char lo=(t.p[0]>='A'&&t.p[0]<='Z')?(char)(t.p[0]+32):t.p[0];
    if ((lo=='x'||lo=='w') && t.len>=2) {
        u32 n=0; for(sz i=1;i<t.len;i++) n=n*10+(u32)(t.p[i]-'0');
        return (i32)n;
    }
    if (tok_eqi(t,"sp")) return 31;
    if (tok_eqi(t,"xzr")||tok_eqi(t,"wzr")) return 31;
    if (tok_eqi(t,"lr")) return 30;
    if (tok_eqi(t,"fp")) return 29;
    return -1;
}

static int reg64_sf(Tok t) { /* 0=w, 1=x */
    if (t.kind!=TK_IDENT||t.len<1) return 1;
    char lo=(t.p[0]>='A'&&t.p[0]<='Z')?(char)(t.p[0]+32):t.p[0];
    return lo=='w'?0:1;
}

static i32 reg32a(Tok t) {
    if (t.kind!=TK_IDENT) return -1;
    char lo=(t.p[0]>='A'&&t.p[0]<='Z')?(char)(t.p[0]+32):t.p[0];
    if (lo=='r' && t.len>=2) {
        u32 n=0; for(sz i=1;i<t.len;i++) n=n*10+(u32)(t.p[i]-'0');
        return (i32)n;
    }
    if (tok_eqi(t,"sp")) return A32_SP;
    if (tok_eqi(t,"lr")) return A32_LR;
    if (tok_eqi(t,"pc")) return A32_PC;
    return -1;
}

/* ── condition code parser ────────────────────────────────────────── */
static u32 parse_cc64(Tok t) {
    if (tok_eqi(t,"eq")) return CC_EQ;
    if (tok_eqi(t,"ne")) return CC_NE;
    if (tok_eqi(t,"cs")||tok_eqi(t,"hs")) return CC_CS;
    if (tok_eqi(t,"cc")||tok_eqi(t,"lo")) return CC_CC;
    if (tok_eqi(t,"mi")) return CC_MI;
    if (tok_eqi(t,"pl")) return CC_PL;
    if (tok_eqi(t,"vs")) return CC_VS;
    if (tok_eqi(t,"vc")) return CC_VC;
    if (tok_eqi(t,"hi")) return CC_HI;
    if (tok_eqi(t,"ls")) return CC_LS;
    if (tok_eqi(t,"ge")) return CC_GE;
    if (tok_eqi(t,"lt")) return CC_LT;
    if (tok_eqi(t,"gt")) return CC_GT;
    if (tok_eqi(t,"le")) return CC_LE;
    return CC_AL;
}

static u32 parse_cc32(Tok t) {
    if (tok_eqi(t,"eq")) return A32_EQ;
    if (tok_eqi(t,"ne")) return A32_NE;
    if (tok_eqi(t,"cs")||tok_eqi(t,"hs")) return A32_CS;
    if (tok_eqi(t,"cc")||tok_eqi(t,"lo")) return A32_CC;
    if (tok_eqi(t,"mi")) return A32_MI;
    if (tok_eqi(t,"pl")) return A32_PL;
    if (tok_eqi(t,"vs")) return A32_VS;
    if (tok_eqi(t,"vc")) return A32_VC;
    if (tok_eqi(t,"hi")) return A32_HI;
    if (tok_eqi(t,"ls")) return A32_LS;
    if (tok_eqi(t,"ge")) return A32_GE;
    if (tok_eqi(t,"lt")) return A32_LT;
    if (tok_eqi(t,"gt")) return A32_GT;
    if (tok_eqi(t,"le")) return A32_LE;
    return A32_AL;
}

/* skip optional # prefix and get integer */
static u64 lex_imm(Lex *l) {
    if (l->cur.kind==TK_HASH) lex_next(l);
    u64 v=l->cur.ival; lex_next(l); return v;
}

/* ── Emit context ─────────────────────────────────────────────────── */
typedef struct {
    u8 *buf; sz cap; sz pos;
    u32 sym1_va; u32 sym2_va;
    int has_sym1; int has_sym2;
} Emit;

static void emit32(Emit *e, u32 w) {
    if (e->pos+4>e->cap) return;
    w32(e->buf+e->pos,w); e->pos+=4;
}

/* ── ARM64 assembler ──────────────────────────────────────────────── */
static void asm_insn64(Emit *em, Tok mn, Lex *l) {
    lex_next(l); /* advance past mnemonic */
    u32 pos=(u32)em->pos;

    /* detect .sym1 / .sym2 markers */
    if (tok_eq(mn,".sym1")) { em->sym1_va=pos; em->has_sym1=1; return; }
    if (tok_eq(mn,".sym2")) { em->sym2_va=pos; em->has_sym2=1; return; }

    if (tok_eqi(mn,"nop"))  { emit32(em,A64_NOP); return; }
    if (tok_eqi(mn,"ret"))  { emit32(em,A64_RET); return; }
    if (tok_eqi(mn,"brk"))  {
        u32 imm=(u32)lex_imm(l);
        emit32(em,0xD4200000u|((imm&0xFFFFu)<<5)); return;
    }
    if (tok_eqi(mn,"svc"))  {
        u32 imm=(u32)lex_imm(l);
        emit32(em,a64_svc(imm)); return;
    }
    if (tok_eqi(mn,"blr"))  {
        i32 rn=reg64(l->cur); lex_next(l);
        emit32(em,a64_blr((u32)rn)); return;
    }
    if (tok_eqi(mn,"br"))   {
        i32 rn=reg64(l->cur); lex_next(l);
        emit32(em,a64_br((u32)rn)); return;
    }
    if (tok_eqi(mn,"ret"))  {
        i32 rn=reg64(l->cur); lex_next(l);
        emit32(em,a64_ret((u32)rn)); return;
    }
    /* movz/movk/movn rd, #imm [, lsl #hw*16] */
    if (tok_eqi(mn,"movz")||tok_eqi(mn,"movk")||tok_eqi(mn,"movn")) {
        int sf=reg64_sf(l->cur);
        i32 rd=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        u64 imm=lex_imm(l);
        u32 hw=0;
        if (l->cur.kind==TK_COMMA) {
            lex_next(l); /* skip comma */
            /* expect lsl */
            lex_next(l); /* skip 'lsl' ident */
            u64 sh=lex_imm(l);
            hw=(u32)(sh/16);
        }
        u32 w;
        if (tok_eqi(mn,"movz")) w=a64_movz((u32)rd,(u32)imm,hw,(u32)sf);
        else if (tok_eqi(mn,"movk")) w=a64_movk((u32)rd,(u32)imm,hw,(u32)sf);
        else w=a64_movn((u32)rd,(u32)imm,hw,(u32)sf);
        emit32(em,w); return;
    }
    /* mov rd, #imm64 — expands to movz + movk chain */
    if (tok_eqi(mn,"mov")) {
        int sf=reg64_sf(l->cur);
        i32 rd=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_HASH||l->cur.kind==TK_INT) {
            u64 imm=lex_imm(l);
            /* emit movz+movk chain manually */
            u32 parts[4]; int np=0;
            for (int hw=0;hw<4;hw++) {
                u16 chunk=(u16)((imm>>(hw*16))&0xFFFFu);
                if (hw==0||chunk) {
                    if (np==0) parts[np++]=a64_movz((u32)rd,chunk,(u32)hw,(u32)sf);
                    else        parts[np++]=a64_movk((u32)rd,chunk,(u32)hw,(u32)sf);
                }
            }
            if (np==0) parts[np++]=a64_movz((u32)rd,0,0,(u32)sf);
            for(int i=0;i<np;i++) emit32(em,parts[i]);
        } else {
            /* mov rd, rn — ORR rd, xzr, rn */
            i32 rn=reg64(l->cur); lex_next(l);
            /* ORR (shifted reg): sf|01010|shift2|0|rm5|imm6|rn5|rd5 */
            u32 sf2=(u32)sf;
            u32 w=((sf2)<<31)|(0x2Au<<24)|((u32)rn<<16)|(31u<<5)|(u32)rd;
            emit32(em,w);
        }
        return;
    }
    /* add/sub/and/orr/eor rd, rn, #imm or rm */
    if (tok_eqi(mn,"add")||tok_eqi(mn,"sub")||tok_eqi(mn,"and")||
        tok_eqi(mn,"orr")||tok_eqi(mn,"eor")) {
        int sf=reg64_sf(l->cur);
        i32 rd=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 rn=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_HASH||l->cur.kind==TK_INT) {
            u32 imm=(u32)lex_imm(l);
            u32 w;
            if (tok_eqi(mn,"add")) w=a64_add_imm((u32)rd,(u32)rn,imm,0,(u32)sf);
            else                   w=a64_sub_imm((u32)rd,(u32)rn,imm,0,(u32)sf);
            emit32(em,w);
        } else {
            i32 rm=reg64(l->cur); lex_next(l);
            u32 sf2=(u32)sf;
            /* data-proc 2-source: sf|opc|01011|shift|0|rm|imm6|rn|rd */
            u32 opc;
            if (tok_eqi(mn,"add"))      opc=0x8Bu;
            else if (tok_eqi(mn,"sub")) opc=0xCBu;
            else if (tok_eqi(mn,"and")) opc=0x8Au|(sf2<<31)^(0x8Au);
            else if (tok_eqi(mn,"orr")) opc=0xAAu;
            else                        opc=0xCAu; /* eor */
            /* simplified: encode as add/sub shifted reg */
            u32 w=(sf2<<31)|(opc<<21)|(0u<<22)|((u32)rm<<16)|(0u<<10)|((u32)rn<<5)|(u32)rd;
            emit32(em,w);
        }
        return;
    }
    /* cmp rn, #imm */
    if (tok_eqi(mn,"cmp")) {
        int sf=reg64_sf(l->cur);
        i32 rn=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_HASH||l->cur.kind==TK_INT) {
            u32 imm=(u32)lex_imm(l);
            emit32(em,a64_cmp_imm((u32)rn,imm,(u32)sf));
        } else {
            i32 rm=reg64(l->cur); lex_next(l);
            emit32(em,a64_cmp_reg((u32)rn,(u32)rm,(u32)sf));
        }
        return;
    }
    /* csel rd, rn, rm, cond */
    if (tok_eqi(mn,"csel")) {
        int sf=reg64_sf(l->cur);
        i32 rd=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 rn=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 rm=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        u32 cc=parse_cc64(l->cur); lex_next(l);
        emit32(em,a64_csel((u32)rd,(u32)rn,(u32)rm,cc,(u32)sf));
        return;
    }
    /* ldr/str rd, [rn, #off] */
    if (tok_eqi(mn,"ldr")||tok_eqi(mn,"str")) {
        int sf=reg64_sf(l->cur);
        i32 rd=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        lex_next(l); /* skip [ */
        i32 rn=reg64(l->cur); lex_next(l);
        i32 off=0;
        if (l->cur.kind==TK_COMMA) {
            lex_next(l);
            off=(i32)lex_imm(l);
        }
        lex_next(l); /* skip ] */
        u32 w;
        if (tok_eqi(mn,"ldr")) w=a64_ldr((u32)rd,(u32)rn,(u32)off,(u32)sf);
        else                   w=a64_str((u32)rd,(u32)rn,(u32)off,(u32)sf);
        emit32(em,w); return;
    }
    /* adr/adrp rd, label */
    if (tok_eqi(mn,"adr")||tok_eqi(mn,"adrp")) {
        i32 rd=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_IDENT) {
            pat_add(pos,l->cur,64); lex_next(l);
        } else {
            i32 off=(i32)lex_imm(l);
            u32 w=tok_eqi(mn,"adr")?a64_adr((u32)rd,off):a64_adrp((u32)rd,off);
            emit32(em,w); return;
        }
        /* placeholder */
        emit32(em,a64_adr((u32)rd,0)); return;
    }
    /* b / bl / b.cond */
    if (tok_eqi(mn,"b")||tok_eqi(mn,"bl")) {
        int is_bl=tok_eqi(mn,"bl");
        if (l->cur.kind==TK_IDENT) {
            pat_add(pos,l->cur,64); lex_next(l);
            emit32(em,is_bl?a64_bl(0):a64_b(0)); return;
        }
        i32 off=(i32)lex_imm(l);
        emit32(em,is_bl?a64_bl(off/4):a64_b(off/4)); return;
    }
    /* b.cond label */
    if (tok_eqi(mn,"beq")||tok_eqi(mn,"bne")||tok_eqi(mn,"blt")||
        tok_eqi(mn,"bgt")||tok_eqi(mn,"ble")||tok_eqi(mn,"bge")) {
        /* strip leading 'b', rest is cc */
        char ccbuf[4]; ccbuf[0]=mn.p[1]; ccbuf[1]=mn.p[2]; ccbuf[2]=0;
        Tok cct; cct.p=ccbuf; cct.len=2; cct.kind=TK_IDENT;
        u32 cc=parse_cc64(cct);
        if (l->cur.kind==TK_IDENT) {
            pat_add(pos,l->cur,64); lex_next(l);
            emit32(em,a64_bcond(cc,0)); return;
        }
        i32 off=(i32)lex_imm(l);
        emit32(em,a64_bcond(cc,off/4)); return;
    }
    /* cbz/cbnz */
    if (tok_eqi(mn,"cbz")||tok_eqi(mn,"cbnz")) {
        int sf=reg64_sf(l->cur);
        i32 rn=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_IDENT) {
            pat_add(pos,l->cur,64); lex_next(l);
            emit32(em,tok_eqi(mn,"cbz")?a64_cbz((u32)rn,0,(u32)sf):a64_cbnz((u32)rn,0,(u32)sf));
            return;
        }
        i32 off=(i32)lex_imm(l);
        u32 w=tok_eqi(mn,"cbz")?a64_cbz((u32)rn,off/4,(u32)sf):a64_cbnz((u32)rn,off/4,(u32)sf);
        emit32(em,w); return;
    }
    /* stp/ldp (simplified: pre-index) */
    if (tok_eqi(mn,"stp")) {
        int sf=reg64_sf(l->cur);
        i32 r1=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 r2=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        lex_next(l); /* [ */
        i32 rn=reg64(l->cur); lex_next(l);
        i32 off=0;
        if (l->cur.kind==TK_COMMA) { lex_next(l); off=(i32)lex_imm(l); }
        int pre=0;
        lex_next(l); /* ] */
        if (l->cur.kind==TK_BANG) { pre=1; lex_next(l); }
        if (pre) emit32(em,a64_stp_pre((u32)r1,(u32)r2,(u32)rn,off,(u32)sf));
        else     emit32(em,a64_stp((u32)r1,(u32)r2,(u32)rn,off,(u32)sf));
        return;
    }
    if (tok_eqi(mn,"ldp")) {
        int sf=reg64_sf(l->cur);
        i32 r1=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 r2=reg64(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        lex_next(l); /* [ */
        i32 rn=reg64(l->cur); lex_next(l);
        i32 off=0;
        if (l->cur.kind==TK_COMMA) { lex_next(l); off=(i32)lex_imm(l); }
        lex_next(l); /* ] */
        int post=0;
        if (l->cur.kind==TK_COMMA) { lex_next(l); off=(i32)lex_imm(l); post=1; }
        if (post) emit32(em,a64_ldp_post((u32)r1,(u32)r2,(u32)rn,off,(u32)sf));
        else      emit32(em,a64_stp((u32)r1,(u32)r2,(u32)rn,off,(u32)sf)); /* fallback */
        return;
    }
    /* .word — raw 32-bit literal */
    if (tok_eq(mn,".word")||tok_eqi(mn,".word")) {
        u32 v=(u32)lex_imm(l);
        emit32(em,v); return;
    }
    /* unknown — emit NOP */
    emit32(em,A64_NOP);
}

/* ── ARM32 assembler ──────────────────────────────────────────────── */
static void asm_insn32(Emit *em, Tok mn, Lex *l) {
    lex_next(l);
    u32 pos=(u32)em->pos;

    if (tok_eq(mn,".sym1")) { em->sym1_va=pos; em->has_sym1=1; return; }
    if (tok_eq(mn,".sym2")) { em->sym2_va=pos; em->has_sym2=1; return; }

    if (tok_eqi(mn,"nop"))  { emit32(em,A32_NOP); return; }
    if (tok_eqi(mn,"bx"))   {
        /* bx lr or bx rN [,cond] */
        u32 cc=A32_AL;
        i32 rm=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) { lex_next(l); cc=parse_cc32(l->cur); lex_next(l); }
        emit32(em,a32_bx((u32)rm,cc)); return;
    }
    if (tok_eqi(mn,"swi")||tok_eqi(mn,"svc")) {
        u32 imm=(u32)lex_imm(l);
        emit32(em,a32_swi(imm,A32_AL)); return;
    }
    if (tok_eqi(mn,"push")) {
        /* push {r0,r1,...} */
        u32 regs=0;
        /* skip { if present */
        if (l->cur.kind==TK_LBRK||l->cur.kind==TK_IDENT) {
            /* parse register list */
            while (l->cur.kind!=TK_NL&&l->cur.kind!=TK_EOF&&l->cur.kind!=TK_RBRK) {
                if (l->cur.kind==TK_IDENT) {
                    i32 r=reg32a(l->cur);
                    if (r>=0) regs|=(1u<<(u32)r);
                    lex_next(l);
                } else lex_next(l);
            }
            if (l->cur.kind==TK_RBRK) lex_next(l);
        }
        emit32(em,a32_push(regs,A32_AL)); return;
    }
    if (tok_eqi(mn,"pop")) {
        u32 regs=0;
        while (l->cur.kind!=TK_NL&&l->cur.kind!=TK_EOF&&l->cur.kind!=TK_RBRK) {
            if (l->cur.kind==TK_IDENT) {
                i32 r=reg32a(l->cur);
                if (r>=0) regs|=(1u<<(u32)r);
                lex_next(l);
            } else lex_next(l);
        }
        if (l->cur.kind==TK_RBRK) lex_next(l);
        emit32(em,a32_pop(regs,A32_AL)); return;
    }
    if (tok_eqi(mn,"movw")||tok_eqi(mn,"movt")) {
        i32 rd=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        u32 imm=(u32)lex_imm(l);
        u32 w=tok_eqi(mn,"movw")?a32_movw((u32)rd,imm,A32_AL):a32_movt((u32)rd,imm,A32_AL);
        emit32(em,w); return;
    }
    if (tok_eqi(mn,"mov")) {
        i32 rd=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_HASH||l->cur.kind==TK_INT) {
            u32 imm=(u32)lex_imm(l);
            if (imm<=0xFFFFu) {
                emit32(em,a32_movw((u32)rd,imm,A32_AL));
                if (imm>0xFFu) emit32(em,a32_movt((u32)rd,imm>>16,A32_AL));
            } else {
                emit32(em,a32_movw((u32)rd,imm&0xFFFFu,A32_AL));
                emit32(em,a32_movt((u32)rd,imm>>16,A32_AL));
            }
        } else {
            i32 rm=reg32a(l->cur); lex_next(l);
            emit32(em,a32_mov_reg((u32)rd,(u32)rm,A32_AL));
        }
        return;
    }
    if (tok_eqi(mn,"add")||tok_eqi(mn,"sub")) {
        i32 rd=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 rn=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_HASH||l->cur.kind==TK_INT) {
            u32 imm=(u32)lex_imm(l);
            u32 w=tok_eqi(mn,"add")?a32_add_imm((u32)rd,(u32)rn,imm,0,0,A32_AL)
                                    :a32_sub_imm((u32)rd,(u32)rn,imm,0,0,A32_AL);
            emit32(em,w);
        } else {
            i32 rm=reg32a(l->cur); lex_next(l);
            u32 w=tok_eqi(mn,"add")?a32_add_reg((u32)rd,(u32)rn,(u32)rm,0,0,A32_AL)
                                    :a32_sub_reg((u32)rd,(u32)rn,(u32)rm,0,0,A32_AL);
            emit32(em,w);
        }
        return;
    }
    if (tok_eqi(mn,"cmp")) {
        i32 rn=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        if (l->cur.kind==TK_HASH||l->cur.kind==TK_INT) {
            u32 imm=(u32)lex_imm(l);
            emit32(em,a32_cmp_imm((u32)rn,imm,0,A32_AL));
        } else {
            i32 rm=reg32a(l->cur); lex_next(l);
            emit32(em,a32_cmp_reg((u32)rn,(u32)rm,A32_AL));
        }
        return;
    }
    if (tok_eqi(mn,"b")||tok_eqi(mn,"bl")) {
        int is_bl=tok_eqi(mn,"bl");
        u32 cc=A32_AL;
        /* check for b.cond or bcond syntax in next token */
        if (l->cur.kind==TK_IDENT) {
            pat_add(pos,l->cur,32); lex_next(l);
            emit32(em,is_bl?a32_bl(0,cc):a32_b(0,cc)); return;
        }
        i32 off=(i32)lex_imm(l);
        emit32(em,is_bl?a32_bl(off/4,cc):a32_b(off/4,cc)); return;
    }
    if (tok_eqi(mn,"ldr")||tok_eqi(mn,"str")) {
        i32 rd=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        lex_next(l); /* [ */
        i32 rn=reg32a(l->cur); lex_next(l);
        i32 off=0;
        if (l->cur.kind==TK_COMMA) { lex_next(l); off=(i32)lex_imm(l); }
        lex_next(l); /* ] */
        u32 w;
        if (tok_eqi(mn,"ldr")) w=a32_ldr_imm((u32)rd,(u32)rn,(u32)off,(u32)(off>=0),A32_AL);
        else                   w=a32_str_imm((u32)rd,(u32)rn,(u32)(off>=0?off:-off),(u32)(off>=0),A32_AL);
        emit32(em,w); return;
    }
    if (tok_eqi(mn,"mul")) {
        i32 rd=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 rn=reg32a(l->cur); lex_next(l);
        if (l->cur.kind==TK_COMMA) lex_next(l);
        i32 rm=reg32a(l->cur); lex_next(l);
        emit32(em,a32_mul((u32)rd,(u32)rn,(u32)rm,0,A32_AL)); return;
    }
    if (tok_eq(mn,".word")||tok_eqi(mn,".word")) {
        u32 v=(u32)lex_imm(l);
        emit32(em,v); return;
    }
    emit32(em,A32_NOP);
}

/* ── two-pass assembler ───────────────────────────────────────────── */
typedef struct { sz size; u32 sym1_va; u32 sym2_va; int has_sym1; int has_sym2; } AsmResult;

static u8 _code64[0x10000];
static u8 _code32[0x10000];

static AsmResult assemble(const u8 *src, sz src_len, int arch, u8 *out_code) {
    AsmResult res; res.size=0; res.sym1_va=0; res.sym2_va=0;
    res.has_sym1=0; res.has_sym2=0;

    lbl_reset();
    Emit em; em.buf=out_code; em.cap=0x10000; em.pos=0;
    em.sym1_va=0; em.sym2_va=0; em.has_sym1=0; em.has_sym2=0;

    /* two passes: 1=labels, 2=code */
    for (int pass=0; pass<2; pass++) {
        em.pos=0;
        Lex l; l.s=(const char*)src; l.e=(const char*)(src+src_len);
        lex_next(&l);
        while (l.cur.kind!=TK_EOF) {
            if (l.cur.kind==TK_NL) { lex_next(&l); continue; }
            if (l.cur.kind==TK_IDENT) {
                Tok mn=l.cur; lex_next(&l);
                /* check label */
                if (l.cur.kind==TK_COLON) {
                    lex_next(&l);
                    if (pass==0) lbl_def(mn.p[0]=='.'?(mn.p+1):(mn.p), (u32)em.pos); /* strip . prefix */
                    /* actually store full name */
                    if (pass==0) { char nb[80]; tok_copy(nb,mn); lbl_def(nb,(u32)em.pos); }
                    continue;
                }
                /* directive .section .text .globl .type etc — skip line */
                if (mn.p[0]=='.') {
                    /* .sym1/.sym2 handled in asm_insn */
                    if (tok_eq(mn,".sym1")||tok_eq(mn,".sym2")) {
                        if (pass==1) {
                            if (tok_eq(mn,".sym1")) { em.sym1_va=(u32)em.pos; em.has_sym1=1; }
                            else                    { em.sym2_va=(u32)em.pos; em.has_sym2=1; }
                        }
                    } else if (tok_eq(mn,".word")) {
                        if (pass==1) { u32 v=(u32)lex_imm(&l); emit32(&em,v); } else lex_imm(&l);
                        continue;
                    }
                    /* skip rest of line */
                    while (l.cur.kind!=TK_NL&&l.cur.kind!=TK_EOF) lex_next(&l);
                    continue;
                }
                if (pass==1) {
                    if (arch==64) asm_insn64(&em,mn,&l);
                    else          asm_insn32(&em,mn,&l);
                } else {
                    /* pass 0: count bytes without emitting (use dummy buf) */
                    u8 *real=em.buf; em.buf=_code32; /* reuse as dummy */
                    sz rpos=em.pos;
                    if (arch==64) asm_insn64(&em,mn,&l);
                    else          asm_insn32(&em,mn,&l);
                    em.buf=real; em.pos=rpos; /* restore */
                }
            } else lex_next(&l);
        }
        if (pass==0) {
            /* also do a real first pass for label offsets — use actual buf */
            em.pos=0;
            Lex l2; l2.s=(const char*)src; l2.e=(const char*)(src+src_len);
            lex_next(&l2);
            while (l2.cur.kind!=TK_EOF) {
                if (l2.cur.kind==TK_NL) { lex_next(&l2); continue; }
                if (l2.cur.kind==TK_IDENT) {
                    Tok mn2=l2.cur; lex_next(&l2);
                    if (l2.cur.kind==TK_COLON) {
                        char nb[80]; tok_copy(nb,mn2);
                        lbl_def(nb,(u32)em.pos);
                        lex_next(&l2); continue;
                    }
                    if (mn2.p[0]=='.') {
                        if (tok_eq(mn2,".word")) { lex_imm(&l2); em.pos+=4; }
                        else while (l2.cur.kind!=TK_NL&&l2.cur.kind!=TK_EOF) lex_next(&l2);
                        continue;
                    }
                    /* simulate emit to count bytes */
                    u32 before=(u32)em.pos;
                    if (arch==64) asm_insn64(&em,mn2,&l2);
                    else          asm_insn32(&em,mn2,&l2);
                    (void)before;
                } else lex_next(&l2);
            }
        }
    }

    /* backpatch branches */
    for (u32 i=0;i<_npat;i++) {
        i32 li=lbl_find(_pats[i].tgt);
        if (li<0) continue;
        u32 loff=_lbls[li].off;
        u32 ioff=_pats[i].insn_off;
        u32 insn=r32(em.buf+ioff);
        i32 delta=(i32)((i64)loff-(i64)ioff);
        /* detect instruction type by bits */
        if (_pats[i].arch==64) {
            u32 op=insn>>26;
            if (op==0x05u) { /* B */
                insn=(insn&0xFC000000u)|((u32)(delta/4)&0x03FFFFFFu);
            } else if (op==0x25u) { /* BL */
                insn=(insn&0xFC000000u)|((u32)(delta/4)&0x03FFFFFFu);
            } else if ((insn&0xFF000010u)==0x54000000u) { /* B.cond */
                insn=(insn&0xFF00001Fu)|(((u32)(delta/4)&0x7FFFFu)<<5);
            } else if ((insn&0x7E000000u)==0x34000000u) { /* CBZ/CBNZ */
                insn=(insn&0xFF00001Fu)|(((u32)(delta/4)&0x7FFFFu)<<5);
            } else { /* ADR fallback */
                u32 immlo=(u32)(delta)&3u;
                u32 immhi=(u32)(delta>>2)&0x7FFFFu;
                insn=(insn&0x9F00001Fu)|(immlo<<29)|(immhi<<5);
            }
        } else {
            /* ARM32 B/BL: bits[23:0] = signed offset/4 - 2 */
            i32 enc=(delta/4)-2; /* PC is 8 ahead in A32 */
            insn=(insn&0xFF000000u)|((u32)enc&0x00FFFFFFu);
        }
        w32(em.buf+ioff,insn);
    }

    res.size=em.pos;
    res.sym1_va=em.sym1_va; res.sym2_va=em.sym2_va;
    res.has_sym1=em.has_sym1; res.has_sym2=em.has_sym2;
    return res;
}

/* ── APK builder ──────────────────────────────────────────────────── */
static u8 _axml_buf[0x4000];
static u8 _dex_buf[200];
static u8 _so64_buf[0x8000];
static u8 _so32_buf[0x8000];

static i32 build_apk(
    const u8 *src, sz src_len,
    const char *pkg, const char *label, const char *libname,
    u32 min_sdk, u32 tgt_sdk,
    int do64, int do32,
    const char *outpath)
{
    /* assemble code */
    AsmResult r64; r64.size=0; r64.sym1_va=0; r64.sym2_va=0; r64.has_sym1=0; r64.has_sym2=0;
    AsmResult r32_; r32_.size=0; r32_.sym1_va=0; r32_.sym2_va=0; r32_.has_sym1=0; r32_.has_sym2=0;

    if (do64) r64=assemble(src,src_len,64,_code64);
    if (do32) r32_=assemble(src,src_len,32,_code32);

    /* build AndroidManifest.xml */
    sz axsz=axml_build(pkg,label,libname,min_sdk,tgt_sdk,_axml_buf,sizeof(_axml_buf));
    if (axsz==0) { pr_err("axml_build failed\n"); return -1; }

    /* build classes.dex */
    sz dexsz=dex_build(_dex_buf);

    /* build .so files */
    sz so64sz=0, so32sz=0;
    if (do64) {
        u8 *txt=r64.size?_code64:(u8*)0;
        so64sz=(sz)elf64_build_so(_so64_buf,txt,(u32)r64.size,r64.sym1_va,r64.sym2_va);
    }
    if (do32) {
        u8 *txt=r32_.size?_code32:(u8*)0;
        so32sz=(sz)elf32_build_so(_so32_buf,txt,(u32)r32_.size,r32_.sym1_va,r32_.sym2_va);
    }

    /* assemble ZIP */
    ZipWr zw;
    zip_init(&zw,_apk_buf,sizeof(_apk_buf));

    /* lib/arm64-v8a/lib<name>.so */
    if (do64&&so64sz) {
        /* build path */
        u8 p64[64];
        sz pi=0;
        const char *pf="lib/arm64-v8a/lib"; while(*pf) p64[pi++]=(u8)*pf++;
        const char *lp=libname; while(*lp) p64[pi++]=(u8)*lp++;
        const char *sx=".so"; while(*sx) p64[pi++]=(u8)*sx++;
        p64[pi]=0;
        zip_add(&zw,(const char*)p64,_so64_buf,so64sz);
    }
    if (do32&&so32sz) {
        u8 p32[64]; sz pi=0;
        const char *pf="lib/armeabi-v7a/lib"; while(*pf) p32[pi++]=(u8)*pf++;
        const char *lp=libname; while(*lp) p32[pi++]=(u8)*lp++;
        const char *sx=".so"; while(*sx) p32[pi++]=(u8)*sx++;
        p32[pi]=0;
        zip_add(&zw,(const char*)p32,_so32_buf,so32sz);
    }
    zip_add(&zw,"AndroidManifest.xml",_axml_buf,axsz);
    zip_add(&zw,"classes.dex",_dex_buf,dexsz);

    sz total=zip_finish(&zw);
    if (!total) { pr_err("zip_finish failed\n"); return -1; }

    /* write output file */
    i32 fd=os_open(outpath,0x241,0x1A4); /* O_WRONLY|O_CREAT|O_TRUNC, 0644 */
    if (fd<0) { pr_err("open output failed\n"); return -1; }
    sz written=0;
    while (written<total) {
        sz chunk=total-written; if(chunk>0x8000) chunk=0x8000;
        i32 n=os_write(fd,_apk_buf+written,chunk);
        if (n<=0) break;
        written+=(sz)n;
    }
    os_close(fd);
    pr("wrote "); pr_dec((u64)total); pr(" bytes to "); pr(outpath); pr_nl();
    return 0;
}

/* ── CLI ──────────────────────────────────────────────────────────── */
static u8 _src_local[0x100000];

static i32 apkc_main(i32 argc, char **argv) {
    const char *inpath=0;
    const char *outpath="out.apk";
    const char *pkg="com.example.app";
    const char *label="App";
    const char *libname="main";
    u32 min_sdk=21;
    u32 tgt_sdk=33;
    int do64=1, do32=1;

    for (i32 i=1;i<argc;i++) {
        char *a=argv[i];
        /* -o outpath */
        if (a[0]=='-'&&a[1]=='o'&&a[2]==0 && i+1<argc) { outpath=argv[++i]; continue; }
        if (a[0]=='-'&&a[1]=='p'&&a[2]==0 && i+1<argc) { pkg=argv[++i]; continue; }
        if (a[0]=='-'&&a[1]=='l'&&a[2]==0 && i+1<argc) { label=argv[++i]; continue; }
        if (a[0]=='-'&&a[1]=='n'&&a[2]==0 && i+1<argc) { libname=argv[++i]; continue; }
        if (a[0]=='-'&&a[1]=='m'&&a[2]==0 && i+1<argc) { min_sdk=(u32)0; { char*s=argv[++i]; while(*s) min_sdk=min_sdk*10+(u32)(*s++-'0'); } continue; }
        if (a[0]=='-'&&a[1]=='t'&&a[2]==0 && i+1<argc) { tgt_sdk=(u32)0; { char*s=argv[++i]; while(*s) tgt_sdk=tgt_sdk*10+(u32)(*s++-'0'); } continue; }
        if (a[0]=='-'&&a[1]=='6'&&a[2]=='4'&&a[3]==0) { do64=1; do32=0; continue; }
        if (a[0]=='-'&&a[1]=='3'&&a[2]=='2'&&a[3]==0) { do64=0; do32=1; continue; }
        if (a[0]=='-'&&a[1]=='b'&&a[2]=='o') { do64=1; do32=1; continue; } /* -both */
        if (a[0]!='-') { inpath=a; continue; }
        pr_err("unknown flag: "); pr_err(a); pr_err("\n");
    }

    if (!inpath) {
        pr_err("usage: apkc [options] source.s\n");
        pr_err("  -o <file>   output APK (default: out.apk)\n");
        pr_err("  -p <pkg>    package name\n");
        pr_err("  -l <label>  app label\n");
        pr_err("  -n <name>   native lib name (without lib prefix/.so)\n");
        pr_err("  -m <sdk>    minSdkVersion\n");
        pr_err("  -t <sdk>    targetSdkVersion\n");
        pr_err("  -64         arm64 only\n");
        pr_err("  -32         arm32 only\n");
        pr_err("  -both       both arches (default)\n");
        return 1;
    }

    /* read source file */
    i32 fd=os_open(inpath,0,0);
    if (fd<0) { pr_err("cannot open: "); pr_err(inpath); pr_err("\n"); return 1; }
    sz src_len=0;
    while (src_len<sizeof(_src_local)-1) {
        i32 n=os_read(fd,_src_local+src_len,(sz)sizeof(_src_local)-src_len-1);
        if (n<=0) break;
        src_len+=(sz)n;
    }
    os_close(fd);
    _src_local[src_len]=0;

    return build_apk(_src_local,src_len,pkg,label,libname,min_sdk,tgt_sdk,do64,do32,outpath);
}

/* ── freestanding entry ───────────────────────────────────────────── */
__attribute__((used))
static i32 apkc_entry(uptr *sp) {
    i32 argc=(i32)sp[0];
    char **argv=(char**)(sp+1);
    return apkc_main(argc,argv);
}

#if defined(__aarch64__)
__attribute__((naked,noreturn,section(".text.start")))
void _start(void) {
    __asm__ __volatile__(
        "mov x0, sp\n"
        "and sp, x0, #~15\n"
        "bl apkc_entry\n"
        "mov x8, #93\n"
        "svc #0\n"
        ::: "x0","x8","memory"
    );
}
#elif defined(__arm__)
__attribute__((naked,noreturn,section(".text.start")))
void _start(void) {
    __asm__ __volatile__(
        "mov r0, sp\n"
        "bic sp, r0, #7\n"
        "bl apkc_entry\n"
        "mov r7, #1\n"
        "swi #0\n"
        ::: "r0","r7","memory"
    );
}
#endif
