// file: rafpkg_parse.c
// RAFAELIA :: parse termux build.sh variables (static)
// SPDX-License-Identifier: MIT

#include "rafpkg_parse.h"
#include "rafpkg_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int startswith(const char *s, const char *pfx) {
  while (*pfx) {
    if (*s++ != *pfx++) return 0;
  }
  return 1;
}

static void strip_quotes(char *s) {
  // remove any ', ", \n \r characters by shifting (in place)
  char *r = s;
  char *w = s;
  while (*r) {
    char c = *r++;
    if (c == '\'' || c == '"' || c == '\n' || c == '\r') continue;
    *w++ = c;
  }
  *w = 0;
}

static void remove_paren_expr(char *s) {
  // remove "(...)" blocks
  char *r = s;
  char *w = s;
  while (*r) {
    if (*r == '(') {
      r++;
      while (*r && *r != ')') r++;
      if (*r == ')') r++;
      continue;
    }
    *w++ = *r++;
  }
  *w = 0;
}

static void replace_arch_token(char *s, const char *arch_dash) {
  // replace all occurrences of ${TERMUX_ARCH/_/-} with arch_dash
  const char *tok = "${TERMUX_ARCH/_/-}";
  size_t tlen = strlen(tok);
  size_t alen = strlen(arch_dash);
  // simple single-pass rebuild into temp if needed
  char tmp[RAFPKG_MAX_LINE];
  size_t out = 0;
  for (size_t i = 0; s[i] && out + 1 < sizeof(tmp); ) {
    if (strncmp(&s[i], tok, tlen) == 0) {
      for (size_t k = 0; k < alen && out + 1 < sizeof(tmp); k++) tmp[out++] = arch_dash[k];
      i += tlen;
    } else {
      tmp[out++] = s[i++];
    }
  }
  tmp[out] = 0;
  strncpy(s, tmp, RAFPKG_MAX_LINE - 1);
  s[RAFPKG_MAX_LINE - 1] = 0;
}

static void clean_dep(char *s, const char *arch_dash) {
  strip_quotes(s);
  remove_paren_expr(s);
  replace_arch_token(s, arch_dash);
  // trim
  char *t = raf_trim(s);
  if (t != s) memmove(s, t, strlen(t) + 1);
}

static int arch_in_list(const char *list, const char *arch) {
  // list is comma-separated
  char buf[RAFPKG_MAX_LINE];
  strncpy(buf, list, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  strip_quotes(buf);
  char *p = buf;
  while (*p) {
    // token ends at comma
    char *comma = strchr(p, ',');
    if (comma) *comma = 0;
    char *tok = raf_trim(p);
    if (tok && tok[0]) {
      if (raf_streq(tok, arch)) return 1;
    }
    if (!comma) break;
    p = comma + 1;
  }
  return 0;
}


// gather antideps first, then deps, then subtract by skipping adds
// (simple: we store antideps in a small array and filter)
typedef struct {
  char *items[RAFPKG_MAX_DEPS];
  u32 cnt;
} anti_list_t;

static void anti_add(anti_list_t *a, const char *s) {
  if (!a || !s || !s[0]) return;
  if (a->cnt >= RAFPKG_MAX_DEPS) return;
  // dedup
  for (u32 i = 0; i < a->cnt; i++) if (raf_streq(a->items[i], s)) return;
  size_t n = strlen(s);
  char *p = (char*)malloc(n + 1);
  if (!p) return;
  memcpy(p, s, n + 1);
  a->items[a->cnt++] = p;
}

static int anti_has(const anti_list_t *a, const char *s) {
  if (!a || !s) return 0;
  for (u32 i = 0; i < a->cnt; i++) if (raf_streq(a->items[i], s)) return 1;
  return 0;
}

static void anti_free(anti_list_t *a) {
  if (!a) return;
  for (u32 i = 0; i < a->cnt; i++) free(a->items[i]);
  a->cnt = 0;
}

static void parse_anti_list(anti_list_t *a, const char *value, const char *arch_dash) {
  char buf[RAFPKG_MAX_LINE];
  strncpy(buf, value, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  strip_quotes(buf);
  char *p = buf;
  while (*p) {
    char *q = p;
    while (*q && *q != ',' && *q != '|') q++;
    char saved = *q;
    *q = 0;
    char dep[RAFPKG_MAX_LINE];
    strncpy(dep, p, sizeof(dep) - 1);
    dep[sizeof(dep) - 1] = 0;
    clean_dep(dep, arch_dash);
    if (dep[0]) anti_add(a, dep);
    if (saved == 0) break;
    p = q + 1;
  }
}

int raf_parse_build_sh(raf_db_t *db, u32 pkg_idx, const char *build_sh_path, const char *arch) {
  if (!db || !build_sh_path || !arch) return 0;

  // arch_dash: TERMUX uses x86_64 -> x86-64 substitution in buildorder.py
  char arch_dash[64];
  strncpy(arch_dash, arch, sizeof(arch_dash) - 1);
  arch_dash[sizeof(arch_dash) - 1] = 0;
  if (raf_streq(arch_dash, "x86_64")) strncpy(arch_dash, "x86-64", sizeof(arch_dash) - 1);
  for (char *p = arch_dash; *p; p++) if (*p == '_') *p = '-';

  FILE *f = fopen(build_sh_path, "r");
  if (!f) return 0;

  // pass 1: antideps + excluded arches
  anti_list_t anti;
  memset(&anti, 0, sizeof(anti));
  char line[RAFPKG_MAX_LINE];

  while (fgets(line, sizeof(line), f)) {
    if (startswith(line, "TERMUX_PKG_ANTI_BUILD_DEPENDS")) {
      char *eq = strchr(line, '=');
      if (!eq) continue;
      eq++;
      parse_anti_list(&anti, eq, arch_dash);
    } else if (startswith(line, "TERMUX_PKG_EXCLUDED_ARCHES") || startswith(line, "TERMUX_SUBPKG_EXCLUDED_ARCHES")) {
      char *eq = strchr(line, '=');
      if (!eq) continue;
      eq++;
      char tmp[RAFPKG_MAX_LINE];
      strncpy(tmp, eq, sizeof(tmp) - 1);
      tmp[sizeof(tmp) - 1] = 0;
      strip_quotes(tmp);
      char *t = raf_trim(tmp);
      if (t && t[0] && arch_in_list(t, arch)) {
        db->pkgs[pkg_idx].excluded = 1;
      }
    }
  }

  // pass 2: deps (filter antideps)
  rewind(f);
  raf_db_pkg_deps_begin(db, pkg_idx);

  while (fgets(line, sizeof(line), f)) {
    int is_dep = 0;
    if (startswith(line, "TERMUX_PKG_DEPENDS")) is_dep = 1;
    else if (startswith(line, "TERMUX_PKG_BUILD_DEPENDS")) is_dep = 1;
    else if (startswith(line, "TERMUX_PKG_DEVPACKAGE_DEPENDS")) is_dep = 1;
    else if (startswith(line, "TERMUX_SUBPKG_DEPENDS")) is_dep = 1;
    if (!is_dep) continue;

    char *eq = strchr(line, '=');
    if (!eq) continue;
    eq++;

    // parse list into temporary, then add if not anti
    char buf[RAFPKG_MAX_LINE];
    strncpy(buf, eq, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    strip_quotes(buf);

    char *p = buf;
    while (*p) {
      char *q = p;
      while (*q && *q != ',' && *q != '|') q++;
      char saved = *q;
      *q = 0;
      char dep[RAFPKG_MAX_LINE];
      strncpy(dep, p, sizeof(dep) - 1);
      dep[sizeof(dep) - 1] = 0;
      clean_dep(dep, arch_dash);
      if (dep[0] && !anti_has(&anti, dep)) raf_db_pkg_dep_add(db, pkg_idx, dep);
      if (saved == 0) break;
      p = q + 1;
    }
  }

  fclose(f);
  anti_free(&anti);
  raf_db_pkg_deps_end(db, pkg_idx);
  return 1;
}
