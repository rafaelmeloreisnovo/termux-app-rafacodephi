// file: rafpkg_graph.h
// RAFAELIA :: build dependency graph + deterministic topo order
// SPDX-License-Identifier: MIT

#ifndef RAFPKG_GRAPH_H
#define RAFPKG_GRAPH_H

#include "rafpkg_map.h"
#include "rafpkg_db.h"

// build adjacency + indegrees for db packages (only internal deps present in map)
typedef struct {
  u32 n;
  u32 **adj;
  u32 *adj_len;
  u32 *adj_cap;
  u32 *indeg;
  u8  *skip; // excluded packages
} raf_graph_t;

int  raf_graph_build(raf_graph_t *g, const raf_db_t *db, const raf_map_t *map);
void raf_graph_free(raf_graph_t *g);

// compute deterministic topological order for all nodes not skipped.
// out_order must have capacity >= db->pkgs_cnt.
// returns number of nodes in order, or 0 on cycle/error.
u32 raf_graph_toposort(const raf_graph_t *g, const raf_db_t *db, u32 *out_order, u32 out_cap);

#endif
