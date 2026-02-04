/* raf_termux_essentials.c
   Núcleo determinístico: shell + 50 ferramentas essenciais (sem libc)
*/

#include "raf_termux_essentials.h"

static const raf_essential_t g_essentials[RAF_ESSENTIAL_MAX] = {
  { 0x2c96b2a1u, 2u, 1u, 0x00000001u, 100u }, /* sh */
  { 0x3d8f2aa9u, 2u, 1u, 0x00000002u, 80u }, /* ls */
  { 0x1f1f7c1bu, 2u, 1u, 0x00000004u, 70u }, /* cp */
  { 0x1f20f912u, 2u, 1u, 0x00000008u, 70u }, /* mv */
  { 0x1f1f7c32u, 2u, 1u, 0x00000010u, 70u }, /* rm */
  { 0x2a1e4d77u, 4u, 1u, 0x00000020u, 70u }, /* grep */
  { 0x2fd3e77bu, 4u, 1u, 0x00000040u, 60u }, /* sed */
  { 0x3b7fbb4cu, 3u, 1u, 0x00000080u, 60u }, /* awk */
  { 0x2a1f5b19u, 3u, 1u, 0x00000100u, 60u }, /* tar */
  { 0x2d6a0d5du, 2u, 1u, 0x00000200u, 60u }, /* ps */
  { 0x3b9c9ee6u, 4u, 2u, 0x00001000u, 75u }, /* curl */
  { 0x3f51f1a1u, 3u, 2u, 0x00002000u, 70u }, /* ssh */
  { 0x2b2be7ceu, 4u, 2u, 0x00004000u, 65u }, /* wget */
  { 0x1f3a5a2eu, 3u, 3u, 0x00008000u, 70u }, /* git */
  { 0x2e6a1c19u, 4u, 3u, 0x00010000u, 65u }, /* make */
  { 0x3f3b8e12u, 5u, 3u, 0x00020000u, 65u }, /* clang */
  { 0x1f316e2du, 3u, 3u, 0x00040000u, 65u }, /* gcc */
  { 0x2b71c6c7u, 8u, 3u, 0x00080000u, 60u }, /* binutils */
  { 0x34bc7a11u, 7u, 3u, 0x00100000u, 60u }, /* openssl */
  { 0x1d2e4f36u, 4u, 3u, 0x00200000u, 55u }, /* nano */
  { 0x3b2feaa2u, 3u, 3u, 0x00400000u, 55u }, /* vim */
  { 0x2c7e2d11u, 6u, 4u, 0x00800000u, 60u }, /* python */
  { 0x2d8c53a1u, 4u, 4u, 0x01000000u, 55u }, /* node */
  { 0x2f7f1c99u, 2u, 4u, 0x02000000u, 50u }, /* go */
  { 0x3a3e1c44u, 4u, 4u, 0x04000000u, 50u }, /* rust */
  { 0x1d5f2c18u, 3u, 4u, 0x08000000u, 50u }, /* php */
  { 0x2f2a3d77u, 3u, 4u, 0x10000000u, 50u }, /* lua */
  { 0x2c9f0f55u, 5u, 4u, 0x20000000u, 50u }, /* ruby */
  { 0x2d7a1d40u, 6u, 4u, 0x40000000u, 50u }, /* cmake */
  { 0x3b0f2a88u, 4u, 4u, 0x80000000u, 50u }, /* bash */
  { 0x1f9a2c88u, 6u, 5u, 0x00000001u, 45u }, /* tmux */
  { 0x2f8a3d10u, 6u, 5u, 0x00000002u, 45u }, /* screen */
  { 0x1b9f2a11u, 5u, 5u, 0x00000004u, 45u }, /* htop */
  { 0x3d1a2b90u, 4u, 5u, 0x00000008u, 45u }, /* iperf */
  { 0x2a0f1d42u, 5u, 5u, 0x00000010u, 45u }, /* rsync */
  { 0x1b3a7c12u, 4u, 5u, 0x00000020u, 45u }, /* zip */
  { 0x1b3a7c13u, 6u, 5u, 0x00000040u, 45u }, /* unzip */
  { 0x2d1a3c88u, 7u, 5u, 0x00000080u, 45u }, /* sqlite */
  { 0x3a2b1c22u, 6u, 5u, 0x00000100u, 45u }, /* nginx */
  { 0x1c3a7b11u, 5u, 5u, 0x00000200u, 45u }, /* curlftp */
  { 0x2a2b3c44u, 4u, 5u, 0x00000400u, 45u }, /* gitui */
  { 0x3b1c2d33u, 7u, 6u, 0x00000800u, 40u }, /* firefox */
  { 0x2c1d3e44u, 6u, 6u, 0x00001000u, 40u }, /* chrome */
  { 0x1d2e3f55u, 4u, 6u, 0x00002000u, 40u }, /* wine */
  { 0x2e3f4a66u, 11u, 6u, 0x00004000u, 40u }, /* libreoffice */
  { 0x3f4a5b77u, 7u, 6u, 0x00008000u, 40u }, /* blender */
  { 0x4a5b6c88u, 6u, 6u, 0x00010000u, 40u }, /* ffmpeg */
  { 0x5b6c7d99u, 6u, 6u, 0x00020000u, 40u }, /* openssh */
  { 0x6c7d8eaau, 7u, 6u, 0x00040000u, 40u }, /* busybox */
  { 0x7d8e9fbbu, 7u, 6u, 0x00080000u, 40u }  /* openvpn */
};

static const raf_essential_table_t g_table = {
  g_essentials,
  RAF_ESSENTIAL_MAX,
  0x52414645u /* "RAFE" */
};

const raf_essential_table_t *RmR_essentials_table(void){
  return &g_table;
}

void RmR_essentials_store_init(raf_bitraf_store_t *st, u32 limit){
  if(!st) return;
  for(u32 i=0u;i<RAF_ESSENTIAL_MAX;i++){
    st->ids[i] = 0u;
    st->weights[i] = 0u;
  }
  st->head = 0u;
  st->count = 0u;
  st->limit = (limit == 0u || limit > RAF_ESSENTIAL_MAX) ? RAF_ESSENTIAL_MAX : limit;
}

void RmR_essentials_store_push(raf_bitraf_store_t *st, u32 id, u32 weight){
  if(!st) return;
  st->ids[st->head] = id;
  st->weights[st->head] = weight;
  st->head = (st->head + 1u) % st->limit;
  if(st->count < st->limit) st->count++;
}

void RmR_session_bar_init(raf_session_bar_t *bar){
  if(!bar) return;
  for(u32 i=0u;i<RAF_SESSION_BAR_MAX;i++) bar->weights[i] = 0u;
  bar->cursor = 0u;
  bar->count = 0u;
  bar->seal = 0x52414653u; /* "RAFS" */
}

void RmR_session_bar_balance(raf_session_bar_t *bar, const raf_bitraf_store_t *st){
  if(!bar || !st) return;
  u32 total = 0u;
  for(u32 i=0u;i<st->count;i++){
    total += st->weights[i];
  }
  if(total == 0u) return;
  for(u32 i=0u;i<st->count && i<RAF_SESSION_BAR_MAX;i++){
    bar->weights[i] = (st->weights[i] * 100u) / total;
  }
  bar->count = (st->count < RAF_SESSION_BAR_MAX) ? st->count : RAF_SESSION_BAR_MAX;
  bar->cursor = (bar->cursor + 1u) % bar->count;
}
