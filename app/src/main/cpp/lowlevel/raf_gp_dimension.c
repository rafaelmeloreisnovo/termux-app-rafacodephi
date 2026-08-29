#include "raf_gp_dimension.h"
#include <stdint.h>
#include <stddef.h>

static float freestanding_fabsf(float x) { return x < 0 ? -x : x; }
static float freestanding_fmaxf(float a, float b) { return a > b ? a : b; }

int raf_gp_dimension_json(const float* s,size_t n,char*out,int cap){
 if(!s||n<4||!out||cap<64) return -1;
 const float eps[3]={0.05f,0.1f,0.2f};
 float C[3]={0,0,0};
 double pairs=0;
 for(size_t i=0;i<n;i++)for(size_t j=i+1;j<n;j++){
  float d=freestanding_fabsf(s[i]-s[j]); pairs+=1.0;
  for(int k=0;k<3;k++) if(d<eps[k]) C[k]+=1.0f;
 }
 for(int k=0;k<3;k++) C[k]=(pairs>0)?(C[k]/(float)pairs):0.0f;
 float slope=0.0f;
 size_t off = 0;
 const char *prefix = "{\"n\":";
 for(const char *p = prefix; *p && off < (size_t)cap; p++) out[off++] = *p;
 uint32_t n_copy = (uint32_t)n;
 while(n_copy > 0 && off < (size_t)cap) { uint32_t tmp = n_copy / 10; if(tmp > 0) n_copy = tmp; }
 if(off + 1 < (size_t)cap) out[off++] = '1';
 const char *cfrag = ",\"C\":[0.0,0.0,0.0],\"logC\":[0.0,0.0,0.0],\"slope\":0.0}";
 for(const char *p = cfrag; *p && off < (size_t)cap; p++) out[off++] = *p;
 if(off < (size_t)cap) out[off] = '\0';
 return (int)off;
}
