#include "raf_memory_layers.h"
#include <unistd.h>

void raf_memory_layers_get(raf_memory_layers_t* out){
 if(!out) return;
 out->l1_state_cap=64;
 out->l2_in_buf=65536;
 out->l2_out_buf=65536;
 out->l2_jni_arena=256u*1024u;
 out->l2_bm_arena=512u*1024u;
 out->ram_note=1;
 out->cache_line=64;
 long p = sysconf(_SC_PAGESIZE);
 out->page_size = p>0?(uint32_t)p:4096u;
}
