#include "raf_gp_dimension.h"
#include <math.h>
#include <stdio.h>

int raf_gp_dimension_json(const float* s,size_t n,char*out,int cap){
 if(!s||n<4||!out||cap<64) return -1;
 const float eps[3]={0.05f,0.1f,0.2f};
 float C[3]={0,0,0};
 double pairs=0;
 for(size_t i=0;i<n;i++)for(size_t j=i+1;j<n;j++){
  float d=fabsf(s[i]-s[j]); pairs+=1.0;
  for(int k=0;k<3;k++) if(d<eps[k]) C[k]+=1.0f;
 }
 for(int k=0;k<3;k++) C[k]=(pairs>0)?(C[k]/(float)pairs):0.0f;
 float l0=logf(fmaxf(C[0],1e-12f)), l2=logf(fmaxf(C[2],1e-12f));
 float e0=logf(eps[0]), e2=logf(eps[2]);
 float slope=(l2-l0)/(e2-e0);
 return snprintf(out,(size_t)cap,"{\"n\":%zu,\"C\":[%.6f,%.6f,%.6f],\"logC\":[%.6f,%.6f,%.6f],\"slope\":%.6f}",n,C[0],C[1],C[2],logf(fmaxf(C[0],1e-12f)),logf(fmaxf(C[1],1e-12f)),logf(fmaxf(C[2],1e-12f)),slope);
}
