/**
 * verbovivo_graph.h — Pure Graph Logic (no ASCII, no legacy encoding)
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Hyperdimensional Computing via Graph Traversal:
 * - Nodes: HV vectors (1024-bit logical states)
 * - Edges: Conditional transitions (logic: AND, OR, XOR)
 * - T^7 Toroid: 7-dimensional lattice with 42 attractor regions
 * - Convergence: φ_fst = (1-H)·C as fixed-point walk in state space
 *
 * No ASCII. No legacy byte encoding. Binary operations only.
 * Stack-based conditional logic. Grafo puro.
 */
#ifndef VERBOVIVO_GRAPH_H
#define VERBOVIVO_GRAPH_H

/*
 * Type contract:
 * - Hosted Android/JNI builds use the platform headers.
 * - Pure freestanding builds may use -nostdinc, so rely on compiler-provided
 *   intrinsic type macros instead of guessing LP32/LP64 widths.
 *
 * This keeps uint64_t/int64_t truly 64-bit on armeabi-v7a and avoids
 * redefining standard typedefs when <stdint.h>/<stddef.h> are already active.
 */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stddef.h>
#include <stdint.h>
#else
#if !defined(__UINT8_TYPE__) || !defined(__UINT16_TYPE__) || \
    !defined(__UINT32_TYPE__) || !defined(__UINT64_TYPE__) || \
    !defined(__INT64_TYPE__) || !defined(__SIZE_TYPE__)
#error "Verbovivo freestanding build requires compiler intrinsic integer/size types"
#endif
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __SIZE_TYPE__ size_t;
typedef __INT64_TYPE__ int64_t;
#endif

/* ── Hypervector: 1024-bit node in the graph (128 × 8-byte lanes) ──────── */
#define HV_LANES      128u   /* 128 × 8 bytes = 1024 bits */
#define HV_BYTES      (HV_LANES * 8u)
#define HV_BITS       (HV_BYTES * 8u)

typedef struct {
    uint64_t lane[HV_LANES];  /* 128 lanes of 64-bit logical state */
} HyperVector;

/* ── Graph Edge: Conditional transition (logic operator + target node) ──── */
typedef enum {
    EDGE_XOR = 0,   /* destination = source XOR context */
    EDGE_AND = 1,   /* destination = source AND context */
    EDGE_OR  = 2,   /* destination = source OR context */
    EDGE_NOT = 3,   /* destination = NOT source */
    EDGE_NOOP = 4   /* identity: destination = source */
} EdgeLogic;

typedef struct {
    uint16_t target_node_id;  /* destination node in graph */
    uint8_t  logic;            /* EdgeLogic: XOR, AND, OR, NOT, NOOP */
    uint8_t  _pad;
    uint64_t context_mask;     /* operand for AND/OR/XOR (64-bit slice) */
} GraphEdge;

/* ── Graph Node: State in the computation space ────────────────────────── */
#define MAX_EDGES_PER_NODE  8u   /* max outgoing edges */

typedef struct {
    uint16_t node_id;
    uint8_t  is_attractor;     /* non-zero if fixed point */
    uint8_t  attractor_id;     /* 0-41 if attractor; else 255 */

    HyperVector state;         /* current HV */
    GraphEdge edges[MAX_EDGES_PER_NODE];
    uint8_t n_edges;

    uint32_t visit_count;      /* traversal statistics */
    uint64_t phi_fst_fixed;    /* φ_fst at convergence (Q16) */
} GraphNode;

/* ── T^7 Toroid Graph: 42 attractors in 7-dimensional lattice ──────────── */
#define ATTRACTOR_COUNT  42u

typedef struct {
    GraphNode nodes[256u];     /* sparse: up to 256 nodes */
    uint16_t n_nodes;

    /* Attractor shortcuts (dense) */
    uint16_t attractor_node_ids[ATTRACTOR_COUNT];

    /* Convergence state */
    uint16_t current_node;
    uint64_t iteration;
    uint64_t phi_entropy;      /* H_norm in Q16 */
    uint64_t phi_coherence;    /* C_norm in Q16 */
} T7ToroidGraph;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Initialize empty graph */
void vv_graph_init(T7ToroidGraph *g);

/* Add node to graph; returns node_id or -1 if full */
int vv_graph_add_node(T7ToroidGraph *g, uint8_t is_attractor, uint8_t attractor_id);

/* Add directed edge: from_id --[logic, context_mask]--> to_id */
int vv_graph_add_edge(T7ToroidGraph *g, uint16_t from_id, uint16_t to_id,
                      EdgeLogic logic, uint64_t context_mask);

/* Set node's HV state */
void vv_graph_set_state(T7ToroidGraph *g, uint16_t node_id, const HyperVector *hv);

/* Traverse one edge: apply logic to current state, move to target node */
int vv_graph_step(T7ToroidGraph *g, uint16_t edge_idx);

/* Convergence walk: iterate until φ stabilizes or max_steps exhausted */
int vv_graph_converge(T7ToroidGraph *g, uint32_t max_steps,
                      uint64_t *out_phi, uint8_t *out_attractor);

/* Compute H_norm (entropy) and C_norm (coherence) from current state */
void vv_graph_compute_phi(T7ToroidGraph *g, uint64_t *phi_entropy,
                          uint64_t *phi_coherence, uint64_t *phi_fst);

/* Recall: return attractor_id with highest φ similarity to query vector */
uint8_t vv_graph_recall(T7ToroidGraph *g, const HyperVector *query, uint32_t max_steps);

/* ── T^7 Toroid Builder ──────────────────────────────────────────────────── */

/* Build complete T^7 lattice with 42 attractors and Hamming neighbors */
int vv_build_t7_toroid(T7ToroidGraph *g);

/* Add intermediate bridge nodes between attractors */
int vv_add_intermediate_nodes(T7ToroidGraph *g, uint32_t count);

/* Verify all 42 attractors are present and coherent */
int vv_verify_t7_coherence(T7ToroidGraph *g);

/* Serialize T^7 graph to binary format */
int vv_serialize_t7(T7ToroidGraph *g, uint8_t *buf, uint32_t buf_size,
                    uint32_t *out_len);

#endif /* VERBOVIVO_GRAPH_H */
