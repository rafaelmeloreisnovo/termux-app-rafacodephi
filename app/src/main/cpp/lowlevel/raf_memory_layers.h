#ifndef RAF_MEMORY_LAYERS_H
#define RAF_MEMORY_LAYERS_H
#include <stdint.h>

typedef struct {
    uint32_t l1_state_cap;
    uint32_t l2_in_buf;
    uint32_t l2_out_buf;
    uint32_t l2_jni_arena;
    uint32_t l2_bm_arena;
    uint32_t ram_note;
    uint32_t cache_line;
    uint32_t page_size;
} raf_memory_layers_t;

void raf_memory_layers_get(raf_memory_layers_t* out);

#endif
