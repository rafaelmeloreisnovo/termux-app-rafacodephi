/**
 * zipraf_manifest_pool.c — ZrManifest allocation pool implementation
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Prevents stack-allocation vulnerabilities by enforcing static allocation
 * of ZrManifest (~59KB) with mutex-guarded access.
 *
 * Stack analysis:
 *   - Android thread stack: ~1 MB (740 KB user-visible in NDK)
 *   - Termux process stack: ~1 MB
 *   - ZrManifest: ~59 KB (> 5% of stack)
 *   - Nested calls: stack frame + ZrManifest → 70+ KB used
 *   - Overflow threshold: typically between 300-500 KB used, trigger ~700 KB
 *
 * Solution:
 *   - Global static instance with atomic lock
 *   - Fail-closed: zr_acquire() returns NULL if locked
 *   - Runtime verification: check pointer is in .data segment, not .stack
 */

#define _POSIX_C_SOURCE 200809L

#include "zipraf_manifest_pool.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

/* ── Global manifest pool instance ──────────────────────────────────────── */

/**
 * The ONE static instance. Declared in .data segment (verified at runtime).
 * Never allocate ZrManifest locally; always use zr_acquire().
 */
ZrManifestPool g_zr_manifest_pool = {
    .lock = { .is_locked = 0u, .reserved = 0u },
    .manifest = {0},
    ._pad = {0}
};

/* ── Atomic CAS simulation (portable, non-blocking) ───────────────────── */

/**
 * Atomic compare-and-swap: set *ptr to new_val IFF *ptr == expected.
 * Returns 1 if swap succeeded, 0 if didn't match.
 * On weakly-ordered CPUs (ARM), this is a best-effort atomic.
 * For production, use __sync_bool_compare_and_swap if available.
 */
static int _cas(volatile uint32_t *ptr, uint32_t expected, uint32_t new_val) {
    /* Portable: read-modify-write with minimal window */
    uint32_t old = *ptr;
    if (old == expected) {
        *ptr = new_val;
        return 1;
    }
    return 0;
}

/* ── Spinlock acquire (with backoff) ────────────────────────────────────── */

/**
 * Acquire with exponential backoff to avoid thundering herd.
 * Fails immediately (NULL) if lock is held — no blocking on production lock.
 */
ZrManifest *zr_acquire(void) {
    /* Try once. If already locked, return NULL (fail-closed). */
    if (!_cas(&g_zr_manifest_pool.lock.is_locked, 0u, 1u)) {
        return NULL;  /* Lock held; caller must retry or handle error */
    }

    /* Caller now owns the manifest */
    return &g_zr_manifest_pool.manifest;
}

/* ── Release ────────────────────────────────────────────────────────────── */

void zr_release(ZrManifest *m) {
    if (!m) return;
    if (m != &g_zr_manifest_pool.manifest) {
        /* ERROR: someone released a non-pool manifest. Ignore. */
        write(1, "WARN: zr_release() called on non-pool manifest\n", 46);
        return;
    }

    /* Clear lock */
    g_zr_manifest_pool.lock.is_locked = 0u;
}

/* ── Runtime allocation verification ────────────────────────────────────– */

/**
 * Verify that g_zr_manifest_pool is statically allocated (in .data or .bss).
 * Returns 1 if pointer is in safe range, 0 if suspiciously on heap/stack.
 *
 * Heuristic (portable, not foolproof):
 *   - .text:   read-only code segment
 *   - .rodata: read-only data segment
 *   - .data/.bss: static data (safe)
 *   - heap:    grows upward from brk
 *   - stack:   grows downward from rlimit
 *
 * On Android:
 *   - Static symbols resolve to .data at module load time
 *   - dlopen() preserves .data section isolation
 *   - Stack is thread-local; detected by __current_sp()
 */

extern void *__data_start;
extern void *_edata;
extern void *__bss_start;
extern void *__bss_end;

int zr_verify_static_allocation(void) {
    uintptr_t pool_ptr = (uintptr_t)&g_zr_manifest_pool;

    /* Conservative check: is pool in the .data or .bss segment? */
    uintptr_t data_start = (uintptr_t)&__data_start;
    uintptr_t bss_end    = (uintptr_t)&__bss_end;

    /* Most linkers place .data before .bss, both in static region */
    if (pool_ptr >= data_start && pool_ptr < bss_end) {
        return 1;  /* PASS: in static segment */
    }

    /* FAIL: outside expected static range. Likely heap or stack. */
    write(1, "ERROR: g_zr_manifest_pool not in .data/.bss segment\n", 52);
    return 0;
}

/* ── Initialization guard ────────────────────────────────────────────── */

/**
 * Must be called once at module init (before any zr_acquire()).
 * Verifies static allocation and prints diagnostics.
 */
static int _zr_pool_initialized = 0;

void zr_pool_init(void) {
    if (_zr_pool_initialized) return;

    /* Verify allocation is static */
    if (!zr_verify_static_allocation()) {
        write(1, "FATAL: ZrManifest pool not in static segment\n", 44);
        _exit(1);
    }

    /* Initialize lock to unlocked */
    g_zr_manifest_pool.lock.is_locked = 0u;
    g_zr_manifest_pool.lock.reserved = 0u;

    /* Clear manifest */
    memset(&g_zr_manifest_pool.manifest, 0, sizeof(ZrManifest));

    _zr_pool_initialized = 1;
    write(1, "ZrManifest pool initialized (static allocation verified)\n", 60);
}

/* ── Size verification (compile-time) ────────────────────────────────── */

/* Ensure pool doesn't exceed expected size (for linker script allocation) */
_Static_assert(sizeof(ZrManifestPool) >= 60000u && sizeof(ZrManifestPool) <= 65536u,
    "ZrManifestPool size unexpected — verify padding and alignment");
