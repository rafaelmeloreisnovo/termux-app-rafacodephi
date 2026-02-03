#include <stdint.h>
#include <stddef.h>

#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
  #include <arm_acle.h>
  #define HAVE_HWCRC 1
#else
  #define HAVE_HWCRC 0
#endif

// CRC32 (IEEE) using ARMv8 CRC intrinsics when available.
// NOTE: This is CRC32 (not CRC32C) — if you need CRC32C, say so and we switch polynomial.
uint32_t crc32_hw(uint32_t crc, const void *data, size_t len) {
#if HAVE_HWCRC
    const uint8_t *p = (const uint8_t*)data;

    // Process 8 bytes at a time
    while (len >= 8) {
        uint64_t v;
        __builtin_memcpy(&v, p, 8);
        crc = __crc32d(crc, v);
        p += 8;
        len -= 8;
    }
    while (len--) {
        crc = __crc32b(crc, *p++);
    }
    return crc;
#else
    // Fallback: tiny (slow) placeholder — replace with your crc32_sw if desired
    const uint8_t *p = (const uint8_t*)data;
    while (len--) crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1u))) ^ *p++;
    return crc;
#endif
}
