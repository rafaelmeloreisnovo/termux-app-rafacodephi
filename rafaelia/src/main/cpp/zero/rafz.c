#include "rafz.h"

#define RAFZ_GUARD 0x52415A46u /* RAZF */
#define RAFZ_CRC32C_POLY 0x82F63B78u

RAFZ_INLINE rafz_u32 rafz_rotl32(rafz_u32 x, rafz_u32 r) {
    r &= 31u;
    return (x << r) | (x >> ((32u - r) & 31u));
}

RAFZ_INLINE rafz_u32 rafz_load_le32(const rafz_u8 *p) {
    return ((rafz_u32)p[0]) |
           ((rafz_u32)p[1] << 8u) |
           ((rafz_u32)p[2] << 16u) |
           ((rafz_u32)p[3] << 24u);
}

RAFZ_INLINE void rafz_store_le16(rafz_u8 *p, rafz_u16 v) {
    p[0] = (rafz_u8)v;
    p[1] = (rafz_u8)(v >> 8u);
}

RAFZ_INLINE void rafz_store_le32(rafz_u8 *p, rafz_u32 v) {
    p[0] = (rafz_u8)v;
    p[1] = (rafz_u8)(v >> 8u);
    p[2] = (rafz_u8)(v >> 16u);
    p[3] = (rafz_u8)(v >> 24u);
}

RAFZ_NOINLINE static void rafz_copy_bytes(rafz_u8 *dst, const rafz_u8 *src, rafz_u32 bytes) {
    rafz_u32 i;
    for (i = 0u; i < bytes; ++i) dst[i] = src[i];
}

RAFZ_NOINLINE static void rafz_zero_bytes(rafz_u8 *dst, rafz_u32 bytes) {
    rafz_u32 i;
    for (i = 0u; i < bytes; ++i) dst[i] = 0u;
}

static rafz_u32 rafz_crc32c_skip_header_crc(const rafz_u8 *header) {
    rafz_u32 crc = 0xFFFFFFFFu;
    rafz_u32 i;
    rafz_u32 b;
    for (i = 0u; i < RAFZ_FRAME_HEADER_BYTES; ++i) {
        rafz_u8 byte = (i >= 36u && i < 40u) ? 0u : header[i];
        crc ^= (rafz_u32)byte;
        for (b = 0u; b < 8u; ++b) {
            rafz_u32 mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (RAFZ_CRC32C_POLY & mask);
        }
    }
    return ~crc;
}

static void rafz_mix_payload(rafz_ctx *ctx, const rafz_u8 *payload, rafz_u32 bytes) {
    rafz_u32 round;
    rafz_u32 i;
    for (round = 0u; round < RAFZ_MIX_ROUNDS; ++round) {
        for (i = 0u; i < bytes; ++i) {
            rafz_u32 lane = i & 7u;
            rafz_u32 peer = (lane + 3u + round) & 7u;
            rafz_u32 x = ctx->lane[lane];
            x ^= (rafz_u32)payload[i] + 0x9E3779B9u + i + (round << 16u);
            x ^= rafz_rotl32(ctx->lane[peer], 5u + lane);
            x *= 0x85EBCA6Bu;
            x = rafz_rotl32(x, 7u + lane * 3u);
            ctx->lane[lane] = x + 0xC2B2AE35u + ctx->seq_lo;
        }
    }
    for (i = 0u; i < RAFZ_PHASES; ++i) {
        ctx->phase[i] = rafz_rotl32(ctx->phase[i] ^ ctx->lane[i], i + 3u) + ctx->lane[i + 1u];
    }
}

static void rafz_sequence_inc(rafz_ctx *ctx) {
    ctx->seq_lo += 1u;
    if (ctx->seq_lo == 0u) ctx->seq_hi += 1u;
}

rafz_build_info rafz_get_build_info(void) {
    rafz_build_info info;
    info.abi = 0x00010000u;
    info.arch_id = RAFZ_ARCH_ID;
    info.native_big_endian = RAFZ_NATIVE_BIG_ENDIAN;
    info.lanes = RAFZ_LANES;
    info.phases = RAFZ_PHASES;
    info.slot_count = RAFZ_SLOT_COUNT;
    info.max_payload = RAFZ_MAX_PAYLOAD;
    info.mix_rounds = RAFZ_MIX_ROUNDS;
    info.arena_bytes = RAFZ_ARENA_BYTES;
    info.cacheline = RAFZ_CACHELINE;
    return info;
}

rafz_u32 rafz_crc32c(const void *data, rafz_u32 bytes) {
    const rafz_u8 *p = (const rafz_u8 *)data;
    rafz_u32 crc = 0xFFFFFFFFu;
    rafz_u32 i;
    rafz_u32 b;
    if (p == (const rafz_u8 *)0 && bytes != 0u) return 0u;
    for (i = 0u; i < bytes; ++i) {
        crc ^= (rafz_u32)p[i];
        for (b = 0u; b < 8u; ++b) {
            rafz_u32 mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (RAFZ_CRC32C_POLY & mask);
        }
    }
    return ~crc;
}

rafz_u8 rafz_bagua_rol3(rafz_u8 value) {
    rafz_u8 v = (rafz_u8)(value & 7u);
    return (rafz_u8)((((rafz_u32)v << 1u) | ((rafz_u32)v >> 2u)) & 7u);
}

rafz_u8 rafz_bagua_ror3(rafz_u8 value) {
    rafz_u8 v = (rafz_u8)(value & 7u);
    return (rafz_u8)((((rafz_u32)v >> 1u) | ((rafz_u32)v << 2u)) & 7u);
}

rafz_s32 rafz_q16_step(rafz_s32 current) {
    rafz_s32 high = current >> 16;
    rafz_u32 low = (rafz_u32)current & 0xFFFFu;
    rafz_s32 scaled = high * (rafz_s32)RAFZ_Q16_GEOM;
    scaled += (rafz_s32)((low * (rafz_u32)RAFZ_Q16_GEOM) >> 16u);
    return scaled + (rafz_s32)RAFZ_Q16_FORCE;
}

rafz_status rafz_init(rafz_ctx *ctx, void *arena, rafz_u32 arena_bytes) {
    rafz_u32 i;
    if (ctx == (rafz_ctx *)0 || arena == (void *)0) return RAFZ_E_NULL;
    if (arena_bytes < RAFZ_ARENA_BYTES) return RAFZ_E_SIZE;
    rafz_zero_bytes((rafz_u8 *)ctx, (rafz_u32)sizeof(*ctx));
    ctx->arena = (rafz_u8 *)arena;
    ctx->arena_bytes = arena_bytes;
    ctx->q16_value = 0;
    ctx->guard = RAFZ_GUARD;
    rafz_zero_bytes(ctx->arena, RAFZ_ARENA_BYTES);
    for (i = 0u; i < RAFZ_LANES; ++i) {
        ctx->lane[i] = 0x243F6A88u ^ (0x9E3779B9u * (i + 1u));
    }
    for (i = 0u; i < RAFZ_PHASES; ++i) {
        ctx->phase[i] = 0xA5A5A5A5u ^ (0x7F4A7C15u * (i + 1u));
    }
    return RAFZ_OK;
}

rafz_status rafz_image_init(rafz_image *image) {
    if (image == (rafz_image *)0) return RAFZ_E_NULL;
    return rafz_init(&image->ctx, image->arena, RAFZ_ARENA_BYTES);
}

rafz_status rafz_frame_encode(
    void *dst,
    rafz_u32 dst_bytes,
    const void *payload,
    rafz_u32 payload_bytes,
    rafz_u32 flags,
    rafz_u32 source_lo,
    rafz_u32 source_hi,
    rafz_u32 seq_lo,
    rafz_u32 seq_hi,
    rafz_u32 *frame_bytes) {
    rafz_u8 *out = (rafz_u8 *)dst;
    const rafz_u8 *in = (const rafz_u8 *)payload;
    rafz_u32 total;
    rafz_u32 payload_crc;
    rafz_u32 header_crc;
    if (out == (rafz_u8 *)0 || frame_bytes == (rafz_u32 *)0) return RAFZ_E_NULL;
    if (in == (const rafz_u8 *)0 && payload_bytes != 0u) return RAFZ_E_NULL;
    if (payload_bytes > RAFZ_MAX_PAYLOAD) return RAFZ_E_RANGE;
    total = RAFZ_FRAME_HEADER_BYTES + payload_bytes;
    if (dst_bytes < total) return RAFZ_E_SIZE;
    rafz_zero_bytes(out, RAFZ_FRAME_HEADER_BYTES);
    rafz_store_le32(out + 0u, RAFZ_FRAME_MAGIC);
    rafz_store_le16(out + 4u, (rafz_u16)RAFZ_FRAME_VERSION);
    rafz_store_le16(out + 6u, (rafz_u16)RAFZ_FRAME_HEADER_BYTES);
    rafz_store_le32(out + 8u, flags);
    rafz_store_le32(out + 12u, payload_bytes);
    rafz_store_le32(out + 16u, seq_lo);
    rafz_store_le32(out + 20u, seq_hi);
    rafz_store_le32(out + 24u, source_lo);
    rafz_store_le32(out + 28u, source_hi);
    payload_crc = rafz_crc32c(in, payload_bytes);
    rafz_store_le32(out + 32u, payload_crc);
    header_crc = rafz_crc32c_skip_header_crc(out);
    rafz_store_le32(out + 36u, header_crc);
    rafz_copy_bytes(out + RAFZ_FRAME_HEADER_BYTES, in, payload_bytes);
    *frame_bytes = total;
    return RAFZ_OK;
}

rafz_status rafz_ingest(rafz_ctx *ctx, const void *frame, rafz_u32 frame_bytes) {
    const rafz_u8 *in = (const rafz_u8 *)frame;
    rafz_u32 payload_bytes;
    rafz_u32 expected_payload_crc;
    rafz_u32 expected_header_crc;
    rafz_u32 slot;
    rafz_u8 *dst;
    if (ctx == (rafz_ctx *)0 || in == (const rafz_u8 *)0) return RAFZ_E_NULL;
    if (ctx->guard != RAFZ_GUARD || ctx->arena == (rafz_u8 *)0 || ctx->arena_bytes < RAFZ_ARENA_BYTES) return RAFZ_E_STATE;
    if (frame_bytes < RAFZ_FRAME_HEADER_BYTES) { ctx->rejected += 1u; return RAFZ_E_SIZE; }
    if (rafz_load_le32(in + 0u) != RAFZ_FRAME_MAGIC) { ctx->rejected += 1u; return RAFZ_E_MAGIC; }
    if ((rafz_u16)(in[4] | ((rafz_u16)in[5] << 8u)) != RAFZ_FRAME_VERSION ||
        (rafz_u16)(in[6] | ((rafz_u16)in[7] << 8u)) != RAFZ_FRAME_HEADER_BYTES) {
        ctx->rejected += 1u;
        return RAFZ_E_VERSION;
    }
    payload_bytes = rafz_load_le32(in + 12u);
    if (payload_bytes > RAFZ_MAX_PAYLOAD || frame_bytes != RAFZ_FRAME_HEADER_BYTES + payload_bytes) {
        ctx->rejected += 1u;
        return RAFZ_E_RANGE;
    }
    expected_header_crc = rafz_load_le32(in + 36u);
    if (rafz_crc32c_skip_header_crc(in) != expected_header_crc) {
        ctx->rejected += 1u;
        return RAFZ_E_CRC;
    }
    expected_payload_crc = rafz_load_le32(in + 32u);
    if (rafz_crc32c(in + RAFZ_FRAME_HEADER_BYTES, payload_bytes) != expected_payload_crc) {
        ctx->rejected += 1u;
        return RAFZ_E_CRC;
    }
    slot = ctx->write_slot;
    dst = ctx->arena + slot * RAFZ_SLOT_BYTES;
    rafz_copy_bytes(dst, in, frame_bytes);
    if (frame_bytes < RAFZ_SLOT_BYTES) rafz_zero_bytes(dst + frame_bytes, RAFZ_SLOT_BYTES - frame_bytes);
    rafz_mix_payload(ctx, in + RAFZ_FRAME_HEADER_BYTES, payload_bytes);
    ctx->q16_value = rafz_q16_step(ctx->q16_value);
    ctx->last_payload_crc = expected_payload_crc;
    ctx->accepted += 1u;
    ctx->write_slot = (slot + 1u == RAFZ_SLOT_COUNT) ? 0u : slot + 1u;
    if (ctx->live_slots < RAFZ_SLOT_COUNT) ctx->live_slots += 1u;
    rafz_sequence_inc(ctx);
    return RAFZ_OK;
}

rafz_u32 rafz_state_digest(const rafz_ctx *ctx) {
    rafz_u32 x;
    rafz_u32 i;
    if (ctx == (const rafz_ctx *)0 || ctx->guard != RAFZ_GUARD) return 0u;
    x = 0x6A09E667u ^ ctx->seq_lo ^ rafz_rotl32(ctx->seq_hi, 13u);
    for (i = 0u; i < RAFZ_LANES; ++i) x = rafz_rotl32(x ^ ctx->lane[i], i + 5u) * 0x9E3779B1u;
    for (i = 0u; i < RAFZ_PHASES; ++i) x = rafz_rotl32(x + ctx->phase[i], i + 9u) ^ 0x85EBCA6Bu;
    x ^= (rafz_u32)ctx->q16_value;
    x ^= ctx->last_payload_crc;
    return x;
}

const void *rafz_slot_ptr(const rafz_ctx *ctx, rafz_u32 age, rafz_u32 *frame_bytes) {
    rafz_u32 slot;
    const rafz_u8 *p;
    rafz_u32 payload_bytes;
    if (frame_bytes != (rafz_u32 *)0) *frame_bytes = 0u;
    if (ctx == (const rafz_ctx *)0 || frame_bytes == (rafz_u32 *)0 || ctx->guard != RAFZ_GUARD) return (const void *)0;
    if (age >= ctx->live_slots) return (const void *)0;
    slot = (ctx->write_slot + RAFZ_SLOT_COUNT - 1u - age) % RAFZ_SLOT_COUNT;
    p = ctx->arena + slot * RAFZ_SLOT_BYTES;
    if (rafz_load_le32(p) != RAFZ_FRAME_MAGIC) return (const void *)0;
    payload_bytes = rafz_load_le32(p + 12u);
    if (payload_bytes > RAFZ_MAX_PAYLOAD) return (const void *)0;
    *frame_bytes = RAFZ_FRAME_HEADER_BYTES + payload_bytes;
    return p;
}

rafz_status rafz_selfcheck(void) {
    rafz_u32 x;
    for (x = 0u; x < 8u; ++x) {
        if (rafz_bagua_ror3(rafz_bagua_rol3((rafz_u8)x)) != (rafz_u8)x) return RAFZ_E_STATE;
        if (rafz_bagua_rol3(rafz_bagua_ror3((rafz_u8)x)) != (rafz_u8)x) return RAFZ_E_STATE;
    }
    if (rafz_crc32c("123456789", 9u) != 0xE3069283u) return RAFZ_E_CRC;
    return RAFZ_OK;
}
