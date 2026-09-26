#ifndef RAFZ_H
#define RAFZ_H

/*
 * RAFAELIA ZERO — freestanding binary kernel.
 * No libc, no heap, no GC, no RTTI, no exceptions, no dynamic dispatch.
 * The caller owns all memory. The core performs no I/O and no syscalls.
 */

#if !defined(__GNUC__) && !defined(__clang__)
# error "RAFZ requires a compiler exposing builtin integer types"
#endif

typedef __UINT8_TYPE__  rafz_u8;
typedef __UINT16_TYPE__ rafz_u16;
typedef __UINT32_TYPE__ rafz_u32;
typedef __INT32_TYPE__  rafz_s32;
typedef __UINTPTR_TYPE__ rafz_uptr;

#define RAFZ_VERSION_MAJOR 1u
#define RAFZ_VERSION_MINOR 0u
#define RAFZ_VERSION_PATCH 0u

#ifndef RAFZ_LANES
#define RAFZ_LANES 8u
#endif
#ifndef RAFZ_PHASES
#define RAFZ_PHASES 7u
#endif
#ifndef RAFZ_SLOT_COUNT
#define RAFZ_SLOT_COUNT 42u
#endif
#ifndef RAFZ_MAX_PAYLOAD
#define RAFZ_MAX_PAYLOAD 1024u
#endif
#ifndef RAFZ_CACHELINE
#define RAFZ_CACHELINE 64u
#endif
#ifndef RAFZ_MIX_ROUNDS
#define RAFZ_MIX_ROUNDS 2u
#endif
#ifndef RAFZ_Q16_GEOM
#define RAFZ_Q16_GEOM 56756
#endif
#ifndef RAFZ_Q16_FORCE
#define RAFZ_Q16_FORCE 203333
#endif

#define RAFZ_FRAME_MAGIC 0x315A4652u /* RFZ1 in little endian */
#define RAFZ_FRAME_VERSION 1u
#define RAFZ_FRAME_HEADER_BYTES 40u
#define RAFZ_SLOT_BYTES (RAFZ_FRAME_HEADER_BYTES + RAFZ_MAX_PAYLOAD)
#define RAFZ_ARENA_BYTES (RAFZ_SLOT_COUNT * RAFZ_SLOT_BYTES)
#define RAFZ_Q16_SHIFT 16u
#define RAFZ_Q16_ONE (1u << RAFZ_Q16_SHIFT)

#if RAFZ_LANES != 8u
# error "RAFZ v1 binary ABI fixes RAFZ_LANES at 8"
#endif
#if RAFZ_PHASES != 7u
# error "RAFZ v1 binary ABI fixes RAFZ_PHASES at 7"
#endif
#if RAFZ_SLOT_COUNT < 2u || RAFZ_SLOT_COUNT > 4096u
# error "RAFZ_SLOT_COUNT must be in [2,4096]"
#endif
#if RAFZ_MAX_PAYLOAD < 16u || RAFZ_MAX_PAYLOAD > 65535u
# error "RAFZ_MAX_PAYLOAD must be in [16,65535]"
#endif
#if RAFZ_MIX_ROUNDS < 1u || RAFZ_MIX_ROUNDS > 16u
# error "RAFZ_MIX_ROUNDS must be in [1,16]"
#endif

#if defined(__aarch64__)
# define RAFZ_ARCH_ID 2u
# define RAFZ_ARCH_AARCH64 1
#elif defined(__arm__)
# define RAFZ_ARCH_ID 1u
# define RAFZ_ARCH_ARMV7 1
#elif defined(__x86_64__)
# define RAFZ_ARCH_ID 3u
# define RAFZ_ARCH_X86_64 1
#elif defined(__i386__)
# define RAFZ_ARCH_ID 4u
# define RAFZ_ARCH_I686 1
#elif defined(__riscv) && (__riscv_xlen == 64)
# define RAFZ_ARCH_ID 5u
# define RAFZ_ARCH_RISCV64 1
#elif defined(__mips__)
# define RAFZ_ARCH_ID 6u
# define RAFZ_ARCH_MIPS32 1
#elif defined(__wasm32__)
# define RAFZ_ARCH_ID 7u
# define RAFZ_ARCH_WASM32 1
#else
# define RAFZ_ARCH_ID 0u
# define RAFZ_ARCH_UNKNOWN 1
#endif

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
# define RAFZ_NATIVE_BIG_ENDIAN 1u
#else
# define RAFZ_NATIVE_BIG_ENDIAN 0u
#endif

#if defined(__clang__)
# define RAFZ_INLINE static __inline__ __attribute__((always_inline))
# define RAFZ_NOINLINE __attribute__((noinline))
# define RAFZ_ALIGN(N) __attribute__((aligned(N)))
# define RAFZ_EXPORT __attribute__((visibility("default")))
#elif defined(__GNUC__)
# define RAFZ_INLINE static __inline__ __attribute__((always_inline))
# define RAFZ_NOINLINE __attribute__((noinline))
# define RAFZ_ALIGN(N) __attribute__((aligned(N)))
# define RAFZ_EXPORT __attribute__((visibility("default")))
#endif

#define RAFZ_STATIC_ASSERT(name, expr) typedef char rafz_static_assert_##name[(expr) ? 1 : -1]

typedef enum rafz_status {
    RAFZ_OK = 0,
    RAFZ_E_NULL = -1,
    RAFZ_E_SIZE = -2,
    RAFZ_E_MAGIC = -3,
    RAFZ_E_VERSION = -4,
    RAFZ_E_CRC = -5,
    RAFZ_E_RANGE = -6,
    RAFZ_E_STATE = -7
} rafz_status;

typedef struct rafz_build_info {
    rafz_u32 abi;
    rafz_u32 arch_id;
    rafz_u32 native_big_endian;
    rafz_u32 lanes;
    rafz_u32 phases;
    rafz_u32 slot_count;
    rafz_u32 max_payload;
    rafz_u32 mix_rounds;
    rafz_u32 arena_bytes;
    rafz_u32 cacheline;
} rafz_build_info;

typedef struct RAFZ_ALIGN(RAFZ_CACHELINE) rafz_ctx {
    rafz_u8 *arena;
    rafz_u32 arena_bytes;
    rafz_u32 write_slot;
    rafz_u32 live_slots;
    rafz_u32 seq_lo;
    rafz_u32 seq_hi;
    rafz_u32 accepted;
    rafz_u32 rejected;
    rafz_u32 last_payload_crc;
    rafz_u32 lane[RAFZ_LANES];
    rafz_u32 phase[RAFZ_PHASES];
    rafz_s32 q16_value;
    rafz_u32 guard;
} rafz_ctx;

typedef struct RAFZ_ALIGN(RAFZ_CACHELINE) rafz_image {
    rafz_ctx ctx;
    rafz_u8 arena[RAFZ_ARENA_BYTES];
} rafz_image;

RAFZ_STATIC_ASSERT(u8_is_1, sizeof(rafz_u8) == 1u);
RAFZ_STATIC_ASSERT(u16_is_2, sizeof(rafz_u16) == 2u);
RAFZ_STATIC_ASSERT(u32_is_4, sizeof(rafz_u32) == 4u);
RAFZ_STATIC_ASSERT(frame_is_40, RAFZ_FRAME_HEADER_BYTES == 40u);

#ifdef __cplusplus
extern "C" {
#endif

RAFZ_EXPORT rafz_build_info rafz_get_build_info(void);
RAFZ_EXPORT rafz_u32 rafz_crc32c(const void *data, rafz_u32 bytes);
RAFZ_EXPORT rafz_u8 rafz_bagua_rol3(rafz_u8 value);
RAFZ_EXPORT rafz_u8 rafz_bagua_ror3(rafz_u8 value);
RAFZ_EXPORT rafz_s32 rafz_q16_step(rafz_s32 current);
RAFZ_EXPORT rafz_status rafz_init(rafz_ctx *ctx, void *arena, rafz_u32 arena_bytes);
RAFZ_EXPORT rafz_status rafz_image_init(rafz_image *image);
RAFZ_EXPORT rafz_status rafz_frame_encode(
    void *dst,
    rafz_u32 dst_bytes,
    const void *payload,
    rafz_u32 payload_bytes,
    rafz_u32 flags,
    rafz_u32 source_lo,
    rafz_u32 source_hi,
    rafz_u32 seq_lo,
    rafz_u32 seq_hi,
    rafz_u32 *frame_bytes);
RAFZ_EXPORT rafz_status rafz_ingest(rafz_ctx *ctx, const void *frame, rafz_u32 frame_bytes);
RAFZ_EXPORT rafz_u32 rafz_state_digest(const rafz_ctx *ctx);
RAFZ_EXPORT const void *rafz_slot_ptr(const rafz_ctx *ctx, rafz_u32 age, rafz_u32 *frame_bytes);
RAFZ_EXPORT rafz_status rafz_selfcheck(void);

#ifdef __cplusplus
}
#endif

#endif /* RAFZ_H */
