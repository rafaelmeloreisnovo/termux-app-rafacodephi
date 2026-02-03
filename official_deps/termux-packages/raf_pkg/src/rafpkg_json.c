// file: rafpkg_json.c
// RAFAELIA :: tiny JSON writer
// SPDX-License-Identifier: MIT

#include "rafpkg_json.h"
#include <stdio.h>
#include <string.h>

static void json_escape(FILE *f, const char *s) {
  for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
    unsigned char c = *p;
    if (c == '"') { fputs("\\\"", f); }
    else if (c == '\\') { fputs("\\\\", f); }
    else if (c < 0x20) { fprintf(f, "\\u%04x", (unsigned)c); }
    else { fputc((int)c, f); }
  }
}

int raf_json_write_index(const char *path, const raf_db_t *db) {
  if (!path || !db) return 0;
  FILE *f = fopen(path, "w");
  if (!f) return 0;

  fprintf(f, "{\n");
  fprintf(f, "  \"pkgs\": [\n");
  for (u32 i = 0; i < db->pkgs_cnt; i++) {
    const raf_pkg_t *p = &db->pkgs[i];
    fprintf(f, "    {\"name\":\""); json_escape(f, p->name.s);
    fprintf(f, "\",\"dir\":\""); json_escape(f, p->dir.s);
    fprintf(f, "\",\"excluded\":%s,\"deps\":[", p->excluded ? "true" : "false");

    for (u32 j = 0; j < p->dep_cnt; j++) {
      u32 off = db->dep_idx[p->dep_off + j];
      const char *d = raf_db_dep_at(db, off);
      fprintf(f, "%s\"", (j ? "," : ""));
      json_escape(f, d);
      fputc('"', f);
    }

    fprintf(f, "]}");
    fprintf(f, "%s\n", (i + 1 == db->pkgs_cnt) ? "" : ",");
  }
  fprintf(f, "  ]\n");
  fprintf(f, "}\n");
  fclose(f);
  return 1;
}
