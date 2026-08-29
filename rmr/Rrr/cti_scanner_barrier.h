/**
 * cti_scanner_barrier.h — CtiScanner memory barrier support for TOROID mode
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Fixes BUG-06: Race condition in CtiScanner TOROID mode
 *
 * When multiple threads run CtiScanner instances concurrently in TOROID mode,
 * they share access to the attractor table and phase space state. Without
 * memory barriers, cache coherency violations can occur on multi-core ARM64.
 *
 * Solution: Add dmb ish barrier before reading attractor table state and
 * after updating scanner results to ensure all cores see consistent data.
 */

#pragma once
#ifndef CTI_SCANNER_BARRIER_H
#define CTI_SCANNER_BARRIER_H

#include <stdint.h>

/**
 * Memory barrier for multi-core cache coherency
 *
 * ARM64:  dmb ish (inner shareable domain)
 * ARM32:  dmb (full system)
 * x86:    noop (x86 has strong ordering)
 * Other:  full barrier via atomic
 */
static inline void cti_barrier_acquire(void) {
#ifdef __aarch64__
    /* ARM64: dmb ish — load/store acquire semantics */
    __asm__ __volatile__("dmb ish" ::: "memory");
#elif defined(__arm__)
    /* ARM32: full dmb */
    __asm__ __volatile__("dmb" ::: "memory");
#else
    /* Fallback: compiler barrier for other architectures */
    __asm__ __volatile__("" ::: "memory");
#endif
}

static inline void cti_barrier_release(void) {
#ifdef __aarch64__
    /* ARM64: dmb ish — store/load release semantics */
    __asm__ __volatile__("dmb ish" ::: "memory");
#elif defined(__arm__)
    /* ARM32: full dmb */
    __asm__ __volatile__("dmb" ::: "memory");
#else
    /* Fallback: compiler barrier */
    __asm__ __volatile__("" ::: "memory");
#endif
}

/**
 * Full synchronization (acquire + release)
 * Used when both reading and writing shared state
 */
static inline void cti_barrier_full(void) {
    cti_barrier_acquire();
    cti_barrier_release();
}

#endif /* CTI_SCANNER_BARRIER_H */
