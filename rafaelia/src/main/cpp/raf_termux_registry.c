/* raf_termux_registry.c
   Registro deterministico de ferramentas principais (sem libc, sem dependencias)
*/

#include "raf_termux_registry.h"

static const raf_tool_entry_t g_entries[] = {
  { 0x2c96b2a1u, 3u, 1u, 0x0100u }, /* sh  */
  { 0x3d8f2aa9u, 2u, 1u, 0x0100u }, /* ls  */
  { 0x1f1f7c1bu, 2u, 1u, 0x0100u }, /* cp  */
  { 0x1f20f912u, 2u, 1u, 0x0100u }, /* mv  */
  { 0x1f1f7c32u, 2u, 1u, 0x0100u }, /* rm  */
  { 0x2a1e4d77u, 4u, 2u, 0x0100u }, /* grep */
  { 0x3b7fbb4cu, 4u, 2u, 0x0100u }, /* awk  */
  { 0x2fd3e77bu, 4u, 2u, 0x0100u }, /* sed  */
  { 0x3b9c9ee6u, 4u, 3u, 0x0100u }, /* curl */
  { 0x1f3a5a2eu, 3u, 3u, 0x0100u }, /* git  */
  { 0x3f51f1a1u, 3u, 3u, 0x0100u }, /* ssh  */
  { 0x2a1f5b19u, 3u, 3u, 0x0100u }, /* tar  */
  { 0x2e6a1c19u, 4u, 4u, 0x0100u }, /* make */
  { 0x3f3b8e12u, 5u, 4u, 0x0100u }, /* clang */
  { 0x1f316e2du, 3u, 4u, 0x0100u }, /* gcc */
  { 0x2b71c6c7u, 4u, 4u, 0x0100u }, /* binutils */
  { 0x34bc7a11u, 7u, 5u, 0x0100u }  /* openssl */
};

static const raf_tool_registry_t g_registry = {
  g_entries,
  (u32)(sizeof(g_entries) / sizeof(g_entries[0])),
  0x52414652u /* "RAFR" */
};

const raf_tool_registry_t *RmR_termux_registry(void){
  return &g_registry;
}
