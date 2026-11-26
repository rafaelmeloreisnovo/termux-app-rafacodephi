/* SPDX-License-Identifier: GPL-3.0-only -- hosted test only */
#include "../bootstrap/rafbox_v1/rafbox_core.h"
#include <stdio.h>
#include <string.h>
static unsigned int checks;
#define REQUIRE(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL line %d\n",__LINE__);return 1;}} while(0)
static int expect_span(const char *path,int is_base,const char *expect) {
    rafbox_span s;
    int ok=is_base?rafbox_basename_span(path,&s):rafbox_dirname_span(path,&s);
    ++checks;
    if(!ok || strlen(expect)!=s.size || memcmp(expect,s.ptr,s.size)) {
        fprintf(stderr,"FAIL span for %s (%s)\n",path?path:"(null)",is_base?"base":"dir");
        return 0;
    }
    return 1;
}
int main(void){
    char huge[4098];unsigned int i;
    const char *bad[]={"sh","ash","cp","install","rm","sed","awk","find","busybox","/bin/sh","True","cat/../../sh",""};
    REQUIRE(rafbox_select("true")==RAFBOX_TRUE);
    REQUIRE(rafbox_select("false")==RAFBOX_FALSE);
    REQUIRE(rafbox_select("echo")==RAFBOX_ECHO);
    REQUIRE(rafbox_select("cat")==RAFBOX_CAT);
    REQUIRE(rafbox_select("basename")==RAFBOX_BASENAME);
    REQUIRE(rafbox_select("dirname")==RAFBOX_DIRNAME);
    REQUIRE(rafbox_select((const char *)0)==RAFBOX_UNSUPPORTED);
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i)REQUIRE(rafbox_select(bad[i])==RAFBOX_UNSUPPORTED);
    REQUIRE(expect_span("/tmp/a/b//",1,"b"));
    REQUIRE(expect_span("////",1,"/"));
    REQUIRE(expect_span("a",1,"a"));
    REQUIRE(expect_span("a/",1,"a"));
    REQUIRE(expect_span("/a//b/",0,"/a"));
    REQUIRE(expect_span("/a/b//",0,"/a"));
    REQUIRE(expect_span("/a",0,"/"));
    REQUIRE(expect_span("a",0,"."));
    REQUIRE(expect_span("////",0,"/"));
    REQUIRE(expect_span("./a",0,"."));
    REQUIRE(!rafbox_basename_span("",(rafbox_span *)&(rafbox_span){0}));
    REQUIRE(!rafbox_dirname_span((const char *)0,(rafbox_span *)&(rafbox_span){0}));
    for(i=0;i<4097;++i) {huge[i]='A';}
    huge[4097]='\0';
    REQUIRE(rafbox_strlen_bounded(huge)==RAFBOX_MAX_ARGUMENT_TEXT+1u);
    REQUIRE(!rafbox_basename_span(huge,(rafbox_span *)&(rafbox_span){0}));
    printf("RAFBOX_CORE_PASS checks=%u\n",checks);
    return 0;
}
