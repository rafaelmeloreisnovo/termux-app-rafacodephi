/* raf_termux_packages.c
   IDs deterministicos de pacotes Termux (sem libc, gerado)
*/

#include "raf_termux_packages.h"

static const raf_termux_pkg_id_t g_termux_pkgs[] = {
__PKG_ROWS__
};

static const raf_termux_pkg_table_t g_table = {
  g_termux_pkgs,
  __PKG_COUNT__u,
  2166136261u,
  0x52414650u
};

const raf_termux_pkg_table_t *RmR_termux_packages(void){
  return &g_table;
}
