// file: rafpkg_graph.c
// RAFAELIA :: build dependency graph + deterministic topo order
// SPDX-License-Identifier: MIT

#include "rafpkg_graph.h"
#include "rafpkg_util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int ensure_adj(raf_graph_t *g, u32 u, u32 add) {
  if (g->adj_len[u] + add <= g->adj_cap[u]) return 1;
  u32 newcap = g->adj_cap[u] ? g->adj_cap[u] : 8;
  while (newcap < g->adj_len[u] + add) newcap *= 2;
  u32 *p = (u32*)realloc(g->adj[u], (size_t)newcap * sizeof(u32));
  if (!p) return 0;
  g->adj[u] = p;
  g->adj_cap[u] = newcap;
  return 1;
}

int raf_graph_build(raf_graph_t *g, const raf_db_t *db, const raf_map_t *map) {
  if (!g || !db || !map) return 0;
  memset(g, 0, sizeof(*g));
  g->n = db->pkgs_cnt;
  g->adj = (u32**)calloc((size_t)g->n, sizeof(u32*));
  g->adj_len = (u32*)calloc((size_t)g->n, sizeof(u32));
  g->adj_cap = (u32*)calloc((size_t)g->n, sizeof(u32));
  g->indeg = (u32*)calloc((size_t)g->n, sizeof(u32));
  g->skip = (u8*)calloc((size_t)g->n, sizeof(u8));
  if (!g->adj || !g->adj_len || !g->adj_cap || !g->indeg || !g->skip) return 0;

  // mark excluded
  for (u32 i = 0; i < g->n; i++) g->skip[i] = (db->pkgs[i].excluded ? 1 : 0);

  // build edges: dep -> pkg (so dep must come earlier)
  for (u32 i = 0; i < g->n; i++) {
    if (g->skip[i]) continue;
    const raf_pkg_t *p = &db->pkgs[i];
    for (u32 j = 0; j < p->dep_cnt; j++) {
      u32 dep_off = db->dep_idx[p->dep_off + j];
      const char *dep = raf_db_dep_at(db, dep_off);
      if (!dep || !dep[0]) continue;
      u32 dep_idx = 0;
      if (!raf_map_get(map, dep, &dep_idx)) continue; // external dep
      if (dep_idx >= g->n) continue;
      if (g->skip[dep_idx]) continue;

      // add edge dep_idx -> i
      if (!ensure_adj(g, dep_idx, 1)) return 0;
      g->adj[dep_idx][g->adj_len[dep_idx]++] = i;
      g->indeg[i]++;
    }
  }

  return 1;
}

void raf_graph_free(raf_graph_t *g) {
  if (!g) return;
  if (g->adj) {
    for (u32 i = 0; i < g->n; i++) free(g->adj[i]);
  }
  free(g->adj);
  free(g->adj_len);
  free(g->adj_cap);
  free(g->indeg);
  free(g->skip);
  memset(g, 0, sizeof(*g));
}

u32 raf_graph_toposort(const raf_graph_t *g, const raf_db_t *db, u32 *out_order, u32 out_cap) {
  if (!g || !db || !out_order || out_cap < db->pkgs_cnt) return 0;

  // local indeg copy
  u32 *ind = (u32*)malloc((size_t)g->n * sizeof(u32));
  u8  *used = (u8*)calloc((size_t)g->n, sizeof(u8));
  if (!ind || !used) { free(ind); free(used); return 0; }
  memcpy(ind, g->indeg, (size_t)g->n * sizeof(u32));

  u32 want = 0;
  for (u32 i = 0; i < g->n; i++) if (!g->skip[i]) want++;

  u32 out = 0;
  while (out < want) {
    // pick lexicographically smallest package with indeg==0 and not used
    i32 pick = -1;
    const char *pick_name = NULL;
    for (u32 i = 0; i < g->n; i++) {
      if (g->skip[i] || used[i]) continue;
      if (ind[i] != 0) continue;
      const char *nm = db->pkgs[i].name.s;
      if (!pick_name || strcmp(nm, pick_name) < 0) {
        pick = (i32)i;
        pick_name = nm;
      }
    }
    if (pick < 0) {
      // cycle or missing external constraints
      fprintf(stderr, "rafpkg: cycle detected (remaining nodes with indeg>0).\n");
      u32 rem = 0;
      for (u32 i = 0; i < g->n; i++) {
        if (g->skip[i] || used[i]) continue;
        rem++;
        if (rem <= 50) {
          fprintf(stderr, "  - %s indeg=%u\n", db->pkgs[i].name.s, ind[i]);
        }
      }
      fprintf(stderr, "  remaining=%u\n", rem);
      free(ind);
      free(used);
      return 0;
    }

    u32 u = (u32)pick;
    used[u] = 1;
    out_order[out++] = u;

    // relax
    for (u32 k = 0; k < g->adj_len[u]; k++) {
      u32 v = g->adj[u][k];
      if (g->skip[v]) continue;
      if (ind[v] > 0) ind[v]--;
    }
  }

  free(ind);
  free(used);
  return out;
}
