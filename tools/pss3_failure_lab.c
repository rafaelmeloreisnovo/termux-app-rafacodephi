#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_LINE 4096
#define MAX_CELLS 128
#define MAX_EVENTS 32768
#define MAX_STR 128

typedef struct {
    char domain[MAX_STR], signature_hash[MAX_STR], event_type[MAX_STR];
    char severity[MAX_STR], recurring[MAX_STR], stable_after_fix[MAX_STR], rollback[MAX_STR], zombie_detected[MAX_STR];
    char exit_code[MAX_STR], signal[MAX_STR], errno_s[MAX_STR];
} failure_event;

typedef struct {
    char signature_hash[MAX_STR], domain[MAX_STR], event_type[MAX_STR];
    int count, recurring_count, stable_after_fix_count, attempts_after_fix, rollback_count, zombie_count;
    double severity_sum, recurrence_rate, fix_stability, beta_risk;
    int gate_class;
} failure_bucket;

static const int fibo_01123[] = {0, 1, 1, 2, 3};
static const int fibo_001123[] = {0, 0, 1, 1, 2, 3};

static int to_i(const char *s){ return (s&&*s)?atoi(s):0; }
static double to_d(const char *s){ return (s&&*s)?atof(s):0.0; }
static int eq(const char *a,const char *b){ return a&&b&&!strcasecmp(a,b); }
static void cpy(char *d,const char *s){ snprintf(d,MAX_STR,"%s",s?s:""); }
static const char *at(char **c,int n,int i){ return (i>=0&&i<n&&c[i])?c[i]:""; }

static int csv_split_simple(char *line,char **cells,int max){
    int n=0; char *tok=strtok(line,",\n\r");
    while(tok&&n<max){ cells[n++]=tok; tok=strtok(NULL,",\n\r"); }
    return n;
}

static int pss3_failure_gate(int tick, int severity, int recurring, int rollback, int zombie){
    if (zombie) return 3;
    int seq=fibo_01123[tick%5]+fibo_001123[tick%6];
    int score=severity+recurring+rollback+(seq>=3);
    if(score<=1) return 0;
    if(score<=3) return 1;
    if(score<=5) return 2;
    return 3;
}

static const char *status(const failure_bucket *b){
    if (b->fix_stability>=0.90 && b->recurrence_rate<0.05) return "FIXED_LIKELY";
    if (b->beta_risk>=0.75 || b->gate_class==3) return "BLOCKER_BETA";
    if (b->recurrence_rate>=0.20 && b->beta_risk>=0.40) return "CRITICAL_RECURRENT";
    if (b->recurrence_rate>=0.10) return "RECURRENT";
    if (b->beta_risk<=0.05 && b->count==1) return "NOISE_LIKELY";
    if (b->beta_risk>=0.12) return "WATCH";
    return "STABLE_OK";
}

static int load_csv(const char *path,failure_event *ev,int *count){
    FILE *fp=fopen(path,"r"); if(!fp) return 0;
    char line[MAX_LINE],*cells[MAX_CELLS];
    int idom=-1,isig=-1,ievt=-1,isev=-1,irec=-1,istf=-1,irol=-1,izom=-1,iex=-1,isg=-1,ier=-1;
    if(!fgets(line,sizeof(line),fp)){ fclose(fp); return 0; }
    int hc=csv_split_simple(line,cells,MAX_CELLS);
    for(int i=0;i<hc;i++){ if(eq(cells[i],"domain"))idom=i; else if(eq(cells[i],"signature_hash"))isig=i; else if(eq(cells[i],"event_type"))ievt=i; else if(eq(cells[i],"severity"))isev=i; else if(eq(cells[i],"recurring"))irec=i; else if(eq(cells[i],"stable_after_fix"))istf=i; else if(eq(cells[i],"rollback"))irol=i; else if(eq(cells[i],"zombie_detected"))izom=i; else if(eq(cells[i],"exit_code"))iex=i; else if(eq(cells[i],"signal"))isg=i; else if(eq(cells[i],"errno"))ier=i; }
    if(isig<0){ fclose(fp); return 0; }
    int n=0;
    while(n<MAX_EVENTS && fgets(line,sizeof(line),fp)){
        int cc=csv_split_simple(line,cells,MAX_CELLS); if(cc<=0) continue;
        cpy(ev[n].domain,at(cells,cc,idom)); cpy(ev[n].signature_hash,at(cells,cc,isig)); cpy(ev[n].event_type,at(cells,cc,ievt)); cpy(ev[n].severity,at(cells,cc,isev)); cpy(ev[n].recurring,at(cells,cc,irec)); cpy(ev[n].stable_after_fix,at(cells,cc,istf)); cpy(ev[n].rollback,at(cells,cc,irol)); cpy(ev[n].zombie_detected,at(cells,cc,izom)); cpy(ev[n].exit_code,at(cells,cc,iex)); cpy(ev[n].signal,at(cells,cc,isg)); cpy(ev[n].errno_s,at(cells,cc,ier));
        n++;
    }
    fclose(fp); *count=n; return 1;
}

int main(int argc,char **argv){
    const char *path=(argc>1)?argv[1]:"out/failure_trace.csv";
    failure_event *ev=calloc(MAX_EVENTS,sizeof(*ev)); failure_bucket *b=NULL; if(!ev) return 1;
    int n=0,bn=0,bc=0; if(!load_csv(path,ev,&n)){ fprintf(stderr,"failed to load %s\n",path); free(ev); return 1; }

    for(int i=0;i<n;i++){
        int bi=-1; for(int j=0;j<bn;j++) if(!strcmp(b[j].signature_hash,ev[i].signature_hash)){ bi=j; break; }
        if(bi<0){ if(bn>=bc){ int nx=bc?bc*2:64; failure_bucket *tmp=realloc(b,(size_t)nx*sizeof(*b)); if(!tmp){ free(ev); free(b); return 1; } b=tmp; memset(b+bc,0,(size_t)(nx-bc)*sizeof(*b)); bc=nx; } bi=bn++; cpy(b[bi].signature_hash,ev[i].signature_hash); }
        if(!b[bi].domain[0]) cpy(b[bi].domain,ev[i].domain);
        if(!b[bi].event_type[0]) cpy(b[bi].event_type,ev[i].event_type);
        b[bi].count++; b[bi].severity_sum += to_d(ev[i].severity); b[bi].recurring_count += to_i(ev[i].recurring)>0; b[bi].stable_after_fix_count += to_i(ev[i].stable_after_fix)>0; b[bi].attempts_after_fix += ev[i].stable_after_fix[0]?1:0; b[bi].rollback_count += to_i(ev[i].rollback)>0; b[bi].zombie_count += to_i(ev[i].zombie_detected)>0;
    }
    for(int i=0;i<bn;i++){ double savg=b[i].count?b[i].severity_sum/b[i].count:0.0; b[i].recurrence_rate=n?(double)b[i].count/n:0.0; b[i].fix_stability=b[i].attempts_after_fix?(double)b[i].stable_after_fix_count/b[i].attempts_after_fix:0.0; b[i].beta_risk=savg*b[i].recurrence_rate*(1.0-b[i].fix_stability); b[i].gate_class=pss3_failure_gate(i,(int)(savg+0.5),b[i].recurring_count>0,b[i].rollback_count>0,b[i].zombie_count>0); }

    puts("1. failure stats\n2. top recurring failures\n3. beta risk ranking\n4. fix stability\n5. gate/fibo view\n6. arena failure map\np. paths\nc. color on/off\ne. emoji on/off\n0. exit");
    puts("note: simple CSV parser does not support quoted commas.");

    for(int ch;(ch=getchar())!=EOF;){
        if(ch=='0') break;
        if(ch=='1'){
            int recurring=0,zombie=0,rollback=0,crash=0,build=0,terminal=0,low=0; int fails_rec=0,att_rec=0,fails_non=0,att_non=0;
            for(int i=0;i<n;i++){ recurring+=to_i(ev[i].recurring)>0; zombie+=to_i(ev[i].zombie_detected)>0; rollback+=to_i(ev[i].rollback)>0; crash+=eq(ev[i].event_type,"crash"); build+=eq(ev[i].domain,"build"); terminal+=eq(ev[i].domain,"terminal"); low+=(eq(ev[i].domain,"lowlevel")||eq(ev[i].domain,"rmr")); int fail=(to_i(ev[i].exit_code)!=0||to_i(ev[i].signal)!=0||to_i(ev[i].errno_s)!=0||eq(ev[i].event_type,"fail")||eq(ev[i].event_type,"failure")||eq(ev[i].event_type,"error")||eq(ev[i].event_type,"crash")); if(to_i(ev[i].recurring)>0){att_rec++;fails_rec+=fail;} else {att_non++;fails_non+=fail;} }
            double p_rec=att_rec?(double)fails_rec/att_rec:0.0, p_non=att_non?(double)fails_non/att_non:0.0;
            printf("total_events=%d total_failures=%d unique_signature_hashes=%d recurring_failures=%d zombie_events=%d rollback_events=%d crash_events=%d build_failures=%d terminal_failures=%d lowlevel_failures=%d\n",n,n,bn,recurring,zombie,rollback,crash,build,terminal,low);
            printf("delta_failure=%.6f p_fail_rec=%.6f p_fail_nonrec=%.6f\n",p_rec-p_non,p_rec,p_non);
        } else if(ch=='2' || ch=='3' || ch=='4'){
            for(int i=0;i<bn;i++) printf("ROW|signature_hash=%s|domain=%s|event_type=%s|count=%d|recurrence=%.6f|severity=%.3f|fix_stability=%.6f|beta_risk=%.6f|status=%s\n",b[i].signature_hash,b[i].domain,b[i].event_type,b[i].count,b[i].recurrence_rate,b[i].count?b[i].severity_sum/b[i].count:0.0,b[i].fix_stability,b[i].beta_risk,status(&b[i]));
        } else if(ch=='5'){
            for(int i=0;i<bn;i++) printf("GATE|signature_hash=%s|fwd=%d|rev=%d|inv=%d|invR=%d|gate=%d\n",b[i].signature_hash,fibo_01123[i%5],fibo_01123[(4-(i%5))],fibo_001123[i%6],fibo_001123[(5-(i%6))],b[i].gate_class);
        } else if(ch=='6'){
            int grid[4][4]={{0}}; for(int i=0;i<bn;i++){ int g=b[i].gate_class; int s=(b[i].beta_risk>=0.75)?3:(b[i].beta_risk>=0.40)?2:(b[i].beta_risk>=0.12)?1:0; if(g<0)g=0;if(g>3)g=3; grid[g][s]++; }
            for(int g=0;g<4;g++) for(int s=0;s<4;s++) printf("ARENA|gate=%d|risk_band=%d|count=%d\n",g,s,grid[g][s]);
        } else if(ch=='p') printf("csv_path=%s\n",path);
    }
    free(ev); free(b); return 0;
}
