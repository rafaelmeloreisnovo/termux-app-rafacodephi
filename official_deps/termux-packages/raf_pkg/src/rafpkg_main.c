// file: rafpkg_main.c
// RAFAELIA :: rafpkg CLI
// SPDX-License-Identifier: MIT

#include "rafpkg_scan.h"
#include "rafpkg_graph.h"
#include "rafpkg_json.h"
#include "rafpkg_util.h"
#include "rafpkg_cpu_id.h"
#include "rafpkg_fileaudit.h"
#include "rafpkg_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
  fprintf(stderr,
    "rafpkg (RAFAELIA)\n"
    "usage:\n"
    "  rafpkg scan [--roots a:b:c] [--outdir path] [--arch arch]\n"
    "  rafpkg plan [--roots a:b:c] [--arch arch]\n"
    "  rafpkg audit [--roots a:b:c] [--arch arch]\n"
    "  rafpkg cpu\n"
    "  rafpkg fileaudit [--root path] [--depth n] [--cache path|--no-cache]\n"
    "  rafpkg store-dump <cachefile> [limit]\n"
    "\n"
    "notes:\n"
    "  - roots are relative to current working directory (termux-packages).\n"
    "  - parsing is static: build.sh is NOT executed.\n"
    "  - fileaudit cache default: .rafstore/fileaudit.bin (in current working dir)\n"
  );
}

static int cmd_cpu(void) {
  raf_cpu_id_t id;
  raf_cpu_probe(&id);
  char flags[512];
  raf_cpu_recommend_cflags(&id, flags, (u32)sizeof(flags));

  printf("cpu: aarch64=%u page=%u hwcap=0x%llx hwcap2=0x%llx\n",
         id.is_aarch64, id.page_size,
         (unsigned long long)id.hwcap, (unsigned long long)id.hwcap2);
  printf("cpu: asimd=%u aes=%u pmull=%u sha1=%u sha2=%u crc32=%u atomics=%u sve=%u\n",
         id.asimd, id.aes, id.pmull, id.sha1, id.sha2, id.crc32, id.atomics, id.sve);
  printf("cpu: recommend_cflags: %s\n", flags);
  return 0;
}

static int cmd_fileaudit(const char *root, int max_depth, const char *cache_path, int no_cache) {
  if (!root || !root[0]) root = ".";
  if (max_depth <= 0) max_depth = 6;
  if (no_cache) cache_path = "";
  printf("fileaudit: root=%s depth=%d cache=%s\n", root, max_depth,
         (cache_path && cache_path[0]) ? cache_path : (no_cache ? "(disabled)" : "(default)"));
  if (cache_path) {
    return raf_fileaudit_run_cached(root, max_depth, cache_path);
  }
  return raf_fileaudit_run(root, max_depth);
}

static int cmd_store_dump(const char *path, u32 limit) {
  if (!path || !path[0]) {
    fprintf(stderr, "rafpkg: store-dump needs a cachefile\n");
    return 1;
  }
  raf_store_t st;
  raf_store_init(&st);
  int rc = raf_store_load(&st, path);
  if (rc != 0) {
    fprintf(stderr, "rafpkg: store-dump: failed to load %s (rc=%d)\n", path, rc);
    raf_store_free(&st);
    return 2;
  }

  printf("rafstore: entries=%u\n", st.cnt);
  u32 n = st.cnt;
  if (limit && limit < n) n = limit;
  for (u32 i = 0; i < n; i++) {
    const raf_store_rec_t *r = &st.recs[i];
    printf("%5u kind=%u mtime=%llu size=%llu path=%s\n",
           i, (unsigned)r->kind,
           (unsigned long long)r->mtime_ns,
           (unsigned long long)r->size,
           r->path ? r->path : "");
  }

  raf_store_free(&st);
  return 0;
}

static u32 split_roots(char *s, const char **out, u32 out_cap) {
  u32 n = 0;
  if (!s) return 0;
  char *p = s;
  while (*p && n < out_cap) {
    while (*p == ':' || *p == ',' || *p == ' ') p++;
    if (!*p) break;
    char *q = p;
    while (*q && *q != ':' && *q != ',' ) q++;
    if (*q) *q++ = 0;
    out[n++] = p;
    p = q;
  }
  return n;
}

static const char *env_or_default(const char *k, const char *def) {
  const char *v = getenv(k);
  return (v && v[0]) ? v : def;
}

static int build_map(const raf_db_t *db, raf_map_t *map) {
  if (!db || !map) return 0;
  if (!raf_map_init(map, db->pkgs_cnt * 2 + 64)) return 0;
  for (u32 i = 0; i < db->pkgs_cnt; i++) {
    raf_map_put(map, db->pkgs[i].name.s, i);
  }
  return 1;
}

static int cmd_scan(const char **roots, u32 roots_cnt, const char *arch, const char *outdir) {
  raf_db_t db;
  if (!raf_db_init(&db, RAFPKG_MAX_PKGS)) return 2;
  if (!raf_scan_roots(&db, roots, roots_cnt, arch)) {
    raf_db_free(&db);
    return 3;
  }

  char outpath[4096];
  snprintf(outpath, sizeof(outpath), "%s/index.json", outdir ? outdir : "out");
  if (!raf_json_write_index(outpath, &db)) {
    fprintf(stderr, "rafpkg: failed to write %s\n", outpath);
    raf_db_free(&db);
    return 4;
  }

  // summary
  u32 excluded = 0;
  for (u32 i = 0; i < db.pkgs_cnt; i++) if (db.pkgs[i].excluded) excluded++;
  fprintf(stdout, "scan: pkgs=%u excluded=%u out=%s\n", db.pkgs_cnt, excluded, outpath);

  raf_db_free(&db);
  return 0;
}

static int cmd_plan(const char **roots, u32 roots_cnt, const char *arch) {
  raf_db_t db;
  if (!raf_db_init(&db, RAFPKG_MAX_PKGS)) return 2;
  if (!raf_scan_roots(&db, roots, roots_cnt, arch)) {
    raf_db_free(&db);
    return 3;
  }

  raf_map_t map;
  if (!build_map(&db, &map)) { raf_db_free(&db); return 4; }

  raf_graph_t g;
  if (!raf_graph_build(&g, &db, &map)) {
    raf_map_free(&map);
    raf_db_free(&db);
    return 5;
  }

  u32 *order = (u32*)malloc((size_t)db.pkgs_cnt * sizeof(u32));
  if (!order) {
    raf_graph_free(&g);
    raf_map_free(&map);
    raf_db_free(&db);
    return 6;
  }

  u32 n = raf_graph_toposort(&g, &db, order, db.pkgs_cnt);
  if (n == 0) {
    free(order);
    raf_graph_free(&g);
    raf_map_free(&map);
    raf_db_free(&db);
    return 7;
  }

  // print in same style as buildorder.py: "name dir"
  for (u32 i = 0; i < n; i++) {
    u32 idx = order[i];
    const raf_pkg_t *p = &db.pkgs[idx];
    printf("%-30s %s\n", p->name.s, p->dir.s);
  }

  free(order);
  raf_graph_free(&g);
  raf_map_free(&map);
  raf_db_free(&db);
  return 0;
}

static int cmd_audit(const char **roots, u32 roots_cnt, const char *arch) {
  raf_db_t db;
  if (!raf_db_init(&db, RAFPKG_MAX_PKGS)) return 2;
  if (!raf_scan_roots(&db, roots, roots_cnt, arch)) {
    raf_db_free(&db);
    return 3;
  }

  raf_map_t map;
  if (!build_map(&db, &map)) { raf_db_free(&db); return 4; }

  u32 excluded = 0, total_deps = 0, internal_edges = 0, external_deps = 0;
  for (u32 i = 0; i < db.pkgs_cnt; i++) {
    const raf_pkg_t *p = &db.pkgs[i];
    if (p->excluded) { excluded++; continue; }
    total_deps += p->dep_cnt;
    for (u32 j = 0; j < p->dep_cnt; j++) {
      u32 off = db.dep_idx[p->dep_off + j];
      const char *d = raf_db_dep_at(&db, off);
      u32 di;
      if (raf_map_get(&map, d, &di)) internal_edges++;
      else external_deps++;
    }
  }

  printf("audit: pkgs=%u excluded=%u deps=%u internal=%u external=%u\n",
         db.pkgs_cnt, excluded, total_deps, internal_edges, external_deps);

  raf_map_free(&map);
  raf_db_free(&db);
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) { usage(); return 1; }

  const char *cmd = argv[1];

  // store-dump is positional (no flags parsing)
  if (strcmp(cmd, "store-dump") == 0) {
    const char *path = (argc >= 3) ? argv[2] : NULL;
    u32 limit = 0;
    if (argc >= 4) limit = (u32)strtoul(argv[3], NULL, 10);
    return cmd_store_dump(path, limit);
  }

  const char *arch = env_or_default("TERMUX_ARCH", "aarch64");
  const char *outdir = "out";
  const char *fa_root = ".";
  int fa_depth = 6;
  const char *fa_cache = NULL; // NULL=default, ""=disabled
  int fa_no_cache = 0;

  // defaults
  const char *roots_default[] = {"packages", "x11-packages", "root-packages"};
  const char *roots[32];
  u32 roots_cnt = 0;

  char roots_buf[1024];
  roots_buf[0] = 0;

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--arch") == 0 && i + 1 < argc) {
      arch = argv[++i];
    } else if (strcmp(argv[i], "--roots") == 0 && i + 1 < argc) {
      strncpy(roots_buf, argv[++i], sizeof(roots_buf) - 1);
      roots_buf[sizeof(roots_buf) - 1] = 0;
    } else if (strcmp(argv[i], "--outdir") == 0 && i + 1 < argc) {
      outdir = argv[++i];
    } else if (strcmp(argv[i], "--root") == 0 && i + 1 < argc) {
      fa_root = argv[++i];
    } else if (strcmp(argv[i], "--depth") == 0 && i + 1 < argc) {
      fa_depth = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--cache") == 0 && i + 1 < argc) {
      fa_cache = argv[++i];
    } else if (strcmp(argv[i], "--no-cache") == 0) {
      fa_no_cache = 1;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage();
      return 0;
    } else {
      fprintf(stderr, "rafpkg: unknown arg: %s\n", argv[i]);
      return 1;
    }
  }

  if (roots_buf[0]) {
    roots_cnt = split_roots(roots_buf, roots, 32);
  } else {
    for (u32 i = 0; i < 3; i++) roots[roots_cnt++] = roots_default[i];
  }

  if (roots_cnt == 0) {
    fprintf(stderr, "rafpkg: no roots\n");
    return 1;
  }

  if (strcmp(cmd, "scan") == 0) {
    return cmd_scan(roots, roots_cnt, arch, outdir);
  } else if (strcmp(cmd, "plan") == 0) {
    return cmd_plan(roots, roots_cnt, arch);
  } else if (strcmp(cmd, "audit") == 0) {
    return cmd_audit(roots, roots_cnt, arch);
  } else if (strcmp(cmd, "cpu") == 0) {
    return cmd_cpu();
  } else if (strcmp(cmd, "fileaudit") == 0) {
    return cmd_fileaudit(fa_root, fa_depth, fa_cache, fa_no_cache);
  }

  usage();
  return 1;
}
