/* 
 * RAFAELIA CLOUD BRAIN v2 (Termux + rclone)
 * Edge Agent para gdrive:CientiEspiritual
 * - Processa 1 arquivo por vez (zero entupimento local)
 * - Gera Heatmap, Hash Fractal, Índice CSV
 * - Failsafe para ZIP/ZIPRAF corrompidos
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>

#define REMOTE_BASE   "gdrive:CientiEspiritual"
#define LOCAL_TEMP    "temp_buffer"
#define DB_FILE       "rafaelia_master_index.csv"
#define HEATMAP_FILE  "rafaelia_knowledge.map"
#define STATUS_FILE   "raf_cloud.status"

typedef struct {
    uint64_t byte_freq[256];
    double   entropy;
    uint64_t total_processed_bytes;
} KnowledgeMap;

KnowledgeMap global_brain;

/* ---------- SYS HELPERS ---------- */

int sys_exec(const char *fmt, ...) {
    char cmd[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, args);
    va_end(args);
    return system(cmd);
}

int sys_read_line(char *buffer, size_t size, const char *fmt, ...) {
    char cmd[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, args);
    va_end(args);

    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    if (fgets(buffer, size, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        pclose(fp);
        return 1;
    }
    pclose(fp);
    return 0;
}

/* ---------- NÚCLEO MATEMÁTICO ---------- */

void analyze_chunk(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        global_brain.byte_freq[data[i]]++;
    }
    global_brain.total_processed_bytes += len;
}

double calculate_entropy() {
    if (global_brain.total_processed_bytes == 0) return 0.0;
    double ent = 0.0;
    for (int i = 0; i < 256; i++) {
        if (global_brain.byte_freq[i] > 0) {
            double p = (double)global_brain.byte_freq[i] /
                       (double)global_brain.total_processed_bytes;
            ent -= p * (log(p) / log(2.0));
        }
    }
    global_brain.entropy = ent;
    return ent;
}

uint64_t fractal_hash(const char *filepath, int *ok) {
    FILE *f = fopen(filepath, "rb");
    if (!f) { 
        *ok = 0;
        return 0;
    }

    uint64_t hash = 0xCBF29CE484222325ULL;
    uint8_t buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        analyze_chunk(buf, n);
        for (size_t i = 0; i < n; i++) {
            hash ^= buf[i];
            hash *= 0x100000001B3ULL;
        }
    }
    if (ferror(f)) {
        *ok = 0;
    } else {
        *ok = 1;
    }
    fclose(f);
    return hash;
}

/* ---------- PERSISTÊNCIA ---------- */

void save_brain() {
    FILE *f = fopen(HEATMAP_FILE, "wb");
    if (f) {
        fwrite(&global_brain, sizeof(KnowledgeMap), 1, f);
        fclose(f);
    }
    sys_exec("rclone copy %s \"%s/99_system_data/\"", HEATMAP_FILE, REMOTE_BASE);
}

void append_to_index(const char *filename,
                     const char *class_tag,
                     uint64_t hash,
                     double entropy,
                     const char *status) {
    FILE *f = fopen(DB_FILE, "a");
    if (f) {
        fprintf(f, "%ld,%s,%s,%016lx,%.4f,%s\n",
                time(NULL), filename, class_tag, hash, entropy, status);
        fclose(f);
    }
    sys_exec("rclone copy %s \"%s/99_system_data/\"", DB_FILE, REMOTE_BASE);
}

void update_status(const char *msg) {
    FILE *f = fopen(STATUS_FILE, "w");
    if (f) {
        fprintf(f, "%ld %s\n", time(NULL), msg);
        fclose(f);
    }
    sys_exec("rclone copy %s \"%s/98_onrunning/\"", STATUS_FILE, REMOTE_BASE);
}

/* ---------- CLASSIFICAÇÃO ---------- */

const char *classify_by_name(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return "unknown";

    if (!strcasecmp(dot, ".png") || !strcasecmp(dot, ".jpg") ||
        !strcasecmp(dot, ".jpeg") || !strcasecmp(dot, ".webp"))
        return "image";

    if (!strcasecmp(dot, ".json") || !strcasecmp(dot, ".txt") ||
        !strcasecmp(dot, ".md"))
        return "text";

    if (!strcasecmp(dot, ".zip") || !strcasecmp(dot, ".rafzip") ||
        !strcasecmp(dot, ".zipraf"))
        return "archive";

    return "other";
}

int test_zip(const char *path) {
    /* retorna 1 se ok, 0 se corrompido */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "unzip -tqq \"%s\" > /dev/null 2>&1", path);
    int r = system(cmd);
    return (r == 0);
}

/* ---------- AMBIENTE ---------- */

void init_environment() {
    printf("::: RAFAELIA CLOUD BRAIN v2 :::\n");
    printf("::: Remote: %s :::\n", REMOTE_BASE);

    sys_exec("mkdir -p %s", LOCAL_TEMP);

    FILE *f = fopen(HEATMAP_FILE, "rb");
    if (f) {
        fread(&global_brain, sizeof(KnowledgeMap), 1, f);
        fclose(f);
        printf("[MEMORIA] Heatmap carregado.\n");
    } else {
        memset(&global_brain, 0, sizeof(KnowledgeMap));
        printf("[MEMORIA] Criando novo heatmap.\n");
    }

    update_status("INICIADO");
}

/* ---------- PROCESSAMENTO DE 1 ARQUIVO ---------- */

int process_next_file() {
    char filename[1024];

    if (!sys_read_line(filename, sizeof(filename),
        "rclone lsf \"%s/0_inbox\" --max-depth 1 --format \"p\" | head -n 1",
        REMOTE_BASE)) {
        return 0;
    }
    if (strlen(filename) < 1) return 0;

    printf("\n[DETECTADO] %s\n", filename);
    update_status(filename);

    /* 1) MOVE PARA PROCESSING (LOCK) */
    printf("  -> Cloud LOCK em 1_processing...\n");
    if (sys_exec("rclone move \"%s/0_inbox/%s\" \"%s/1_processing/\"",
                 REMOTE_BASE, filename, REMOTE_BASE) != 0) {
        printf("  !! Falha ao mover para 1_processing.\n");
        return 0;
    }

    /* 2) BAIXA PARA TEMP LOCAL */
    printf("  -> Download Edge (local)...\n");
    sys_exec("rclone copy \"%s/1_processing/%s\" \"%s/\"",
             REMOTE_BASE, filename, LOCAL_TEMP);

    char local_path[1024];
    snprintf(local_path, sizeof(local_path), "%s/%s", LOCAL_TEMP, filename);

    /* Checa se o arquivo veio mesmo */
    struct stat st;
    if (stat(local_path, &st) != 0) {
        printf("  !! Arquivo não encontrado em temp. Mandando para 3_error.\n");
        sys_exec("rclone move \"%s/1_processing/%s\" \"%s/3_error/\"",
                 REMOTE_BASE, filename, REMOTE_BASE);
        append_to_index(filename, "unknown", 0, 0.0, "EDGE_MISS");
        return 1;
    }

    const char *class_tag = classify_by_name(filename);
    int ok_read = 0;
    uint64_t hash = fractal_hash(local_path, &ok_read);
    double entropy = calculate_entropy();

    if (!ok_read) {
        printf("  !! Erro ao ler arquivo. Enviando para 3_error.\n");
        sys_exec("rclone move \"%s/1_processing/%s\" \"%s/3_error/\"",
                 REMOTE_BASE, filename, REMOTE_BASE);
        append_to_index(filename, class_tag, hash, entropy, "READ_ERROR");
        sys_exec("rm -f \"%s\"", local_path);
        save_brain();
        return 1;
    }

    printf("     [CLASS] %s | [HASH] %016lx | [H] %.4f\n",
           class_tag, hash, entropy);

    /* 3) TRATAMENTO ESPECIAL ZIP / ZIPRAF */
    int is_archive = !strcmp(class_tag, "archive");
    int is_broken = 0;
    if (is_archive) {
        printf("  -> Testando integridade do arquivo de arquivo...\n");
        if (!test_zip(local_path)) {
            printf("     !! ZIP/ZIPRAF corrompido. Failsafe.\n");
            is_broken = 1;
        }
    }

    const char *target_folder = NULL;

    if (is_broken) {
        target_folder = "9_failsafe";
    } else {
        if (!strcmp(class_tag, "text")) {
            target_folder = "2_processed/Text_Docs";
        } else if (!strcmp(class_tag, "image")) {
            target_folder = "2_processed/Media_Files";
        } else if (!strcmp(class_tag, "archive")) {
            target_folder = "2_processed/High_Entropy_Archives";
        } else {
            /* usa entropia como fallback */
            if (entropy > 7.5)
                target_folder = "2_processed/High_Entropy_Archives";
            else if (entropy < 3.0)
                target_folder = "2_processed/Text_Docs";
            else
                target_folder = "2_processed/Media_Files";
        }
    }

    printf("  -> Destino final: %s\n", target_folder);

    sys_exec("rclone move \"%s/1_processing/%s\" \"%s/%s/\"",
             REMOTE_BASE, filename, REMOTE_BASE, target_folder);

    append_to_index(filename, class_tag, hash, entropy,
                    is_broken ? "FAILSAFE" : "OK");

    save_brain();

    printf("  -> Limpando cache local.\n");
    sys_exec("rm -f \"%s\"", local_path);

    update_status("IDLE");
    return 1;
}

/* ---------- MAIN LOOP ---------- */

int main() {
    init_environment();
    printf("[SISTEMA] Aguardando novos arquivos na Inbox...\n");

    int idle_count = 0;
    while (1) {
        if (process_next_file()) {
            idle_count = 0;
        } else {
            idle_count++;
            int sleep_time = (idle_count > 5) ? 10 : 3;
            printf("\r[AGUARDANDO] Inbox vazia. Novo scan em %ds...   ",
                   sleep_time);
            fflush(stdout);
            sleep(sleep_time);
        }
    }
    return 0;
}
