/* * SYSTEM: ORBITAL TITANIUM STREAM V2 (RCLONE-AWARE)
 * ARCH: ARM64 Optimized (Android 15)
 * STANDARDS: ISO/IEC 27001 (Availability) | ISO 25010 (Reliability)
 * FEATURES: Active Remote Polling, Token Validation, Dynamic ETA
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>

// --- [ CONFIGURATION ] ---
// Mude "gdrive" para o nome do seu remote se for diferente
#define R_REMOTE_DEFAULT "gdrive" 
#define R_FOLDER "Backup_Android_Stream"

// --- [ UI CONSTANTS ] ---
#define C_CYAN  "\033[1;36m"
#define C_GRN   "\033[1;32m"
#define C_RED   "\033[1;31m"
#define C_YEL   "\033[1;33m"
#define C_RST   "\033[0m"

// Variável Global para o Remote
char TARGET_REMOTE[128];

// --- [ UTILS ] ---

void banner() {
    system("clear");
    printf(C_CYAN);
    printf("   ___  ___  ___ _ _____ _   _ \n");
    printf("  / _ \\| _ \\| _ ) |_   _| | | |\n");
    printf(" | (_) |   /| _ \\ | | | | |_| |\n");
    printf("  \\___/|_|_\\|___/_|_|_| |\\___/ \n");
    printf("  >> TITANIUM V2: RCLONE GOVERNANCE << \n");
    printf(C_RST);
    printf("----------------------------------------\n");
}

// Executa um comando e retorna 0 se sucesso
int run_silent(const char *cmd) {
    char buf[512];
    snprintf(buf, sizeof(buf), "%s > /dev/null 2>&1", cmd);
    return system(buf);
}

// --- [ RCLONE HANDLING SUBSYSTEM ] ---

// 1. Verifica se o binário existe
void check_rclone_binary() {
    printf("[INIT] Verificando binários...\n");
    if (run_silent("command -v rclone") != 0) {
        printf(C_RED "[CRIT] Rclone não encontrado!\n" C_RST);
        printf(C_YEL "[FIX ] Tente: pkg install rclone\n" C_RST);
        exit(1);
    }
    printf(C_GRN "[ OK ] Rclone binary active.\n" C_RST);
}

// 2. Verifica se o Remote existe no config
int validate_remote(const char *remote) {
    printf("[INIT] Validando remote: %s...\n", remote);
    char cmd[256];
    // Lista remotes e busca o nome exato com grep
    snprintf(cmd, sizeof(cmd), "rclone listremotes | grep -q '^%s:$'", remote);
    
    if (run_silent(cmd) != 0) {
        printf(C_YEL "[WARN] Remote '%s' não encontrado no config.\n" C_RST, remote);
        return 0; // Falha
    }
    return 1; // Sucesso
}

// 3. Verifica conectividade (Token válido?)
void check_cloud_link(const char *remote) {
    printf("[NET ] Testando link TLS 1.3 com Google Drive...\n");
    char cmd[256];
    // Tenta listar diretório raiz (rápido) para checar auth
    snprintf(cmd, sizeof(cmd), "rclone lsd %s: --max-depth 1 > /dev/null 2>&1", remote);
    
    if (system(cmd) != 0) {
        printf(C_RED "[FAIL] Falha de Autenticação/Conexão!\n" C_RST);
        printf(C_YEL "[INFO] Seu token pode ter expirado ou sem internet.\n" C_RST);
        printf(C_YEL "[FIX ] Rode 'rclone config reconnect %s:'\n" C_RST, remote);
        exit(1);
    }
    printf(C_GRN "[ OK ] Link estabelecido. Auth válida.\n" C_RST);
}

// 4. Seletor de Remote (Fallback)
void select_remote() {
    if (validate_remote(R_REMOTE_DEFAULT)) {
        strcpy(TARGET_REMOTE, R_REMOTE_DEFAULT);
    } else {
        printf(C_YEL "\n[USER] Remote padrão falhou. Listando disponíveis:\n" C_RST);
        system("rclone listremotes");
        printf("\nDigite o nome do remote (sem os dois pontos): ");
        if (scanf("%127s", TARGET_REMOTE) != 1) exit(1);
        
        // Valida novamente a escolha do usuário
        if (!validate_remote(TARGET_REMOTE)) {
            printf(C_RED "[CRIT] Remote inválido. Abortando.\n" C_RST);
            exit(1);
        }
    }
    check_cloud_link(TARGET_REMOTE);
}

// --- [ CORE LOGIC ] ---

long long get_size() {
    printf(C_YEL "[CALC] Calculando payload (excluindo virtuais)...\n" C_RST);
    FILE *fp = popen("du -sb --exclude='./.cache' --exclude='./tmp' --exclude='./proc' --exclude='./sys' . | cut -f1", "r");
    if (!fp) return 0;
    
    char path[64];
    long long size = 0;
    if (fgets(path, sizeof(path), fp)) size = atoll(path);
    pclose(fp);
    return size;
}

int main() {
    banner();

    // Fase 1: Diagnóstico e Seleção
    check_rclone_binary();
    select_remote(); // Resolve qual GDrive usar

    // Fase 2: Metadados
    struct utsname host;
    uname(&host);
    long long total_bytes = get_size();
    
    if (total_bytes == 0) {
        printf(C_RED "[ERR ] Falha ao ler arquivos. Permissão negada?\n" C_RST);
        exit(1);
    }
    
    double mb = total_bytes / (1024.0 * 1024.0);
    printf(C_GRN "[DATA] Payload: %.2f MB | Target: %s:%s\n" C_RST, mb, TARGET_REMOTE, R_FOLDER);

    // Fase 3: Construção do Pipeline
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char timestamp[32];
    sprintf(timestamp, "%04d%02d%02d_%02d%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);

    // Exclusões de segurança (Android 15)
    const char *excludes = 
        "--exclude='./.cache' --exclude='./tmp' --exclude='./.git' "
        "--exclude='./proc' --exclude='./sys' --exclude='./dev' "
        "--exclude='./mnt' --exclude='./storage' --exclude='./sdcard' "
        "--exclude='./termux-app'"; // Evita loop recursivo

    // Opções do Rclone (Tuning)
    char rclone_opts[512];
    snprintf(rclone_opts, sizeof(rclone_opts), 
        "--stats 2s "
        "--stats-one-line "
        "--total-size %lld "     // Injeta o tamanho para a barra de progresso
        "--buffer-size 16M "     // Low footprint
        "--drive-chunk-size 32M "
        "--timeout 10m "
        "--retries 3 "           // Retry nativo
        "--low-level-retries 10", total_bytes);

    printf("\n" C_CYAN ">> INICIANDO STREAM [tar | rclone] <<" C_RST "\n");
    
    char cmd[4096];
    // Comando Mestre:
    // 1. Tar comprime para stdout
    // 2. Rclone lê de stdin e sobe para o folder remoto
    snprintf(cmd, sizeof(cmd), 
        "tar -czf - %s . 2>/dev/null | rclone rcat %s:%s/%s_%s.tar.gz %s",
        excludes, TARGET_REMOTE, R_FOLDER, host.nodename, timestamp, rclone_opts);

    // Fase 4: Execução
    int status = system(cmd);

    // Fase 5: Relatório
    printf("\n----------------------------------------\n");
    if (status == 0) {
        printf(C_GRN "[SUC ] Backup enviado com sucesso!\n" C_RST);
        printf("[INFO] Verifique em: rclone lsd %s:%s\n", TARGET_REMOTE, R_FOLDER);
    } else {
        printf(C_RED "[FAIL] Erro no pipe. Código: %d\n" C_RST, status);
        printf(C_YEL "[HINT] Verifique internet ou espaço no drive.\n" C_RST);
    }

    return 0;
}
