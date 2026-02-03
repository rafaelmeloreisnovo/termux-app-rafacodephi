// raf_pkg_cli.c
// RAFAELIA :: rafpkg — deterministic Termux packaging scan/plan/audit
// build: clang -O2 -std=c11 -Wall -Wextra raf_pkg_cli.c -o rafpkg

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// -----------------------------
// tiny types/helpers
// -----------------------------

typedef uint32_t u32;
typedef uint64_t u64;

enum { MAX_LINE = 4096 };

static int streq(const char *a, const char *b){
  return a && b && strcmp(a,b)==0;
}

static int starts_with(const char *s, const char *p){
  size_t n = strlen(p);
  return strncmp(s, p, n) == 0;
}

static void *xmalloc(size_t n){
  void *p = malloc(n);
  if(!p){ fprintf(stderr, "ERROR: OOM\n"); exit(2); }
  return p;
}

static void *xrealloc(void *p, size_t n){
  void *q = realloc(p, n);
  if(!q){ fprintf(stderr, "ERROR: OOM\n"); exit(2); }
  return q;
}

static char *xstrdup(const char *s){
  size_t n = strlen(s);
  char *p = (char*)xmalloc(n+1);
  memcpy(p, s, n+1);
  return p;
}

static int is_dir(const char *path){
  struct stat st;
  if(stat(path, &st) != 0) return 0;
  return S_ISDIR(st.st_mode);
}

static int is_file(const char *path){
  struct stat st;
  if(stat(path, &st) != 0) return 0;
  return S_ISREG(st.st_mode);
}

static void path_join(char *out, size_t cap, const char *a, const char *b){
  size_t na = strlen(a);
  size_t nb = strlen(b);
  if(na + 1 + nb + 1 > cap){
    fprintf(stderr, "ERROR: path too long\n");
    exit(2);
  }
  memcpy(out, a, na);
  if(na && a[na-1] != '/') out[na++] = '/';
  memcpy(out+na, b, nb);
  out[na+nb] = 0;
}

static void trim_quotes_nl(char *s){
  // remove surrounding quotes and trailing newline
  size_t n = strlen(s);
  while(n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = 0;
  // strip quotes anywhere (like python remove_nl_and_quotes)
  for(size_t i=0;i<n;i++){
    if(s[i] == '\"' || s[i] == '\'') s[i] = ' ';
  }
}

static void trim_spaces(char *s){
  // in-place trim
  size_t n = strlen(s);
  size_t i = 0;
  while(i < n && (s[i] == ' ' || s[i] == '\t')) i++;
  size_t j = n;
  while(j > i && (s[j-1] == ' ' || s[j-1] == '\t')) j--;
  if(i>0) memmove(s, s+i, j-i);
  s[j-i] = 0;
}

static void remove_paren_qualifiers(char *s){
  // remove (...)
  char *w = s;
  int depth = 0;
  for(char *p=s; *p; p++){
    if(*p == '(') { depth++; continue; }
    if(*p == ')') { if(depth>0) depth--; continue; }
    if(depth==0) *w++ = *p;
  }
  *w = 0;
}

static void subst_arch_token(char *s, const char *arch){
  // replace ${TERMUX_ARCH/_/-} with arch (with '_' -> '-')
  const char *tok = "${TERMUX_ARCH/_/-}";
  char arch2[64];
  size_t al = strlen(arch);
  if(al >= sizeof(arch2)) al = sizeof(arch2)-1;
  memcpy(arch2, arch, al);
  arch2[al] = 0;
  for(size_t i=0;i<al;i++) if(arch2[i]=='_') arch2[i]='-';

  char buf[4096];
  buf[0]=0;
  const char *p = s;
  while(1){
    const char *q = strstr(p, tok);
    if(!q){
      strncat(buf, p, sizeof(buf)-strlen(buf)-1);
      break;
    }
    strncat(buf, p, (size_t)(q-p) > sizeof(buf)-strlen(buf)-1 ? sizeof(buf)-strlen(buf)-1 : (size_t)(q-p));
    strncat(buf, arch2, sizeof(buf)-strlen(buf)-1);
    p = q + strlen(tok);
  }
  strncpy(s, buf, 4095);
  s[4095] = 0;
}

// -----------------------------
// package model
// -----------------------------

typedef struct {
  char *name;
  char *dir;
  char **deps;      u32 deps_n;
  char **antideps;  u32 antideps_n;
  char **ex_arches; u32 ex_arches_n;
} pkg_t;

typedef struct {
  pkg_t *v;
  u32 n;
  u32 cap;
} pkg_list_t;

static void pkg_list_push(pkg_list_t *L, pkg_t p){
  if(L->n == L->cap){
    L->cap = L->cap ? (L->cap*2) : 256;
    L->v = (pkg_t*)xrealloc(L->v, (size_t)L->cap * sizeof(pkg_t));
  }
  L->v[L->n++] = p;
}

static void str_list_push(char ***v, u32 *n, u32 *cap, const char *s){
  if(!s || !*s) return;
  if(*n == *cap){
    *cap = *cap ? (*cap*2) : 8;
    *v = (char**)xrealloc(*v, (size_t)(*cap) * sizeof(char*));
  }
  (*v)[(*n)++] = xstrdup(s);
}

static int str_list_contains(char **v, u32 n, const char *s){
  for(u32 i=0;i<n;i++) if(streq(v[i], s)) return 1;
  return 0;
}

// split dependencies on ',' and '|' (like python buildorder)
static void parse_dep_string(char *deps_string, const char *arch, char ***out, u32 *out_n){
  // normalize
  trim_quotes_nl(deps_string);
  remove_paren_qualifiers(deps_string);
  subst_arch_token(deps_string, arch);

  // split
  u32 cap = 0;
  char **arr = NULL;
  u32 n = 0;

  char *p = deps_string;
  while(*p){
    // skip separators/spaces
    while(*p == ' ' || *p == '\t' || *p == ',' || *p == '|') p++;
    if(!*p) break;
    char *start = p;
    while(*p && *p != ',' && *p != '|') p++;
    char tmp[256];
    size_t len = (size_t)(p - start);
    if(len >= sizeof(tmp)) len = sizeof(tmp)-1;
    memcpy(tmp, start, len);
    tmp[len] = 0;
    trim_spaces(tmp);
    if(tmp[0]){
      // de-dup
      if(!str_list_contains(arr, n, tmp)){
        str_list_push(&arr, &n, &cap, tmp);
      }
    }
  }

  *out = arr;
  *out_n = n;
}

static void parse_build_sh(const char *build_sh, pkg_t *pkg, const char *arch){
  FILE *f = fopen(build_sh, "rb");
  if(!f) return;
  char line[MAX_LINE];
  while(fgets(line, sizeof(line), f)){
    if(starts_with(line, "TERMUX_PKG_DEPENDS") ||
       starts_with(line, "TERMUX_PKG_BUILD_DEPENDS") ||
       starts_with(line, "TERMUX_SUBPKG_DEPENDS") ||
       starts_with(line, "TERMUX_PKG_DEVPACKAGE_DEPENDS")){
      char *eq = strchr(line, '=');
      if(!eq) continue;
      eq++;
      char buf[MAX_LINE];
      strncpy(buf, eq, sizeof(buf)-1);
      buf[sizeof(buf)-1] = 0;
      char **deps=NULL; u32 deps_n=0;
      parse_dep_string(buf, arch, &deps, &deps_n);
      // append into pkg->deps
      for(u32 i=0;i<deps_n;i++){
        // ignore version qualifiers already removed
        if(!str_list_contains(pkg->deps, pkg->deps_n, deps[i])){
          u32 cap = pkg->deps_n;
          // cap not stored; use realloc each time (small list)
          pkg->deps = (char**)xrealloc(pkg->deps, (size_t)(pkg->deps_n + 1) * sizeof(char*));
          pkg->deps[pkg->deps_n++] = deps[i];
        } else {
          free(deps[i]);
        }
      }
      free(deps);
    } else if(starts_with(line, "TERMUX_PKG_ANTI_BUILD_DEPENDS")){
      char *eq = strchr(line, '=');
      if(!eq) continue;
      eq++;
      char buf[MAX_LINE];
      strncpy(buf, eq, sizeof(buf)-1);
      buf[sizeof(buf)-1] = 0;
      char **deps=NULL; u32 deps_n=0;
      parse_dep_string(buf, arch, &deps, &deps_n);
      for(u32 i=0;i<deps_n;i++){
        if(!str_list_contains(pkg->antideps, pkg->antideps_n, deps[i])){
          pkg->antideps = (char**)xrealloc(pkg->antideps, (size_t)(pkg->antideps_n + 1) * sizeof(char*));
          pkg->antideps[pkg->antideps_n++] = deps[i];
        } else {
          free(deps[i]);
        }
      }
      free(deps);
    } else if(starts_with(line, "TERMUX_PKG_EXCLUDED_ARCHES") ||
              starts_with(line, "TERMUX_SUBPKG_EXCLUDED_ARCHES")){
      char *eq = strchr(line, '=');
      if(!eq) continue;
      eq++;
      char buf[MAX_LINE];
      strncpy(buf, eq, sizeof(buf)-1);
      buf[sizeof(buf)-1] = 0;
      trim_quotes_nl(buf);
      // split by ','
      char *p = buf;
      while(*p){
        while(*p==' ' || *p=='\t' || *p==',') p++;
        if(!*p) break;
        char *st = p;
        while(*p && *p!=',') p++;
        char tmp[64];
        size_t len = (size_t)(p-st);
        if(len >= sizeof(tmp)) len = sizeof(tmp)-1;
        memcpy(tmp, st, len);
        tmp[len]=0;
        trim_spaces(tmp);
        if(tmp[0] && !str_list_contains(pkg->ex_arches, pkg->ex_arches_n, tmp)){
          pkg->ex_arches = (char**)xrealloc(pkg->ex_arches, (size_t)(pkg->ex_arches_n + 1) * sizeof(char*));
          pkg->ex_arches[pkg->ex_arches_n++] = xstrdup(tmp);
        }
      }
    }
  }
  fclose(f);

  // apply antideps removal from deps
  if(pkg->antideps_n){
    u32 w=0;
    for(u32 i=0;i<pkg->deps_n;i++){
      if(str_list_contains(pkg->antideps, pkg->antideps_n, pkg->deps[i])){
        free(pkg->deps[i]);
      } else {
        pkg->deps[w++] = pkg->deps[i];
      }
    }
    pkg->deps_n = w;
    if(w==0){ free(pkg->deps); pkg->deps=NULL; }
  }
}

static int pkg_excluded_for_arch(const pkg_t *p, const char *arch){
  for(u32 i=0;i<p->ex_arches_n;i++){
    if(streq(p->ex_arches[i], arch)) return 1;
  }
  return 0;
}

// -----------------------------
// scanning package directories
// -----------------------------

static void scan_root_dir(pkg_list_t *L, const char *root, const char *arch){
  if(!is_dir(root)) return;
  DIR *d = opendir(root);
  if(!d) return;
  struct dirent *e;
  while((e = readdir(d))){
    if(e->d_name[0]=='.') continue;
    char pkgdir[4096];
    path_join(pkgdir, sizeof(pkgdir), root, e->d_name);
    if(!is_dir(pkgdir)) continue;
    char buildsh[4096];
    path_join(buildsh, sizeof(buildsh), pkgdir, "build.sh");
    if(!is_file(buildsh)) continue;

    pkg_t p;
    memset(&p, 0, sizeof(p));
    p.name = xstrdup(e->d_name);
    p.dir  = xstrdup(pkgdir);

    parse_build_sh(buildsh, &p, arch);

    if(pkg_excluded_for_arch(&p, arch)){
      // drop
      free(p.name); free(p.dir);
      for(u32 i=0;i<p.deps_n;i++) free(p.deps[i]); free(p.deps);
      for(u32 i=0;i<p.antideps_n;i++) free(p.antideps[i]); free(p.antideps);
      for(u32 i=0;i<p.ex_arches_n;i++) free(p.ex_arches[i]); free(p.ex_arches);
      continue;
    }
    pkg_list_push(L, p);
  }
  closedir(d);
}

static void parse_roots(char *csv, char ***out, u32 *out_n){
  char **arr = NULL; u32 n=0; u32 cap=0;
  char *p = csv;
  while(*p){
    while(*p==',' || *p==' ' || *p=='\t') p++;
    if(!*p) break;
    char *st=p;
    while(*p && *p!=',') p++;
    char tmp[512];
    size_t len=(size_t)(p-st);
    if(len>=sizeof(tmp)) len=sizeof(tmp)-1;
    memcpy(tmp, st, len);
    tmp[len]=0;
    trim_spaces(tmp);
    if(tmp[0]) str_list_push(&arr, &n, &cap, tmp);
  }
  *out=arr; *out_n=n;
}

// -----------------------------
// graph + topo sort (stable)
// -----------------------------

typedef struct {
  u32 *adj; u32 adj_n; // indices
} node_edges_t;

typedef struct {
  node_edges_t *edges;
  u32 *indeg;
} graph_t;

static int cmp_name(const void *A, const void *B, void *ctx){
  const pkg_list_t *L = (const pkg_list_t*)ctx;
  u32 a = *(const u32*)A;
  u32 b = *(const u32*)B;
  return strcmp(L->v[a].name, L->v[b].name);
}

static int find_pkg_index(const pkg_list_t *L, const char *name){
  for(u32 i=0;i<L->n;i++) if(streq(L->v[i].name, name)) return (int)i;
  return -1;
}

static graph_t build_graph(const pkg_list_t *L){
  graph_t g;
  g.edges = (node_edges_t*)xmalloc((size_t)L->n * sizeof(node_edges_t));
  g.indeg = (u32*)xmalloc((size_t)L->n * sizeof(u32));
  memset(g.edges, 0, (size_t)L->n * sizeof(node_edges_t));
  memset(g.indeg, 0, (size_t)L->n * sizeof(u32));

  for(u32 i=0;i<L->n;i++){
    // add edges to known deps
    for(u32 k=0;k<L->v[i].deps_n;k++){
      const char *dep = L->v[i].deps[k];
      if(streq(dep, L->v[i].name)) continue;
      int j = find_pkg_index(L, dep);
      if(j < 0) continue; // external
      // edge: j -> i  (dep must be built before i)
      node_edges_t *E = &g.edges[(u32)j];
      E->adj = (u32*)xrealloc(E->adj, (size_t)(E->adj_n + 1) * sizeof(u32));
      E->adj[E->adj_n++] = i;
      g.indeg[i]++;
    }
  }
  return g;
}

static void free_graph(graph_t *g, u32 n){
  for(u32 i=0;i<n;i++) free(g->edges[i].adj);
  free(g->edges);
  free(g->indeg);
}

static int topo_sort_plan(const pkg_list_t *L, const graph_t *g, u32 **out_order, u32 *out_n){
  u32 n = L->n;
  u32 *ind = (u32*)xmalloc((size_t)n * sizeof(u32));
  memcpy(ind, g->indeg, (size_t)n * sizeof(u32));

  u32 *order = (u32*)xmalloc((size_t)n * sizeof(u32));
  u32 outc = 0;
  uint8_t *used = (uint8_t*)xmalloc(n);
  memset(used, 0, n);

  for(u32 step=0; step<n; step++){
    int best = -1;
    for(u32 i=0;i<n;i++){
      if(used[i]) continue;
      if(ind[i] != 0) continue;
      if(best < 0 || strcmp(L->v[i].name, L->v[(u32)best].name) < 0) best = (int)i;
    }
    if(best < 0){
      // cycle
      fprintf(stderr, "ERROR: cycle detected (remaining nodes with indegree>0)\n");
      for(u32 i=0;i<n;i++) if(!used[i] && ind[i]){
        fprintf(stderr, "  %s indeg=%u\n", L->v[i].name, ind[i]);
      }
      free(ind); free(order); free(used);
      return 0;
    }
    u32 u = (u32)best;
    used[u] = 1;
    order[outc++] = u;
    // remove edges
    node_edges_t *E = &g->edges[u];
    for(u32 k=0;k<E->adj_n;k++){
      u32 v = E->adj[k];
      if(ind[v]) ind[v]--;
    }
  }

  free(ind);
  free(used);
  *out_order = order;
  *out_n = outc;
  return 1;
}

// -----------------------------
// JSON writer (minimal)
// -----------------------------

static void json_escape(FILE *f, const char *s){
  fputc('"', f);
  for(const unsigned char *p=(const unsigned char*)s; *p; p++){
    unsigned char c=*p;
    if(c=='\\' || c=='"') { fputc('\\', f); fputc(c, f); }
    else if(c=='\n') fputs("\\n", f);
    else if(c=='\r') fputs("\\r", f);
    else if(c=='\t') fputs("\\t", f);
    else if(c < 0x20) fprintf(f, "\\u%04x", (unsigned)c);
    else fputc(c, f);
  }
  fputc('"', f);
}

static void write_index_json(const char *path, const pkg_list_t *L){
  FILE *f = fopen(path, "wb");
  if(!f){ fprintf(stderr, "WARN: cannot write %s\n", path); return; }
  fputs("{\n  \"packages\": [\n", f);
  for(u32 i=0;i<L->n;i++){
    const pkg_t *p=&L->v[i];
    fputs("    {\n      \"name\": ", f); json_escape(f, p->name); fputs(",\n", f);
    fputs("      \"dir\": ", f); json_escape(f, p->dir); fputs(",\n", f);
    fputs("      \"deps\": [", f);
    for(u32 k=0;k<p->deps_n;k++){
      if(k) fputs(", ", f);
      json_escape(f, p->deps[k]);
    }
    fputs("]\n    }", f);
    if(i+1<L->n) fputs(",", f);
    fputs("\n", f);
  }
  fputs("  ]\n}\n", f);
  fclose(f);
}

static void write_audit_md(const char *path, const pkg_list_t *L){
  FILE *f = fopen(path, "wb");
  if(!f){ fprintf(stderr, "WARN: cannot write %s\n", path); return; }
  u64 deps_total=0;
  for(u32 i=0;i<L->n;i++) deps_total += (u64)L->v[i].deps_n;
  fprintf(f, "# rafpkg audit\n\n");
  fprintf(f, "- packages: %u\n", L->n);
  fprintf(f, "- deps_total: %llu\n", (unsigned long long)deps_total);
  fprintf(f, "\n## top deps samples\n\n");
  for(u32 i=0;i<L->n && i<20;i++){
    fprintf(f, "- %s: %u deps\n", L->v[i].name, L->v[i].deps_n);
  }
  fclose(f);
}

// -----------------------------
// CLI
// -----------------------------

static void usage(void){
  fprintf(stderr,
    "rafpkg (RAFAELIA)\n"
    "Usage:\n"
    "  rafpkg scan  --roots a,b,c [--out DIR]\n"
    "  rafpkg plan  --roots a,b,c\n"
    "  rafpkg audit --roots a,b,c\n"
    "\nEnv:\n"
    "  TERMUX_ARCH (default: aarch64)\n");
  exit(1);
}

static const char *get_arch(void){
  const char *a = getenv("TERMUX_ARCH");
  if(!a || !*a) a = "aarch64";
  return a;
}

static pkg_list_t load_pkgs(const char *roots_csv){
  const char *arch = get_arch();

  char *roots_buf = xstrdup(roots_csv);
  char **roots=NULL; u32 roots_n=0;
  parse_roots(roots_buf, &roots, &roots_n);
  free(roots_buf);

  pkg_list_t L; memset(&L, 0, sizeof(L));
  for(u32 i=0;i<roots_n;i++){
    scan_root_dir(&L, roots[i], arch);
    free(roots[i]);
  }
  free(roots);

  return L;
}

static void free_pkgs(pkg_list_t *L){
  for(u32 i=0;i<L->n;i++){
    pkg_t *p=&L->v[i];
    free(p->name);
    free(p->dir);
    for(u32 k=0;k<p->deps_n;k++) free(p->deps[k]);
    free(p->deps);
    for(u32 k=0;k<p->antideps_n;k++) free(p->antideps[k]);
    free(p->antideps);
    for(u32 k=0;k<p->ex_arches_n;k++) free(p->ex_arches[k]);
    free(p->ex_arches);
  }
  free(L->v);
  memset(L, 0, sizeof(*L));
}

int main(int argc, char **argv){
  if(argc < 2) usage();
  const char *cmd = argv[1];

  const char *roots = NULL;
  const char *outdir = "out";

  for(int i=2;i<argc;i++){
    if(streq(argv[i], "--roots") && i+1<argc){ roots = argv[++i]; }
    else if(streq(argv[i], "--out") && i+1<argc){ outdir = argv[++i]; }
    else usage();
  }
  if(!roots) roots = "packages,x11-packages,root-packages";

  if(streq(cmd, "scan")){
    pkg_list_t L = load_pkgs(roots);

    // ensure outdir exists (best-effort)
    (void)mkdir(outdir, 0755);

    char p1[4096];
    snprintf(p1, sizeof(p1), "%s/index.json", outdir);
    write_index_json(p1, &L);

    char p2[4096];
    snprintf(p2, sizeof(p2), "%s/audit.md", outdir);
    write_audit_md(p2, &L);

    fprintf(stderr, "OK: scanned %u packages (arch=%s)\n", L.n, get_arch());

    free_pkgs(&L);
    return 0;

  } else if(streq(cmd, "plan")){
    pkg_list_t L = load_pkgs(roots);
    graph_t g = build_graph(&L);
    u32 *order=NULL; u32 on=0;
    int ok = topo_sort_plan(&L, &g, &order, &on);
    if(!ok){
      free_graph(&g, L.n);
      free_pkgs(&L);
      return 3;
    }

    for(u32 i=0;i<on;i++){
      pkg_t *p = &L.v[order[i]];
      // match buildorder.py format: name padded to 30 chars
      printf("%-30s %s\n", p->name, p->dir);
    }

    free(order);
    free_graph(&g, L.n);
    free_pkgs(&L);
    return 0;

  } else if(streq(cmd, "audit")){
    pkg_list_t L = load_pkgs(roots);
    u64 deps_total=0;
    for(u32 i=0;i<L.n;i++) deps_total += (u64)L.v[i].deps_n;
    fprintf(stderr, "rafpkg audit: packages=%u deps_total=%llu arch=%s\n",
            L.n, (unsigned long long)deps_total, get_arch());

    // quick unknown deps count
    u64 unknown=0;
    for(u32 i=0;i<L.n;i++){
      for(u32 k=0;k<L.v[i].deps_n;k++){
        if(find_pkg_index(&L, L.v[i].deps[k]) < 0) unknown++;
      }
    }
    fprintf(stderr, "unknown_deps (external)=%llu\n", (unsigned long long)unknown);

    free_pkgs(&L);
    return 0;
  }

  usage();
  return 1;
}
