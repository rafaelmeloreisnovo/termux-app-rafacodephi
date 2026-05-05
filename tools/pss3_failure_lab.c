#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_LINE 4096
#define MAX_CELLS 128
#define Q16_ONE 65536u

typedef struct {
    char *timestamp, *run_id, *domain, *layer, *file, *function_name, *line;
    char *signature_hash, *event_type, *severity, *recurring, *stable_after_fix;
    char *rollback, *exit_code, *signal, *errno_s, *latency_ms, *delta_ms;
    char *gate, *notes, *zombie_detected;
} failure_event;

typedef struct {
    char *signature_hash;
    int count;
    double severity_sum;
    int recurring_count;
    int stable_after_fix_count;
    int attempts_after_fix;
    int rollback_count;
    int zombie_count;
    char *domain;
    char *event_type;
    double recurrence_rate;
    double fix_stability;
    double beta_risk;
    int status_gate;
} failure_bucket;

static int g_color = 1, g_emoji = 1;
static const int fibo_01123[] = {0,1,1,2,3};
static const int fibo_001123[] = {0,0,1,1,2,3};

static char *dup_s(const char *s){ size_t n=strlen(s)+1; char *p=(char*)malloc(n); if(p) memcpy(p,s,n); return p; }
static int to_i(const char *s){ return s?atoi(s):0; }
static double to_d(const char *s){ return s?atof(s):0.0; }

static const char *cell_at(char **cells, int n, int idx){ return (idx>=0 && idx<n && cells[idx])?cells[idx]:""; }

static int csv_split_simple(char *line, char **cells, int max_cells) {
    int c = 0;
    char *tok = strtok(line, ",\n\r");
    while (tok && c < max_cells) { cells[c++] = tok; tok = strtok(NULL, ",\n\r"); }
    return c;
}

static int append_event(failure_event **ev, int *n, int *cap, failure_event e){
    if(*n >= *cap){
        int new_cap = (*cap==0)?128:(*cap*2);
        failure_event *tmp = (failure_event*)realloc(*ev, (size_t)new_cap * sizeof(failure_event));
        if(!tmp) return 0;
        *ev = tmp; *cap = new_cap;
    }
    (*ev)[(*n)++] = e;
    return 1;
}

static int load_csv(const char *path, failure_event **events, int *count){
    FILE *fp = fopen(path, "r"); if(!fp) return 0;
    char line[MAX_LINE]; char *cells[MAX_CELLS];
    int idx_sig=-1, idx_sev=-1, idx_rec=-1, idx_stf=-1, idx_roll=-1, idx_evt=-1, idx_dom=-1, idx_gate=-1, idx_zom=-1;
    if(!fgets(line,sizeof(line),fp)){ fclose(fp); return 0; }
    int hc = csv_split_simple(line,cells,MAX_CELLS);
    for(int i=0;i<hc;i++){
        if(!strcasecmp(cells[i],"signature_hash")) idx_sig=i;
        else if(!strcasecmp(cells[i],"severity")) idx_sev=i;
        else if(!strcasecmp(cells[i],"recurring")) idx_rec=i;
        else if(!strcasecmp(cells[i],"stable_after_fix")) idx_stf=i;
        else if(!strcasecmp(cells[i],"rollback")) idx_roll=i;
        else if(!strcasecmp(cells[i],"event_type")) idx_evt=i;
        else if(!strcasecmp(cells[i],"domain")) idx_dom=i;
        else if(!strcasecmp(cells[i],"gate")) idx_gate=i;
        else if(!strcasecmp(cells[i],"zombie_detected")) idx_zom=i;
    }
    if(idx_sig<0){ fclose(fp); return 0; }

    int n=0, cap=0; failure_event *arr=NULL;
    while(fgets(line,sizeof(line),fp)){
        int cc = csv_split_simple(line,cells,MAX_CELLS); if(cc<=0) continue;
        failure_event e = {0};
        e.signature_hash=dup_s(cell_at(cells,cc,idx_sig));
        e.severity=dup_s(cell_at(cells,cc,idx_sev));
        e.recurring=dup_s(cell_at(cells,cc,idx_rec));
        e.stable_after_fix=dup_s(cell_at(cells,cc,idx_stf));
        e.rollback=dup_s(cell_at(cells,cc,idx_roll));
        e.event_type=dup_s(cell_at(cells,cc,idx_evt));
        e.domain=dup_s(cell_at(cells,cc,idx_dom));
        e.gate=dup_s(cell_at(cells,cc,idx_gate));
        e.zombie_detected=dup_s(cell_at(cells,cc,idx_zom));
        if(!append_event(&arr,&n,&cap,e)){ fclose(fp); return 0; }
    }
    fclose(fp); *events=arr; *count=n; return 1;
}

static int pss3_failure_gate(int tick, int severity, int recurring, int rollback, int zombie){
    if(zombie || rollback) return 3;
    int seqA = fibo_01123[tick % 5], seqB = fibo_001123[tick % 6];
    int score = severity + recurring + ((seqA + seqB) >= 3 ? 1 : 0);
    if(score <= 1) return 0;
    if(score <= 3) return 1;
    if(score <= 5) return 2;
    return 3;
}

static int find_or_create_bucket(failure_bucket **buckets, int *bucket_count, int *bucket_cap, const char *signature_hash){
    for(int i=0;i<*bucket_count;i++){
        if(!strcmp((*buckets)[i].signature_hash, signature_hash)) return i;
    }
    if(*bucket_count >= *bucket_cap){
        int new_cap = (*bucket_cap==0)?64:(*bucket_cap*2);
        failure_bucket *tmp = (failure_bucket*)realloc(*buckets, (size_t)new_cap * sizeof(failure_bucket));
        if(!tmp) return -1;
        *buckets = tmp;
        *bucket_cap = new_cap;
    }
    failure_bucket *b = &(*buckets)[*bucket_count];
    memset(b, 0, sizeof(*b));
    b->signature_hash = dup_s(signature_hash ? signature_hash : "");
    (*bucket_count)++;
    return (*bucket_count)-1;
}

static int build_buckets(failure_event *ev, int n, failure_bucket **out, int *out_count){
    failure_bucket *buckets = NULL;
    int bucket_count = 0, bucket_cap = 0;
    for(int i=0;i<n;i++){
        const char *sig = ev[i].signature_hash ? ev[i].signature_hash : "";
        int bi = find_or_create_bucket(&buckets, &bucket_count, &bucket_cap, sig);
        if(bi < 0) return 0;
        failure_bucket *b = &buckets[bi];
        int recurring = to_i(ev[i].recurring) > 0;
        int stable = to_i(ev[i].stable_after_fix) > 0;
        int rollback = to_i(ev[i].rollback) > 0;
        int zombie = to_i(ev[i].zombie_detected) > 0;
        b->count++;
        b->severity_sum += to_d(ev[i].severity);
        b->recurring_count += recurring;
        b->stable_after_fix_count += stable;
        b->attempts_after_fix += (ev[i].stable_after_fix && ev[i].stable_after_fix[0]) ? 1 : 0;
        b->rollback_count += rollback;
        b->zombie_count += zombie;
        if(!b->domain || !b->domain[0]) b->domain = ev[i].domain;
        if(!b->event_type || !b->event_type[0]) b->event_type = ev[i].event_type;
    }

    for(int i=0;i<bucket_count;i++){
        failure_bucket *b = &buckets[i];
        double severity_avg = (b->count > 0) ? (b->severity_sum / (double)b->count) : 0.0;
        b->recurrence_rate = (n > 0) ? ((double)b->count / (double)n) : 0.0;
        b->fix_stability = (b->attempts_after_fix > 0)
            ? ((double)b->stable_after_fix_count / (double)b->attempts_after_fix)
            : 0.0;
        b->beta_risk = severity_avg * b->recurrence_rate * (1.0 - b->fix_stability);
        b->status_gate = pss3_failure_gate(i, (int)(severity_avg+0.5), b->recurring_count > 0, b->rollback_count > 0, b->zombie_count > 0);
    }

    *out = buckets;
    *out_count = bucket_count;
    return 1;
}

static int cmp_count_desc(const void *a, const void *b){
    const failure_bucket *x = (const failure_bucket*)a, *y = (const failure_bucket*)b;
    if(y->count != x->count) return y->count - x->count;
    return strcmp(x->signature_hash, y->signature_hash);
}

static int cmp_beta_desc(const void *a, const void *b){
    const failure_bucket *x = (const failure_bucket*)a, *y = (const failure_bucket*)b;
    if(y->beta_risk > x->beta_risk) return 1;
    if(y->beta_risk < x->beta_risk) return -1;
    return strcmp(x->signature_hash, y->signature_hash);
}

int main(int argc, char **argv){
    const char *path = (argc>1)?argv[1]:"out/failure_trace.csv";
    failure_event *ev=NULL; int n=0;
    if(!load_csv(path,&ev,&n)){ fprintf(stderr,"failed to load %s\n",path); return 1; }

    failure_bucket *buckets = NULL; int bucket_count = 0;
    if(!build_buckets(ev, n, &buckets, &bucket_count)){ fprintf(stderr,"failed to aggregate buckets\n"); return 1; }

    printf("PSS3 loaded %d events from %s\n",n,path);
    printf("1.failure stats 2.top recurring failures 3.beta risk ranking 4.fix stability 5.gate/fibo view 6.arena failure map p.paths c.color e.emoji 0.exit\n");

    int opt='1';
    while(opt!='0' && (opt=getchar())!=EOF){
        if(opt=='1'){
            int fail=0, rec=0, zom=0, roll=0, crash=0, build=0, term=0, low=0;
            for(int i=0;i<n;i++){
                fail++; rec += to_i(ev[i].recurring)>0; zom += to_i(ev[i].zombie_detected)>0; roll += to_i(ev[i].rollback)>0;
                crash += ev[i].event_type && !strcasecmp(ev[i].event_type,"crash");
                build += ev[i].domain && !strcasecmp(ev[i].domain,"build");
                term += ev[i].domain && !strcasecmp(ev[i].domain,"terminal");
                low += ev[i].domain && (!strcasecmp(ev[i].domain,"lowlevel") || !strcasecmp(ev[i].domain,"rmr"));
            }
            printf("total_events=%d total_failures=%d recurring_failures=%d zombie_events=%d rollback_events=%d crash_events=%d build_failures=%d terminal_failures=%d lowlevel_failures=%d\n",n,fail,rec,zom,roll,crash,build,term,low);
            double pfr = (rec>0)?1.0:0.0, pfn = ((fail-rec)>0)?1.0:0.0;
            printf("delta_failure=%.3f\n", pfr-pfn);
        } else if(opt=='2'){
            failure_bucket *tmp = (failure_bucket*)malloc((size_t)bucket_count * sizeof(failure_bucket));
            if(!tmp) continue;
            memcpy(tmp, buckets, (size_t)bucket_count * sizeof(failure_bucket));
            qsort(tmp, (size_t)bucket_count, sizeof(failure_bucket), cmp_count_desc);
            for(int i=0;i<bucket_count;i++){
                printf("OPT2|rank=%d|signature_hash=%s|count=%d|recurrence_rate=%.6f|severity_sum=%.3f|recurring_count=%d|rollback_count=%d|zombie_count=%d|domain=%s|event_type=%s\n",
                    i+1, tmp[i].signature_hash, tmp[i].count, tmp[i].recurrence_rate, tmp[i].severity_sum, tmp[i].recurring_count, tmp[i].rollback_count, tmp[i].zombie_count,
                    tmp[i].domain?tmp[i].domain:"", tmp[i].event_type?tmp[i].event_type:"");
            }
            free(tmp);
        } else if(opt=='3'){
            failure_bucket *tmp = (failure_bucket*)malloc((size_t)bucket_count * sizeof(failure_bucket));
            if(!tmp) continue;
            memcpy(tmp, buckets, (size_t)bucket_count * sizeof(failure_bucket));
            qsort(tmp, (size_t)bucket_count, sizeof(failure_bucket), cmp_beta_desc);
            for(int i=0;i<bucket_count;i++){
                const char *status = (tmp[i].status_gate>=3)?"BLOCKER":(tmp[i].status_gate==2)?"HIGH":(tmp[i].status_gate==1)?"MEDIUM":"LOW";
                printf("OPT3|rank=%d|signature_hash=%s|beta_risk=%.6f|status=%s|gate=%d|severity_avg=%.3f|recurrence_rate=%.6f|fix_stability=%.6f|count=%d|attempts_after_fix=%d|stable_after_fix_count=%d\n",
                    i+1, tmp[i].signature_hash, tmp[i].beta_risk, status, tmp[i].status_gate,
                    tmp[i].count?tmp[i].severity_sum/(double)tmp[i].count:0.0, tmp[i].recurrence_rate, tmp[i].fix_stability,
                    tmp[i].count, tmp[i].attempts_after_fix, tmp[i].stable_after_fix_count);
            }
            free(tmp);
        } else if(opt=='4'){
            for(int i=0;i<bucket_count;i++){
                printf("OPT4|signature_hash=%s|fix_stability=%.6f|stable_after_fix_count=%d|attempts_after_fix=%d|count=%d|rollback_count=%d|zombie_count=%d\n",
                    buckets[i].signature_hash, buckets[i].fix_stability, buckets[i].stable_after_fix_count,
                    buckets[i].attempts_after_fix, buckets[i].count, buckets[i].rollback_count, buckets[i].zombie_count);
            }
        } else if(opt=='6'){
            int grid[4][4] = {{0}};
            for(int i=0;i<bucket_count;i++){
                int gate = buckets[i].status_gate;
                int status = (buckets[i].beta_risk > 0.75)?3:(buckets[i].beta_risk > 0.30)?2:(buckets[i].beta_risk > 0.10)?1:0;
                if(gate<0) gate=0;
                if(gate>3) gate=3;
                grid[gate][status]++;
            }
            for(int g=0;g<4;g++){
                for(int s=0;s<4;s++){
                    printf("OPT6|gate=%d|status=%d|bucket_count=%d\n", g, s, grid[g][s]);
                }
            }
        } else if(opt=='5'){
            for(int i=0;i<n && i<8;i++){
                int g=pss3_failure_gate(i,to_i(ev[i].severity),to_i(ev[i].recurring),to_i(ev[i].rollback),to_i(ev[i].zombie_detected));
                printf("tick=%d sig=%s gate=%d\n",i,ev[i].signature_hash?ev[i].signature_hash:"",g);
            }
        } else if(opt=='p'){
            printf("csv_path=%s\n",path);
        } else if(opt=='c') g_color = !g_color;
        else if(opt=='e') g_emoji = !g_emoji;
    }
    (void)g_color; (void)g_emoji;
    return 0;
}
