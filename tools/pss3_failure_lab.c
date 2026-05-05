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
    char timestamp[MAX_STR];
    char run_id[MAX_STR];
    char domain[MAX_STR];
    char signature_hash[MAX_STR];
    char event_type[MAX_STR];
    char severity[MAX_STR];
    char recurring[MAX_STR];
    char stable_after_fix[MAX_STR];
    char rollback[MAX_STR];
    char gate[MAX_STR];
    char zombie_detected[MAX_STR];
    char exit_code[MAX_STR];
    char signal[MAX_STR];
    char errno_s[MAX_STR];
} failure_event;

typedef struct {
    char signature_hash[MAX_STR];
    char domain[MAX_STR];
    char event_type[MAX_STR];
    int count;
    int recurring_count;
    int stable_after_fix_count;
    int attempts_after_fix;
    int rollback_count;
    int zombie_count;
    double severity_sum;
    double recurrence_rate;
    double fix_stability;
    double beta_risk;
    int gate_class;
} failure_bucket;

static const int fibo_01123[] = {0, 1, 1, 2, 3};
static const int fibo_001123[] = {0, 0, 1, 1, 2, 3};

static int to_i(const char *s) { return (s && *s) ? atoi(s) : 0; }
static double to_d(const char *s) { return (s && *s) ? atof(s) : 0.0; }
static int str_eq(const char *a, const char *b) { return a && b && !strcasecmp(a, b); }
static void cpy(char *dst, const char *src) { snprintf(dst, MAX_STR, "%s", src ? src : ""); }

static int csv_split_simple(char *line, char **cells, int max_cells) {
    int c = 0;
    char *tok = strtok(line, ",\n\r");
    while (tok && c < max_cells) {
        cells[c++] = tok;
        tok = strtok(NULL, ",\n\r");
    }
    return c;
}

static const char *cell_at(char **cells, int n, int idx) {
    return (idx >= 0 && idx < n && cells[idx]) ? cells[idx] : "";
}

static int pss3_failure_gate(int tick, int severity, int recurring, int rollback, int zombie) {
    if (zombie) return 3;
    if (rollback && recurring) return 3;
    int seq = fibo_01123[tick % 5] + fibo_001123[tick % 6];
    int score = severity + recurring + rollback + (seq >= 3 ? 1 : 0);
    if (score <= 1) return 0;
    if (score <= 3) return 1;
    if (score <= 5) return 2;
    return 3;
}

static const char *status_from_bucket(const failure_bucket *b) {
    if (b->fix_stability >= 0.90 && b->recurrence_rate < 0.05) return "FIXED_LIKELY";
    if (b->beta_risk >= 0.75 || b->gate_class == 3) return "BLOCKER_BETA";
    if (b->recurrence_rate >= 0.20 && b->beta_risk >= 0.40) return "CRITICAL_RECURRENT";
    if (b->recurrence_rate >= 0.10) return "RECURRENT";
    if (b->beta_risk <= 0.05 && b->count == 1) return "NOISE_LIKELY";
    if (b->beta_risk >= 0.12) return "WATCH";
    return "STABLE_OK";
}

static int load_csv(const char *path, failure_event *events, int *count) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char line[MAX_LINE], *cells[MAX_CELLS];
    int idx_ts=-1, idx_run=-1, idx_dom=-1, idx_sig=-1, idx_evt=-1, idx_sev=-1, idx_rec=-1, idx_stf=-1, idx_roll=-1, idx_gate=-1, idx_zom=-1, idx_exit=-1, idx_signal=-1, idx_errno=-1;

    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return 0; }
    int hc = csv_split_simple(line, cells, MAX_CELLS);
    for (int i = 0; i < hc; ++i) {
        if (str_eq(cells[i], "timestamp")) idx_ts = i;
        else if (str_eq(cells[i], "run_id")) idx_run = i;
        else if (str_eq(cells[i], "domain")) idx_dom = i;
        else if (str_eq(cells[i], "signature_hash")) idx_sig = i;
        else if (str_eq(cells[i], "event_type")) idx_evt = i;
        else if (str_eq(cells[i], "severity")) idx_sev = i;
        else if (str_eq(cells[i], "recurring")) idx_rec = i;
        else if (str_eq(cells[i], "stable_after_fix")) idx_stf = i;
        else if (str_eq(cells[i], "rollback")) idx_roll = i;
        else if (str_eq(cells[i], "gate")) idx_gate = i;
        else if (str_eq(cells[i], "zombie_detected")) idx_zom = i;
        else if (str_eq(cells[i], "exit_code")) idx_exit = i;
        else if (str_eq(cells[i], "signal")) idx_signal = i;
        else if (str_eq(cells[i], "errno")) idx_errno = i;
    }
    if (idx_sig < 0) { fclose(fp); return 0; }

    int n = 0;
    while (n < MAX_EVENTS && fgets(line, sizeof(line), fp)) {
        int cc = csv_split_simple(line, cells, MAX_CELLS);
        if (cc <= 0) continue;
        cpy(events[n].timestamp, cell_at(cells, cc, idx_ts));
        cpy(events[n].run_id, cell_at(cells, cc, idx_run));
        cpy(events[n].domain, cell_at(cells, cc, idx_dom));
        cpy(events[n].signature_hash, cell_at(cells, cc, idx_sig));
        cpy(events[n].event_type, cell_at(cells, cc, idx_evt));
        cpy(events[n].severity, cell_at(cells, cc, idx_sev));
        cpy(events[n].recurring, cell_at(cells, cc, idx_rec));
        cpy(events[n].stable_after_fix, cell_at(cells, cc, idx_stf));
        cpy(events[n].rollback, cell_at(cells, cc, idx_roll));
        cpy(events[n].gate, cell_at(cells, cc, idx_gate));
        cpy(events[n].zombie_detected, cell_at(cells, cc, idx_zom));
        cpy(events[n].exit_code, cell_at(cells, cc, idx_exit));
        cpy(events[n].signal, cell_at(cells, cc, idx_signal));
        cpy(events[n].errno_s, cell_at(cells, cc, idx_errno));
        n++;
    }
    fclose(fp);
    *count = n;
    return 1;
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "out/failure_trace.csv";
    failure_event *events = calloc(MAX_EVENTS, sizeof(failure_event));
    failure_bucket *buckets = NULL;
    if (!events) return 1;

    int n = 0;
    if (!load_csv(path, events, &n)) { fprintf(stderr, "failed to load %s\n", path); free(events); return 1; }

    int bcount = 0, bcap = 0;
    for (int i = 0; i < n; i++) {
        int bi = -1;
        for (int j = 0; j < bcount; j++) if (!strcmp(buckets[j].signature_hash, events[i].signature_hash)) { bi = j; break; }
        if (bi < 0) {
            if (bcount >= bcap) {
                int next = (bcap == 0) ? 64 : bcap * 2;
                failure_bucket *tmp = realloc(buckets, (size_t)next * sizeof(*buckets));
                if (!tmp) { free(events); free(buckets); return 1; }
                buckets = tmp;
                memset(buckets + bcap, 0, (size_t)(next - bcap) * sizeof(*buckets));
                bcap = next;
            }
            bi = bcount++;
            cpy(buckets[bi].signature_hash, events[i].signature_hash);
        }
        failure_bucket *b = &buckets[bi];
        if (!b->domain[0]) cpy(b->domain, events[i].domain);
        if (!b->event_type[0]) cpy(b->event_type, events[i].event_type);
        b->count++;
        b->severity_sum += to_d(events[i].severity);
        b->recurring_count += to_i(events[i].recurring) > 0;
        b->stable_after_fix_count += to_i(events[i].stable_after_fix) > 0;
        b->attempts_after_fix += events[i].stable_after_fix[0] ? 1 : 0;
        b->rollback_count += to_i(events[i].rollback) > 0;
        b->zombie_count += to_i(events[i].zombie_detected) > 0;
    }

    for (int i = 0; i < bcount; i++) {
        failure_bucket *b = &buckets[i];
        double severity_avg = b->count ? b->severity_sum / b->count : 0.0;
        b->recurrence_rate = n ? (double)b->count / n : 0.0;
        b->fix_stability = b->attempts_after_fix ? (double)b->stable_after_fix_count / b->attempts_after_fix : 0.0;
        b->beta_risk = severity_avg * b->recurrence_rate * (1.0 - b->fix_stability);
        b->gate_class = pss3_failure_gate(i, (int)(severity_avg + 0.5), b->recurring_count > 0, b->rollback_count > 0, b->zombie_count > 0);
    }

    printf("PSS3 loaded %d events from %s\n", n, path);
    printf("1. failure stats\n2. top recurring failures\n3. beta risk ranking\n4. fix stability\n5. gate/fibo view\n6. arena failure map\np. paths\nc. color on/off\ne. emoji on/off\n0. exit\n");
    printf("delta_failure= p(fail|recurring)-p(fail|non_recurring), beta_risk=severity*recurrence_rate*(1-fix_stability)\n");
    printf("csv parser note: simple CSV (no quoted commas)\n");

    for (int i = 0; i < bcount; i++) {
        const char *status = status_from_bucket(&buckets[i]);
        printf("ROW|signature_hash=%s|domain=%s|event_type=%s|count=%d|recurrence=%.6f|severity=%.3f|fix_stability=%.6f|beta_risk=%.6f|status=%s\n",
               buckets[i].signature_hash, buckets[i].domain, buckets[i].event_type, buckets[i].count, buckets[i].recurrence_rate,
               buckets[i].count ? buckets[i].severity_sum / buckets[i].count : 0.0, buckets[i].fix_stability, buckets[i].beta_risk, status);
    }

    free(buckets);
    free(events);
    return 0;
}
