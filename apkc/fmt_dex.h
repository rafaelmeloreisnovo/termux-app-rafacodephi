/* fmt_dex.h — Minimal structural classes.dex generator.
 * Produces a DEX 035 container with no class definitions (header + map list).
 * Includes Adler-32 and SHA-1 from scratch. No malloc. No libc in production.
 *
 * This is a container/format primitive, not a Java/Kotlin compiler and not a
 * functional Android application by itself. */
#pragma once
#include "mem.h"

/* ── Adler-32 (DEX checksum, bytes [12..end]) ────────────────────────── */
static inline u32 adler32(const u8 *data, sz len) {
    u32 s1 = 1u, s2 = 0u;
    for (sz i = 0; i < len; i++) {
        s1 = (s1 + data[i]) % 65521u;
        s2 = (s2 + s1) % 65521u;
    }
    return (s2 << 16) | s1;
}

/* ── SHA-1 (DEX signature, bytes [32..end]) ──────────────────────────── */
typedef struct { u32 h[5]; u8 buf[64]; u64 bits; u32 blen; } SHA1Ctx;

static inline u32 _sha1_rot(u32 v, u8 n) { return (v << n) | (v >> (32u - n)); }

static inline void sha1_init(SHA1Ctx *c) {
    c->h[0] = 0x67452301u; c->h[1] = 0xEFCDAB89u;
    c->h[2] = 0x98BADCFEu; c->h[3] = 0x10325476u;
    c->h[4] = 0xC3D2E1F0u;
    c->bits = 0u; c->blen = 0u;
}

static inline void _sha1_block(SHA1Ctx *c, const u8 *blk) {
    u32 w[80], a, b, d, e, f, k, t;
    for (u32 i = 0; i < 16u; i++)
        w[i] = ((u32)blk[i * 4u] << 24u) |
               ((u32)blk[i * 4u + 1u] << 16u) |
               ((u32)blk[i * 4u + 2u] << 8u) |
               (u32)blk[i * 4u + 3u];
    for (u32 i = 16u; i < 80u; i++)
        w[i] = _sha1_rot(w[i - 3u] ^ w[i - 8u] ^ w[i - 14u] ^ w[i - 16u], 1u);

    a = c->h[0]; b = c->h[1]; u32 cc = c->h[2]; d = c->h[3]; e = c->h[4];
    for (u32 i = 0; i < 80u; i++) {
        if (i < 20u)      { f = (b & cc) | (~b & d);       k = 0x5A827999u; }
        else if (i < 40u) { f = b ^ cc ^ d;                k = 0x6ED9EBA1u; }
        else if (i < 60u) { f = (b & cc) | (b & d) | (cc & d); k = 0x8F1BBCDCu; }
        else              { f = b ^ cc ^ d;                k = 0xCA62C1D6u; }
        t = _sha1_rot(a, 5u) + f + e + k + w[i];
        e = d; d = cc; cc = _sha1_rot(b, 30u); b = a; a = t;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d; c->h[4] += e;
}

static inline void sha1_update(SHA1Ctx *c, const u8 *data, sz len) {
    c->bits += (u64)len * 8u;
    for (sz i = 0; i < len; i++) {
        c->buf[c->blen++] = data[i];
        if (c->blen == 64u) { _sha1_block(c, c->buf); c->blen = 0u; }
    }
}

static inline void sha1_final(SHA1Ctx *c, u8 out[20]) {
    /* Preserve the original message length. Padding bytes are not message bits. */
    const u64 message_bits = c->bits;

    c->buf[c->blen++] = 0x80u;
    if (c->blen > 56u) {
        while (c->blen < 64u) c->buf[c->blen++] = 0u;
        _sha1_block(c, c->buf);
        c->blen = 0u;
    }
    while (c->blen < 56u) c->buf[c->blen++] = 0u;
    for (i32 i = 7; i >= 0; i--)
        c->buf[c->blen++] = (u8)(message_bits >> (u32)(i * 8));
    _sha1_block(c, c->buf);
    c->blen = 0u;

    for (i32 i = 0; i < 5; i++) {
        out[i * 4]     = (u8)(c->h[i] >> 24u);
        out[i * 4 + 1] = (u8)(c->h[i] >> 16u);
        out[i * 4 + 2] = (u8)(c->h[i] >> 8u);
        out[i * 4 + 3] = (u8)c->h[i];
    }
}

/* ── DEX format constants ────────────────────────────────────────────── */
#define DEX_HEADER_SZ    0x70u
#define DEX_ENDIAN_TAG   0x12345678u
#define DEX_MINIMAL_SZ   0x8Cu
#define DEX_MAP_OFF      DEX_HEADER_SZ
#define DEX_DATA_SZ      (DEX_MINIMAL_SZ - DEX_MAP_OFF)

#define DEX_TYPE_HEADER  0x0000u
#define DEX_TYPE_MAPLIST 0x1000u

/* ── DEX generator ───────────────────────────────────────────────────── */
/*
 * Layout:
 *   [0x000..0x06F] header   (112 bytes)
 *   [0x070..0x08B] map list (28 bytes: count + 2 entries)
 *
 * There are deliberately no strings, types, methods or class definitions.
 */
static inline u32 dex_build_checked(u8 *out, sz cap) {
    if (!out || cap < (sz)DEX_MINIMAL_SZ) return 0u;

    m_set(out, 0u, (sz)DEX_MINIMAL_SZ);

    out[0] = 'd'; out[1] = 'e'; out[2] = 'x'; out[3] = '\n';
    out[4] = '0'; out[5] = '3'; out[6] = '5'; out[7] = '\0';

    w32(out + 32, DEX_MINIMAL_SZ);
    w32(out + 36, DEX_HEADER_SZ);
    w32(out + 40, DEX_ENDIAN_TAG);
    w32(out + 48, DEX_MAP_OFF);
    w32(out + 104, DEX_DATA_SZ);
    w32(out + 108, DEX_MAP_OFF);

    u8 *mp = out + DEX_MAP_OFF;
    w32(mp, 2u);
    w16(mp + 4, DEX_TYPE_HEADER);  w16(mp + 6, 0u);
    w32(mp + 8, 1u);               w32(mp + 12, 0u);
    w16(mp + 16, DEX_TYPE_MAPLIST); w16(mp + 18, 0u);
    w32(mp + 20, 1u);               w32(mp + 24, DEX_MAP_OFF);

    SHA1Ctx sc;
    sha1_init(&sc);
    sha1_update(&sc, out + 32, (sz)(DEX_MINIMAL_SZ - 32u));
    sha1_final(&sc, out + 12);

    w32(out + 8, adler32(out + 12, (sz)(DEX_MINIMAL_SZ - 12u)));
    return DEX_MINIMAL_SZ;
}

/* Compatibility entry point for existing callers that already guarantee size. */
static inline u32 dex_build(u8 *out) {
    return dex_build_checked(out, (sz)DEX_MINIMAL_SZ);
}
