#include "raf_bitraf.h"
#if !defined(RMR_NO_DEBUG_STRING)
#include <stdint.h>
#include <stddef.h>

static void append_int(char *out, size_t *off, size_t cap, int val) {
    if (*off >= cap) return;
    if (val < 0) {
        if (*off + 1 < cap) out[(*off)++] = '-';
        val = -val;
    }
    if (val == 0) {
        if (*off + 1 < cap) out[(*off)++] = '0';
    } else {
        int digits[10], d = 0;
        int tmp = val;
        while (tmp > 0) { digits[d++] = tmp % 10; tmp /= 10; }
        for (int i = d - 1; i >= 0 && *off < cap; i--) out[(*off)++] = (char)('0' + digits[i]);
    }
}

int bitraf_to_string(uint64_t i,char*out,int cap){
 if (cap <= 0) return 0;
 uint8_t o,d; uint16_t l,m,f; bitraf_decode(i,&o,&d,&l,&m,&f);
 size_t off = 0;
 const char *start = "{\"opcode\":";
 for (const char *p = start; *p && off < (size_t)cap; p++) out[off++] = *p;
 append_int(out, &off, (size_t)cap, o);
 const char *sep = ",\"dir\":";
 for (const char *p = sep; *p && off < (size_t)cap; p++) out[off++] = *p;
 append_int(out, &off, (size_t)cap, d);
 sep = ",\"layer\":";
 for (const char *p = sep; *p && off < (size_t)cap; p++) out[off++] = *p;
 append_int(out, &off, (size_t)cap, l);
 sep = ",\"imm\":";
 for (const char *p = sep; *p && off < (size_t)cap; p++) out[off++] = *p;
 append_int(out, &off, (size_t)cap, m);
 sep = ",\"flags\":";
 for (const char *p = sep; *p && off < (size_t)cap; p++) out[off++] = *p;
 append_int(out, &off, (size_t)cap, f);
 if (off + 1 < (size_t)cap) out[off++] = '}';
 if (off < (size_t)cap) out[off] = '\0';
 return (int)off;
}
#endif
