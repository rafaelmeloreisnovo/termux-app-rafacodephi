#include "raf_gp_dimension.h"
#include <stddef.h>
int main(void) {
 float s[4]={0,1,2,3}; char b[130];
 for(unsigned i=0;i<130;i++) b[i]='X';
 if(raf_gp_dimension_json(s,4,b+1,128)!=RAF_GP_UNIMPLEMENTED) return 1;
 if(b[1]!=0 || b[0]!='X' || b[129]!='X') return 2;
 if(raf_gp_dimension_json(s,(size_t)-1,b+1,128)!=RAF_GP_UNIMPLEMENTED) return 3;
 if(raf_gp_dimension_json(s,3,b+1,128)!=RAF_GP_EINVAL) return 4;
 if(raf_gp_dimension_json(NULL,4,b+1,128)!=RAF_GP_EINVAL) return 5;
 if(raf_gp_dimension_json(s,4,NULL,128)!=RAF_GP_EINVAL) return 6;
 if(raf_gp_dimension_json(s,4,b+1,0)!=RAF_GP_EINVAL) return 7;
 if(raf_gp_dimension_json(s,4,b+1,-1)!=RAF_GP_EINVAL) return 8;
 if(raf_gp_dimension_json(s,4,b+1,63)!=RAF_GP_EINVAL) return 9;
 return 0;
}
