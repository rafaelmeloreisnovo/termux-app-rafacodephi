/* SPDX-License-Identifier: GPL-3.0-only
 * RAFCODEPHI PA core: direct ELF entry payload.
 * No headers. No libc. No malloc. No JNI. No Java native methods.
 */

typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed long        sl;

typedef struct { sl s; sl n; } ts;

static u32 __attribute__((aligned(64))) m[1000];
static u32 __attribute__((aligned(64))) q[16];
static char __attribute__((aligned(64))) o[4096];
static u32 z;

#if defined(__aarch64__)
static inline sl w(sl f, const void *p, sl n) {
    register sl x0 __asm__("x0") = f;
    register const void *x1 __asm__("x1") = p;
    register sl x2 __asm__("x2") = n;
    register sl x8 __asm__("x8") = 64;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory", "cc");
    return x0;
}
static inline u64 t(void) {
    u64 x;
    __asm__ volatile("isb\n\tmrs %0, cntvct_el0" : "=r"(x));
    return x;
}
#define ARCH_FLAG 0xA6400001u
#elif defined(__arm__)
static inline sl w(sl f, const void *p, sl n) {
    register sl r0 __asm__("r0") = f;
    register const void *r1 __asm__("r1") = p;
    register sl r2 __asm__("r2") = n;
    register sl r7 __asm__("r7") = 4;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory", "cc");
    return r0;
}
static inline u64 t(void) {
    ts x;
    register sl r0 __asm__("r0") = 1;
    register ts *r1 __asm__("r1") = &x;
    register sl r7 __asm__("r7") = 263;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r7) : "memory", "cc");
    return ((u64)(u32)x.s << 32) | (u32)x.n;
}
#define ARCH_FLAG 0xA3200001u
#else
#error unsupported architecture
#endif

static void b(char c0) {
    if (z < (u32)sizeof(o)) o[z++] = c0;
}

static void x32(u32 v0) {
    static const char h[16] = "0123456789ABCDEF";
    u32 i;
    for (i = 0; i != 8; ++i) b(h[(v0 >> (28u - (i << 2))) & 15u]);
}

static void x64(u64 v0) {
    x32((u32)(v0 >> 32));
    x32((u32)v0);
}

static void s(const char *p) {
    while (*p) b(*p++);
}

static u32 c(const void *p0, u32 n) {
    const u8 *p = (const u8 *)p0;
    u32 r0 = ~0u;
    u32 i;
    for (i = 0; i != n; ++i) {
        u32 v0 = (r0 ^ p[i]) & 255u;
        u32 k;
        for (k = 0; k != 8; ++k) {
            u32 mask = 0u - (v0 & 1u);
            v0 = (v0 >> 1) ^ (0x82F63B78u & mask);
        }
        r0 = (r0 >> 8) ^ v0;
    }
    return ~r0;
}

static u32 a(u32 v0, u32 n) {
    u32 i;
    for (i = 0; i != n; ++i) {
        v0 ^= v0 >> 16;
        v0 *= 0x7FEB352Du;
        v0 ^= v0 >> 15;
        v0 *= 0x846CA68Bu;
        v0 ^= v0 >> 16;
    }
    return v0;
}

static void r(u32 cat, u32 profile, u64 *cy, u32 *cs, u32 *op, u32 *fl) {
    u32 i;
    u32 v0 = 0x9E3779B9u ^ (cat << 24) ^ (profile << 16);
    u64 t0 = t();
    *fl = ARCH_FLAG | 0x00000042u;

    if (cat == 0u) {
        for (i = 0; i != (4096u << (profile == 2u)); ++i) v0 = a(v0 + i + 0x67u, 1u);
        *op = i;
    } else if (cat == 1u) {
        static const u32 f[28] = {
            0x02u,0x03u,0x05u,0x07u,0x0Bu,0x11u,0x13u,0x17u,
            0x1Du,0x1Fu,0x25u,0x29u,0x2Bu,0x2Fu,0x3Bu,0x3Du,
            0x43u,0x47u,0x4Fu,0x53u,0x59u,0x5Bu,0x61u,0x67u,
            0x6Bu,0x6Du,0x71u,0x7Fu
        };
        for (i = 0; i != 42u; ++i) {
            u32 f0 = f[i % 28u];
            q[i & 15u] = a(q[i & 15u] ^ f0 ^ v0, 1u);
            v0 ^= q[i & 15u] + (f0 << 24);
        }
        *op = 42u;
    } else if (cat == 2u) {
        for (i = 0; i != 1000u; ++i) m[i] = a(v0 + i, 1u);
        for (i = 0; i != 1000u; ++i) v0 ^= m[(i * 37u) % 1000u];
        *op = 2000u;
    } else if (cat == 3u) {
        for (i = 0; i != 1000u; ++i) m[i] = v0 = a(v0 + i, 1u);
        v0 ^= c(m, (u32)sizeof(m));
        *op = (u32)sizeof(m);
    } else if (cat == 4u) {
        for (i = 0; i != 1008u; ++i) {
            u32 j = (i * 7u + 3u) % 1000u;
            m[j] ^= (v0 << (i & 7u)) | (v0 >> ((32u - (i & 7u)) & 31u));
            v0 = a(v0 ^ m[j], 1u);
        }
        *op = 1008u;
    } else {
        static const u32 f[32] = {
            0x02000000u,0x03000000u,0x05000000u,0x07000000u,
            0x0B000000u,0x11000000u,0x13000000u,0x17000000u,
            0x1D000000u,0x1F000000u,0x25000000u,0x29000000u,
            0x2B000000u,0x2F000000u,0x3B000000u,0x3D000000u,
            0x43000000u,0x47000000u,0x4F000000u,0x53000000u,
            0x59000000u,0x5B000000u,0x61000000u,0x67000000u,
            0x6B000000u,0x6D000000u,0x71000000u,0x7F000000u,
            0xE320F000u,0xD503201Fu,0xEF000000u,0xD4000001u
        };
        for (i = 0; i != 4096u; ++i) v0 = a(v0 ^ f[i & 31u] ^ i, 1u);
        *op = i;
    }

    *cy = t() - t0;
    *cs = v0 ^ c(q, (u32)sizeof(q)) ^ c(m, (u32)sizeof(m));
}

__attribute__((visibility("hidden"))) void v(void *sp) {
    u32 cat;
    u32 profile = 0u;
    (void)sp;
    z = 0u;
    for (cat = 0; cat != 1000u; ++cat) m[cat] = 0x9E3779B9u ^ cat;
    for (cat = 0; cat != 16u; ++cat) q[cat] = 0x00010001u * (cat + 1u);

    s("RAFCODEPHI-PA-ELF 00000001\n");
    s("MODE FREESTANDING NO_LIBC NO_MALLOC NO_JNI DIRECT_SYSCALL\n");
    for (cat = 0; cat != 6u; ++cat) {
        u64 cy;
        u32 cs, op, fl;
        u64 score;
        r(cat, profile, &cy, &cs, &op, &fl);
        score = ((u64)cs << 32) ^ ((u64)op << 13) ^ ~cy;
        b('R'); b((char)('0' + cat)); b(' ');
        x64(score); b(' '); x64(cy); b(' '); x32(cs); b(' '); x32(op); b(' '); x32(fl); b('\n');
    }
    s("END 00000000\n");
    (void)w(1, o, (sl)z);
}
