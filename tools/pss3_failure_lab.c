#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <ctype.h>

/* NOTE: simplistic CSV parser (comma split only); quoted commas are NOT supported. */

#define MAX_COLS 128
#define MAX_LINE 8192

typedef struct {
  char signature_hash[128];
  char domain[64];
  char event_type[64];
  long count;
  long recurring_count;
  long stable_after_fix;
  long attempts_after_fix;
  double severity_sum;
  int zombie_seen;
  int rollback_seen;
} SigStat;

typedef struct {
  SigStat *arr;
  size_t n;
  long total_events,total_failures,rec_fail,zombie,rollback,crash,build,term,low;
  double delta_failure;
} Stats;

static void trim_newline(char *s){ size_t n=strlen(s); while(n && (s[n-1]=='\n'||s[n-1]=='\r')) s[--n]=0; }
static int split_csv_row(char *line, char *cells[], int maxc){ int c=0; char *p=line; while(*p && c<maxc){ cells[c++]=p; while(*p && *p!=',') p++; if(*p==','){*p=0; p++;}} return c; }
static int col_index(char cols[][64], int n, const char *name){ for(int i=0;i<n;i++) if(strcmp(cols[i],name)==0) return i; return -1; }
static int parse_header(char *line, char cols[][64], int maxc){ char *cells[MAX_COLS]={0}; int n=split_csv_row(line,cells,maxc); for(int i=0;i<n;i++){ strncpy(cols[i], cells[i]?cells[i]:"",63); cols[i][63]=0; } return n; }
static const char* cell(char *cells[], int idx, int nc){ return (idx>=0 && idx<nc && cells[idx]) ? cells[idx] : ""; }
static int contains_ci(const char *h,const char *n){ if(!*n) return 1; size_t nl=strlen(n); for(;*h;h++){ size_t i=0; while(i<nl && h[i] && tolower((unsigned char)h[i])==tolower((unsigned char)n[i])) i++; if(i==nl) return 1;} return 0; }

static int pss3_failure_gate(int tick, int severity, int recurring, int rollback, int zombie){
  int s1[]={0,1,1,2,3}, s2[]={0,0,1,1,2,3};
  int base=(s1[tick%5] + 2*s2[tick%6] + (tick&3)) & 3;
  if(zombie) return 3;
  if(rollback && base<2) base=2;
  if(recurring && base<2) base=2;
  if(severity>=8 && base<3) base=3;
  return base;
}

static const char *status_of(double beta_risk,int gate,long recurring_count,double fix_stability){
  if(beta_risk>3.5 || gate==3) return "BLOCKER_BETA";
  if(beta_risk>2.0) return "CRITICAL_RECURRENT";
  if(recurring_count>0) return "RECURRENT";
  if(fix_stability>0.8) return "FIXED_LIKELY";
  if(beta_risk<0.05) return "NOISE_LIKELY";
  if(beta_risk<0.15) return "STABLE_OK";
  return "WATCH";
}

static int load_stats(const char *path, Stats *st){
  memset(st,0,sizeof(*st));
  FILE *f=fopen(path,"rb"); if(!f) return 1;
  char line[MAX_LINE]; if(!fgets(line,sizeof(line),f)){ fclose(f); return 2; }
  trim_newline(line);
  char cols[MAX_COLS][64]; int ncols=parse_header(line, cols, MAX_COLS);
  int i_sig=col_index(cols,ncols,"signature_hash"), i_domain=col_index(cols,ncols,"domain"), i_event=col_index(cols,ncols,"event_type");
  int i_sev=col_index(cols,ncols,"severity"), i_rec=col_index(cols,ncols,"recurring"), i_st=col_index(cols,ncols,"stable_after_fix");
  int i_roll=col_index(cols,ncols,"rollback"), i_zom=col_index(cols,ncols,"zombie_detected");
  if(i_sig<0||i_domain<0||i_event<0||i_sev<0||i_rec<0||i_st<0||i_roll<0){ fclose(f); return 3; }

  size_t cap=0;
  while(fgets(line,sizeof(line),f)){
    trim_newline(line); if(!line[0]) continue;
    char *cells[MAX_COLS]={0}; int nc=split_csv_row(line,cells,MAX_COLS);
    const char *sig=cell(cells,i_sig,nc), *dom=cell(cells,i_domain,nc), *ev=cell(cells,i_event,nc);
    int sev=atoi(cell(cells,i_sev,nc)), recurring=atoi(cell(cells,i_rec,nc)), stable=atoi(cell(cells,i_st,nc));
    int rb=atoi(cell(cells,i_roll,nc)); int zb=i_zom>=0?atoi(cell(cells,i_zom,nc)):0;
    st->total_events++; st->total_failures++;
    if(recurring) st->rec_fail++;
    if(zb) st->zombie++;
    if(rb) st->rollback++;
    if(contains_ci(ev,"crash")) st->crash++;
    if(contains_ci(dom,"build")) st->build++;
    if(contains_ci(dom,"terminal")) st->term++;
    if(contains_ci(dom,"low")) st->low++;

    size_t idx=st->n; for(size_t i=0;i<st->n;i++) if(strcmp(st->arr[i].signature_hash,sig)==0){ idx=i; break; }
    if(idx==st->n){
      if(st->n==cap){ size_t ncap=cap?cap*2:64; SigStat *tmp=realloc(st->arr,ncap*sizeof(SigStat)); if(!tmp){ free(st->arr); fclose(f); return 4; } st->arr=tmp; cap=ncap; }
      memset(&st->arr[st->n],0,sizeof(SigStat));
      strncpy(st->arr[st->n].signature_hash,sig,127); strncpy(st->arr[st->n].domain,dom,63); strncpy(st->arr[st->n].event_type,ev,63);
      st->n++;
    }
    SigStat *s=&st->arr[idx]; s->count++; s->recurring_count+=recurring; s->stable_after_fix+=stable; s->attempts_after_fix++; s->severity_sum+=sev; s->zombie_seen|=zb; s->rollback_seen|=rb;
  }
  fclose(f);
  double pfr = st->total_failures? (double)st->rec_fail/st->total_failures : 0.0;
  double pfn = st->total_failures? (double)(st->total_failures-st->rec_fail)/st->total_failures : 0.0;
  st->delta_failure = pfr - pfn;
  return 0;
}

static void show_failure_stats(const Stats *st){
  printf("total_events=%ld\n",st->total_events);
  printf("total_failures=%ld\n",st->total_failures);
  printf("unique_signature_hashes=%zu\n",st->n);
  printf("recurring_failures=%ld zombie_events=%ld rollback_events=%ld crash_events=%ld build_failures=%ld terminal_failures=%ld lowlevel_failures=%ld\n",st->rec_fail,st->zombie,st->rollback,st->crash,st->build,st->term,st->low);
  printf("delta_failure=%.6f\n",st->delta_failure);
}

static void show_rank(const Stats *st, int top_only){
  for(size_t i=0;i<st->n;i++){
    const SigStat *s=&st->arr[i];
    double recurrence_rate=st->total_failures? (double)s->count/st->total_failures:0.0;
    double fix_stability=s->attempts_after_fix? (double)s->stable_after_fix/s->attempts_after_fix:0.0;
    double sev=s->count? s->severity_sum/s->count:0.0;
    double beta_risk=sev*recurrence_rate*(1.0-fix_stability);
    int gate=pss3_failure_gate((int)i,(int)sev,s->recurring_count>0,s->rollback_seen,s->zombie_seen);
    if(top_only && s->count<2) continue;
    printf("sig=%s count=%ld recurrence=%.4f sev=%.2f fix=%.3f beta_risk=%.4f gate=%d status=%s\n",s->signature_hash,s->count,recurrence_rate,sev,fix_stability,beta_risk,gate,status_of(beta_risk,gate,s->recurring_count,fix_stability));
  }
}

static void show_fix_stability(const Stats *st){
  for(size_t i=0;i<st->n;i++){
    const SigStat *s=&st->arr[i];
    double fix_stability=s->attempts_after_fix? (double)s->stable_after_fix/s->attempts_after_fix:0.0;
    printf("sig=%s attempts=%ld stable_after_fix=%ld fix_stability=%.4f\n",s->signature_hash,s->attempts_after_fix,s->stable_after_fix,fix_stability);
  }
}

static void show_gate_fibo(void){
  puts("01123 fwd=01123 rev=32110 inv=10023 invR=32001");
  puts("001123 fwd=001123 rev=321100 inv=110023 invR=320011");
  puts("J=0 noise J=1 light J=2 recurring J=3 blocker");
}

int main(int argc, char **argv){
  const char *path = argc>1 ? argv[1] : "out/failure_trace.csv";
  Stats st; int rc=load_stats(path,&st);
  if(rc){ printf("load failure_trace failed rc=%d path=%s\n",rc,path); return rc; }

  while(1){
    puts("\nPSS3 menu: 1 stats 2 top recurring 3 beta risk 4 fix stability 5 gate/fibo 6 arena failure map p paths c color e emoji 0 exit");
    printf("select> ");
    char in[16]={0}; if(!fgets(in,sizeof(in),stdin)) break; trim_newline(in);
    if(!strcmp(in,"0")) break;
    else if(!strcmp(in,"1")) show_failure_stats(&st);
    else if(!strcmp(in,"2")) show_rank(&st,1);
    else if(!strcmp(in,"3")) show_rank(&st,0);
    else if(!strcmp(in,"4")) show_fix_stability(&st);
    else if(!strcmp(in,"5")) show_gate_fibo();
    else if(!strcmp(in,"6")) puts("arena failure map: not enabled in headless mode");
    else if(!strcasecmp(in,"p")) printf("trace=%s\n",path);
    else if(!strcasecmp(in,"c")) puts("color toggle reserved");
    else if(!strcasecmp(in,"e")) puts("emoji toggle reserved");
    else puts("?");
  }
  free(st.arr);
  return 0;
}
