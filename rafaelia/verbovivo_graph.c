/**
 * verbovivo_graph.c — Pure Graph Computation (Binary Logic, No ASCII)
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * T^7 Toroid as directed graph:
 * - 42 attractors as fixed points
 * - Transitions via conditional logic (XOR, AND, OR)
 * - Convergence: φ = (1-H)·C as state walk until fixed point
 *
 * Zero malloc. Static stack allocation. Freestanding syscalls only.
 */
#define _POSIX_C_SOURCE 200809L
#include "verbovivo_graph.h"

/* Freestanding memcpy, memset */
static void* memcpy_fs(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* memset_fs(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

#define memcpy memcpy_fs
#define memset memset_fs

/* ── Bit operations (no libc, no ASCII) ──────────────────────────────── */
static inline uint32_t popcnt64(uint64_t x) {
    x -= (x >> 1) & 0x5555555555555555ULL;
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint32_t)(x * 0x0101010101010101ULL >> 56);
}

static uint32_t hamming_weight(const HyperVector *hv) {
    uint32_t count = 0;
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        count += popcnt64(hv->lane[i]);
    }
    return count;
}

/* XOR two hypervectors lane-wise */
static void hv_xor(HyperVector *out, const HyperVector *a, const HyperVector *b) {
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        out->lane[i] = a->lane[i] ^ b->lane[i];
    }
}

/* AND two hypervectors */
static void hv_and(HyperVector *out, const HyperVector *a, const HyperVector *b) {
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        out->lane[i] = a->lane[i] & b->lane[i];
    }
}

/* OR two hypervectors */
static void hv_or(HyperVector *out, const HyperVector *a, const HyperVector *b) {
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        out->lane[i] = a->lane[i] | b->lane[i];
    }
}

/* Bitwise NOT */
static void hv_not(HyperVector *out, const HyperVector *a) {
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        out->lane[i] = ~a->lane[i];
    }
}

/* Copy hypervector */
static void hv_copy(HyperVector *out, const HyperVector *in) {
    memcpy(out, in, sizeof(HyperVector));
}

/* Hamming distance (XOR + popcount) */
static uint32_t hv_hamming_distance(const HyperVector *a, const HyperVector *b) {
    uint32_t dist = 0;
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        dist += popcnt64(a->lane[i] ^ b->lane[i]);
    }
    return dist;
}

/* ── Graph Initialization ────────────────────────────────────────────── */
void vv_graph_init(T7ToroidGraph *g) {
    if (!g) return;
    memset(g, 0, sizeof(*g));
    g->current_node = 0xFFFFu;  /* undefined */
    g->iteration = 0u;
}

/* ── Node Management ─────────────────────────────────────────────────── */
int vv_graph_add_node(T7ToroidGraph *g, uint8_t is_attractor, uint8_t attractor_id) {
    if (!g || g->n_nodes >= 256u) return -1;

    uint16_t nid = g->n_nodes;
    GraphNode *node = &g->nodes[nid];

    node->node_id = nid;
    node->is_attractor = is_attractor ? 1u : 0u;
    node->attractor_id = attractor_id;
    node->n_edges = 0u;
    node->visit_count = 0u;
    node->phi_fst_fixed = 0u;
    memset(&node->state, 0, sizeof(HyperVector));

    g->n_nodes++;

    if (is_attractor && attractor_id < ATTRACTOR_COUNT) {
        g->attractor_node_ids[attractor_id] = nid;
    }

    return (int)nid;
}

/* ── Edge Management ─────────────────────────────────────────────────── */
int vv_graph_add_edge(T7ToroidGraph *g, uint16_t from_id, uint16_t to_id,
                      EdgeLogic logic, uint64_t context_mask) {
    if (!g || from_id >= g->n_nodes || to_id >= g->n_nodes) return -1;

    GraphNode *node = &g->nodes[from_id];
    if (node->n_edges >= MAX_EDGES_PER_NODE) return -1;

    uint8_t edge_idx = node->n_edges;
    GraphEdge *edge = &node->edges[edge_idx];

    edge->target_node_id = to_id;
    edge->logic = (uint8_t)logic;
    edge->context_mask = context_mask;

    node->n_edges++;
    return (int)edge_idx;
}

/* ── State Management ────────────────────────────────────────────────── */
void vv_graph_set_state(T7ToroidGraph *g, uint16_t node_id, const HyperVector *hv) {
    if (!g || node_id >= g->n_nodes || !hv) return;
    hv_copy(&g->nodes[node_id].state, hv);
}

/* ── Single Step: Apply edge logic ───────────────────────────────────── */
int vv_graph_step(T7ToroidGraph *g, uint16_t edge_idx) {
    if (!g || g->current_node >= g->n_nodes) return -1;

    GraphNode *node = &g->nodes[g->current_node];
    if (edge_idx >= node->n_edges) return -1;

    GraphEdge *edge = &node->edges[edge_idx];
    GraphNode *next_node = &g->nodes[edge->target_node_id];

    HyperVector temp;
    switch (edge->logic) {
        case EDGE_XOR:
            /* Create context mask hypervector (replicate context_mask across lanes) */
            for (uint32_t i = 0u; i < HV_LANES; i++) {
                temp.lane[i] = edge->context_mask;
            }
            hv_xor(&temp, &node->state, &temp);
            hv_copy(&next_node->state, &temp);
            break;

        case EDGE_AND:
            for (uint32_t i = 0u; i < HV_LANES; i++) {
                temp.lane[i] = edge->context_mask;
            }
            hv_and(&temp, &node->state, &temp);
            hv_copy(&next_node->state, &temp);
            break;

        case EDGE_OR:
            for (uint32_t i = 0u; i < HV_LANES; i++) {
                temp.lane[i] = edge->context_mask;
            }
            hv_or(&temp, &node->state, &temp);
            hv_copy(&next_node->state, &temp);
            break;

        case EDGE_NOT:
            hv_not(&temp, &node->state);
            hv_copy(&next_node->state, &temp);
            break;

        case EDGE_NOOP:
            hv_copy(&next_node->state, &node->state);
            break;

        default:
            return -1;
    }

    next_node->visit_count++;
    g->current_node = edge->target_node_id;
    g->iteration++;
    return 0;
}

/* ── Phi Computation: H_norm (entropy), C_norm (coherence) ────────────── */
void vv_graph_compute_phi(T7ToroidGraph *g, uint64_t *phi_entropy,
                          uint64_t *phi_coherence, uint64_t *phi_fst) {
    if (!g || g->current_node >= g->n_nodes) {
        if (phi_entropy) *phi_entropy = 0u;
        if (phi_coherence) *phi_coherence = 0u;
        if (phi_fst) *phi_fst = 0u;
        return;
    }

    GraphNode *node = &g->nodes[g->current_node];
    HyperVector *state = &node->state;

    /* H_norm = unique bit count / 1024 (in Q16: max 0x10000) */
    uint32_t hamming = hamming_weight(state);
    uint64_t h_norm = (((uint64_t)hamming << 16) / HV_BITS);  /* Q16 */
    if (h_norm > 0x10000u) h_norm = 0x10000u;

    /* C_norm = KAM-7 coherence metric (dot product vs seed) */
    /* Seed: repeating pattern 0x40503... across lanes */
    uint64_t c_accum = 0u;
    const uint64_t seed = 0x4050302010080402ULL;  /* KAM-7 seed */
    for (uint32_t i = 0u; i < HV_LANES; i++) {
        uint64_t xor_result = state->lane[i] ^ seed;
        c_accum += popcnt64(xor_result);
    }
    uint64_t c_norm = ((HV_BITS - c_accum) << 16) / HV_BITS;  /* Q16 */
    if (c_norm > 0x10000u) c_norm = 0x10000u;

    /* φ_fst = (1 - H_norm) × C_norm in Q16 */
    uint64_t one_minus_h = 0x10000u - h_norm;  /* Q16 */
    uint64_t phi = (one_minus_h * c_norm) >> 16;  /* Q16 result */

    if (phi_entropy) *phi_entropy = h_norm;
    if (phi_coherence) *phi_coherence = c_norm;
    if (phi_fst) *phi_fst = phi;

    g->phi_entropy = h_norm;
    g->phi_coherence = c_norm;
}

/* ── Convergence: Walk until fixed point or max iterations ────────────── */
int vv_graph_converge(T7ToroidGraph *g, uint32_t max_steps,
                      uint64_t *out_phi, uint8_t *out_attractor) {
    if (!g || g->current_node >= g->n_nodes) return -1;

    uint64_t prev_phi = 0u;
    uint32_t stable_count = 0u;

    for (uint32_t step = 0u; step < max_steps; step++) {
        GraphNode *node = &g->nodes[g->current_node];

        /* Compute current φ */
        uint64_t phi;
        vv_graph_compute_phi(g, 0, 0, &phi);

        /* Check for fixed point (attractor) */
        if (node->is_attractor) {
            node->phi_fst_fixed = phi;
            if (out_phi) *out_phi = phi;
            if (out_attractor) *out_attractor = node->attractor_id;
            return 0;  /* converged */
        }

        /* Check φ stability */
        if (phi == prev_phi) {
            stable_count++;
            if (stable_count >= 3u) {
                /* Reached pseudo-fixed point */
                if (out_phi) *out_phi = phi;
                if (out_attractor) *out_attractor = 255u;  /* non-attractor */
                return 1;  /* stable, not attractor */
            }
        } else {
            stable_count = 0u;
        }
        prev_phi = phi;

        /* Step to random or best outgoing edge (greedy towards attractor) */
        if (node->n_edges == 0u) {
            /* Dead end */
            if (out_phi) *out_phi = phi;
            if (out_attractor) *out_attractor = 255u;
            return 2;  /* no edges */
        }

        /* Greedy: follow edge 0 (or implement smarter heuristic) */
        vv_graph_step(g, 0u);
    }

    /* Max steps exhausted */
    uint64_t phi;
    vv_graph_compute_phi(g, 0, 0, &phi);
    if (out_phi) *out_phi = phi;
    if (out_attractor) *out_attractor = 255u;
    return 3;  /* timeout */
}

/* ── Recall: Find attractor closest to query vector ────────────────────── */
uint8_t vv_graph_recall(T7ToroidGraph *g, const HyperVector *query, uint32_t max_steps) {
    if (!g || !query) return 255u;

    uint8_t best_attractor = 255u;
    uint32_t min_distance = 0xFFFFFFFFu;

    /* Scan all attractors */
    for (uint8_t ai = 0u; ai < ATTRACTOR_COUNT; ai++) {
        uint16_t node_id = g->attractor_node_ids[ai];
        if (node_id >= g->n_nodes) continue;

        GraphNode *node = &g->nodes[node_id];
        uint32_t dist = hv_hamming_distance(&node->state, query);

        if (dist < min_distance) {
            min_distance = dist;
            best_attractor = ai;
        }
    }

    return best_attractor;
}
