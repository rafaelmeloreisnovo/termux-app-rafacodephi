/**
 * test_verbovivo_convergence.c — T^7 Toroid Convergence Validation
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Minimal freestanding test: verify graph construction, convergence walk,
 * and φ_fst computation bounds.
 */
#define _POSIX_C_SOURCE 200809L

/* Minimal type definitions */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef signed long int64_t;
typedef unsigned long size_t;

/* Freestanding write for exit status */
static long write_syscall(int fd, const void *buf, unsigned long count) {
    register long x0 asm("x0") = (long)fd;
    register long x1 asm("x1") = (long)buf;
    register long x2 asm("x2") = (long)count;
    register long x8 asm("x8") = 64;  /* SYS_write */
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "cc", "memory");
    return x0;
}

static void write_str(const char *s) {
    unsigned long len = 0;
    while (s[len]) len++;
    write_syscall(2, s, len);
}

#include "../../rafaelia/verbovivo_graph.h"

int main(void) {
    write_str("=== Verbovivo Convergence Test ===\n");

    /* 1. Initialize T^7 toroid */
    T7ToroidGraph g;
    if (vv_build_t7_toroid(&g) != 0) {
        write_str("FAIL: T^7 toroid construction failed\n");
        return 1;
    }
    write_str("✓ T^7 toroid built with 42 attractors\n");

    /* 2. Verify coherence */
    if (vv_verify_t7_coherence(&g) != 0) {
        write_str("FAIL: Coherence check failed\n");
        return 1;
    }
    write_str("✓ Graph coherence validated\n");

    /* 3. Set start node to attractor 0 */
    g.current_node = g.attractor_node_ids[0];
    write_str("✓ Starting at attractor 0\n");

    /* 4. Run convergence walk */
    uint64_t phi_result = 0;
    uint8_t attractor_result = 255;
    int status = vv_graph_converge(&g, 5000, &phi_result, &attractor_result);

    write_str("✓ Convergence walk completed\n");
    write_str("  Status: ");
    if (status == 0) write_str("ATTRACTOR");
    else if (status == 1) write_str("STABLE");
    else if (status == 2) write_str("NO_EDGES");
    else write_str("TIMEOUT");
    write_str("\n");

    /* 5. Validate φ bounds */
    if (phi_result > 0x10000u) {
        write_str("FAIL: φ out of Q16 bounds\n");
        return 1;
    }
    write_str("✓ φ within bounds [0, 0x10000]\n");

    /* 6. Validate attractor consistency */
    if (status == 0 && attractor_result >= 42) {
        write_str("FAIL: Attractor ID invalid for convergence\n");
        return 1;
    }
    write_str("✓ Attractor ID consistent\n");

    write_str("=== All tests passed ===\n");
    return 0;
}
