/**
 * zipraf_manifest_pool.h — ZrManifest allocation pool (static-only)
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Prevents accidental stack allocation of ZrManifest (~59KB) which causes
 * silent stack overflow on Android (1MB thread stack).
 *
 * Architecture:
 *   - Single pre-allocated static instance per module
 *   - Mutex-guarded access (atomic flag for single-threaded use)
 *   - Page-alignment for Android 15+ (16KB boundary crossing prevention)
 *   - Compile-time and runtime guards against stack allocation
 */

#ifndef ZIPRAF_MANIFEST_POOL_H
#define ZIPRAF_MANIFEST_POOL_H

#include <stdint.h>
#include <stddef.h>
#include "zipraf_index.h"

/* ── Compile-time verification ──────────────────────────────────────────── */

/* Verify ZrManifest is exactly the size we expect. */
_Static_assert(sizeof(ZrManifest) >= 58000u && sizeof(ZrManifest) <= 60000u,
    "ZrManifest size changed — update ZR_MANIFEST_BYTES and pool allocation");

/* ── Global manifest pool (static, never on stack) ──────────────────────── */

/* Marker for runtime guard: manifest pool is locked (in use) */
typedef struct {
    uint32_t is_locked;     /* Atomic flag: 0 = free, 1 = in use */
    uint32_t reserved;      /* Pad to 8 bytes */
} ZrManifestLock;

/* Single global instance, page-aligned for Android 15+ */
typedef struct __attribute__((aligned(16384))) {
    ZrManifestLock  lock;           /* Acquisition guard */
    ZrManifest      manifest;       /* ~59 KB payload */
    uint8_t         _pad[256];      /* Padding to 16KB boundary for mmap safety */
} ZrManifestPool;

/* The ONE global instance — never allocate this on stack */
extern ZrManifestPool g_zr_manifest_pool;

/* ── Acquisition API (fail-closed) ──────────────────────────────────────– */

/**
 * Acquire exclusive access to the global manifest.
 * Returns pointer to manifest, or NULL if already locked (fail-closed).
 * Caller MUST call zr_release() when done.
 */
ZrManifest *zr_acquire(void);

/**
 * Release exclusive access to the manifest.
 * Sets lock to 0 for next acquisition.
 */
void zr_release(ZrManifest *m);

/**
 * Runtime guard: call at module init to verify manifest is static, not stack.
 * Returns 1 if valid (static allocation), 0 if suspect (likely stack).
 */
int zr_verify_static_allocation(void);

#endif /* ZIPRAF_MANIFEST_POOL_H */
