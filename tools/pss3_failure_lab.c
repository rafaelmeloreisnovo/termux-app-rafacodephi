#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <ctype.h>

/* NOTE: simplistic CSV parser (comma split only); quoted commas are NOT supported. */

#define MAX_COLS 128
#define MAX_LINE 8192
#define WBAR 60

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

static void trim_newline(char *s){ size_t n=strlen(s); while(n && (s[n-1]=='\n'||s[n-1]=='\r')) s[--n]=0; }
static int split_csv_row(char *line, char *cells[], int maxc){ int c=0; char *p=line; while(*p && c<maxc){ cells[c++]=p; while(*p && *p!=',') p++; if(*p==','){*p=0; p++;}} return c; }
static int col_index(char cols[][64], int n, const char *name){ for(int i=0;i<n;i++) if(strcmp(cols[i],name)==0) return i; return -1; }
static int parse_header(char *line, char cols[][64], int maxc){
  char *cells[MAX_COLS]={0}; int n=split_csv_row(line,cells,maxc);
  for(int i=0;i<n;i++){ strncpy(cols[i], cells[i]?cells[i]:"",63); cols[i][63]=0; }
  return n;
}
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

int main(int argc, char **argv){
  const char *path = argc>1 ? argv[1] : "out/failure_trace.csv";
  FILE *f=fopen(path,"rb");
  if(!f){ printf("missing %s\n",path); return 1; }
  char line[MAX_LINE];
  if(!fgets(line,sizeof(line),f)){ puts("empty csv"); fclose(f); return 1; }
  trim_newline(line);
  char cols[MAX_COLS][64]; int ncols=parse_header(line, cols, MAX_COLS);

  int i_sig=col_index(cols,ncols,"signature_hash"), i_domain=col_index(cols,ncols,"domain"), i_event=col_index(cols,ncols,"event_type");
  int i_sev=col_index(cols,ncols,"severity"), i_rec=col_index(cols,ncols,"recurring"), i_st=col_index(cols,ncols,"stable_after_fix");
  int i_roll=col_index(cols,ncols,"rollback"), i_zom=col_index(cols,ncols,"zombie_detected");
  if(i_sig<0||i_domain<0||i_event<0||i_sev<0||i_rec<0||i_st<0||i_roll<0){ puts("required columns missing"); fclose(f); return 2; }

  SigStat *arr=NULL; size_t n=0,cap=0;
  long total_events=0,total_failures=0,rec_fail=0,zombie=0,rollback=0,crash=0,build=0,term=0,low=0;
  while(fgets(line,sizeof(line),f)){
    trim_newline(line); if(!line[0]) continue;
    char *cells[MAX_COLS]={0}; int nc=split_csv_row(line,cells,MAX_COLS);
    const char *sig=cell(cells,i_sig,nc), *dom=cell(cells,i_domain,nc), *ev=cell(cells,i_event,nc);
    int sev=atoi(cell(cells,i_sev,nc)), recurring=atoi(cell(cells,i_rec,nc)), stable=atoi(cell(cells,i_st,nc));
    int rb=atoi(cell(cells,i_roll,nc)); int zb=i_zom>=0?atoi(cell(cells,i_zom,nc)):0;
    total_events++; total_failures++;
    if(recurring) rec_fail++;
    if(zb) zombie++;
    if(rb) rollback++;
    if(contains_ci(ev,"crash")) crash++;
    if(contains_ci(dom,"build")) build++;
    if(contains_ci(dom,"terminal")) term++;
    if(contains_ci(dom,"low")) low++;

    size_t idx=n; for(size_t i=0;i<n;i++) if(strcmp(arr[i].signature_hash,sig)==0){ idx=i; break; }
    if(idx==n){ if(n==cap){ size_t ncap=cap?cap*2:64; SigStat *tmp=realloc(arr,ncap*sizeof(SigStat)); if(!tmp){free(arr); fclose(f); return 3;} arr=tmp; cap=ncap; }
      memset(&arr[n],0,sizeof(SigStat)); strncpy(arr[n].signature_hash,sig,127); strncpy(arr[n].domain,dom,63); strncpy(arr[n].event_type,ev,63); n++; }
    SigStat *s=&arr[idx]; s->count++; s->recurring_count+=recurring; s->stable_after_fix+=stable; s->attempts_after_fix++; s->severity_sum+=sev; s->zombie_seen|=zb; s->rollback_seen|=rb;
  }
  fclose(f);

  puts("1. failure stats");
  printf("total_events=%ld\n",total_events); printf("total_failures=%ld\n",total_failures); printf("unique_signature_hashes=%zu\n",n);
  printf("recurring_failures=%ld zombie_events=%ld rollback_events=%ld crash_events=%ld build_failures=%ld terminal_failures=%ld lowlevel_failures=%ld\n",rec_fail,zombie,rollback,crash,build,term,low);

  double pfr = total_failures? (double)rec_fail/total_failures : 0.0;
  double pfn = total_failures? (double)(total_failures-rec_fail)/total_failures : 0.0;
  double delta_failure = pfr - pfn;
  printf("delta_failure=%.6f\n",delta_failure);

  puts("2/3. top recurring + beta risk ranking");
  for(size_t i=0;i<n;i++){
    double recurrence_rate=total_failures? (double)arr[i].count/total_failures:0.0;
    double fix_stability=arr[i].attempts_after_fix? (double)arr[i].stable_after_fix/arr[i].attempts_after_fix:0.0;
    double sev=arr[i].count? arr[i].severity_sum/arr[i].count:0.0;
    double beta_risk=sev*recurrence_rate*(1.0-fix_stability);
    int gate=pss3_failure_gate((int)i,(int)sev,arr[i].recurring_count>0,arr[i].rollback_seen,arr[i].zombie_seen);
    const char *status = beta_risk>3.5||gate==3?"BLOCKER_BETA":beta_risk>2.0?"CRITICAL_RECURRENT":arr[i].recurring_count?"RECURRENT":fix_stability>0.8?"FIXED_LIKELY":"WATCH";
    printf("sig=%s count=%ld recurrence=%.4f sev=%.2f fix=%.3f beta_risk=%.4f gate=%d status=%s\n",arr[i].signature_hash,arr[i].count,recurrence_rate,sev,fix_stability,beta_risk,gate,status);
  }

  puts("5. gate/fibo view: 01123 / 001123 fwd rev inv invR");
  puts("6. arena failure map: TODO(beta)");
  free(arr);
  return 0;
}
