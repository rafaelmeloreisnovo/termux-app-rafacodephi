#!/bin/bash

# ==============================================================================
# PROJECT: RANOVO SILENT (BUFFERED FULLSCREEN)
# FIX: PREVENTS SCROLLING/FLICKERING ON MOBILE TERMINALS
# ENGINE: ALTERNATE SCREEN BUFFER + DYNAMIC HEIGHT
# ==============================================================================

set -euo pipefail

# --- CONFIGURAÇÃO ---
RCLONE_REMOTE="ranovo"
REMOTE_PATH_ROOT="/_Backup_Cripto"
CURRENT_DIR="/storage/0886-EC05" # Ajuste se necessário, ou use $PWD
TEMP_LIST="${TMPDIR:-/tmp}/ranovo_list.tmp"

SAFE_PREFIXES=("${TMPDIR:-/tmp}/" "/data/" "/dev/" "/cache/" "${HOME%/}/")

ensure_safe_path() {
    local path="$1"
    if [[ -z "${path}" || "${path}" == "/" || "${path}" == "." || ${#path} -lt 5 ]]; then
        echo "Unsafe path: '${path}'" >&2
        exit 1
    fi
    local normalized
    normalized="$(realpath -m "${path}")"
    local safe=false
    for prefix in "${SAFE_PREFIXES[@]}"; do
        if [[ "${normalized}" == "${prefix}"* ]]; then
            safe=true
            break
        fi
    done
    if [[ "${safe}" != true ]]; then
        echo "Path outside allowed prefixes: ${normalized}" >&2
        exit 1
    fi
}

safe_rm_f() {
    local target="$1"
    ensure_safe_path "${target}"
    rm -f -- "${target}"
}

# --- CONTROLE ---
declare -A selected_b64
CURSOR=0
OFFSET=0

# --- CORES ---
C_RESET=$(tput sgr0)
C_INV=$(tput rev)
C_GREEN=$(tput setaf 2)
C_CYAN=$(tput setaf 6)
C_BLUE=$(tput setaf 4)
C_WHITE=$(tput setaf 7)
C_RED=$(tput setaf 1)

# --- TRATAMENTO DE SAÍDA (Obrigatorio para não quebrar o terminal) ---
cleanup() {
    tput rmcup # Sai do modo fullscreen
    tput cnorm # Mostra cursor
    stty echo  # Ativa escrita
    safe_rm_f "$TEMP_LIST"
}
trap cleanup EXIT INT TERM

# --- INICIALIZAÇÃO VISUAL ---
tput smcup  # Entra no modo fullscreen (sem histórico)
tput civis  # Esconde cursor
stty -echo  # Não mostra teclas digitadas

# --- ENGINE: CARREGAMENTO BLINDADO (BASE64) ---
load_directory() {
    : > "$TEMP_LIST"
    
    # Adiciona ".."
    parent=$(dirname "$CURRENT_DIR")
    echo "D|$(echo ".." | base64 -w0)|$(echo "$parent" | base64 -w0)" >> "$TEMP_LIST"
    
    # Lista Pastas
    find "$CURRENT_DIR" -maxdepth 1 -mindepth 1 -type d | sort | \
    while read -r line; do
        name=$(basename "$line")
        echo "D|$(echo "$name/" | base64 -w0)|$(echo "$line" | base64 -w0)" >> "$TEMP_LIST"
    done

    # Lista Arquivos
    find "$CURRENT_DIR" -maxdepth 1 -mindepth 1 -type f | sort | \
    while read -r line; do
        name=$(basename "$line")
        echo "F|$(echo "$name" | base64 -w0)|$(echo "$line" | base64 -w0)" >> "$TEMP_LIST"
    done
    
    TOTAL_LINES=$(wc -l < "$TEMP_LIST")
}

# --- ENGINE: DESENHO OTIMIZADO ---
draw() {
    # 1. Detectar altura atual do terminal (evita quebra de linha)
    TERM_H=$(tput lines)
    TERM_W=$(tput cols)
    
    # Cabeçalho (2 linhas) + Rodapé (2 linhas) = 4 linhas fixas
    LIST_H=$((TERM_H - 5))
    if [ $LIST_H -lt 1 ]; then LIST_H=1; fi

    # 2. Lógica de Janela (Scrolling)
    if [ $CURSOR -ge $((OFFSET + LIST_H)) ]; then OFFSET=$((CURSOR - LIST_H + 1)); fi
    if [ $CURSOR -lt $OFFSET ]; then OFFSET=$CURSOR; fi

    # 3. Construir Buffer de Tela (String única para evitar flicker)
    BUFFER=""
    
    # Move p/ topo
    tput cup 0 0
    
    # Cabeçalho
    echo "${C_GREEN}:: RANOVO SILENT :: ${C_CYAN}${CURRENT_DIR: -30} ${C_RESET}"
    echo "${C_WHITE}Itens: $TOTAL_LINES | Sel: ${#selected_b64[@]}${C_RESET}"
    
    # Lista
    limit=$((OFFSET + LIST_H))
    
    # Loop de renderização (Lê do arquivo temporário)
    i=0
    # O 'sed' pega exatamente a fatia que cabe na tela
    sed -n "$((OFFSET + 1)),${limit}p" "$TEMP_LIST" | while IFS='|' read -r type b64_name b64_path; do
        real_idx=$((OFFSET + i))
        
        # Decode só para display
        name=$(echo "$b64_name" | base64 -d)
        # Corta nome se for maior que a tela
        max_name_len=$((TERM_W - 5))
        name="${name:0:$max_name_len}"
        
        # Ícone
        if [ "$type" == "D" ]; then icon="${C_BLUE}DIR${C_RESET}"; else icon="   "; fi
        
        # Checkbox
        if [[ -v "selected_b64[$b64_path]" ]]; then check="${C_GREEN}[X]${C_RESET}"; else check="[ ]"; fi
        
        # Formata linha
        line_str=" $check $icon $name"
        
        # Highlight Cursor
        if [ $real_idx -eq $CURSOR ]; then
            # Imprime linha invertida e limpa até o fim da linha (el)
            printf "${C_INV}%-$(tput cols)s${C_RESET}\n" "$line_str"
        else
            # Imprime linha normal e limpa até o fim da linha (el)
            printf "%s\033[K\n" "$line_str"
        fi
        ((i++))
    done
    
    # Limpa linhas vazias se a lista for curta
    while [ $i -lt $LIST_H ]; do
        echo "" # Linha em branco
        ((i++))
    done
    
    # Rodapé
    echo "--------------------------------------------------"
    printf "${C_INV} [SETAS]=Mover  [ESPAÇO]=Marcar  [E]=Enviar ${C_RESET}"
}

# --- LOOP PRINCIPAL ---
load_directory

while true; do
    draw
    
    # Leitura de Tecla (Silenciosa)
    read -rsn1 key
    if [[ "$key" == $'\x1b' ]]; then
        read -rsn2 key
        case "$key" in
            '[A') ((CURSOR--)); if [ $CURSOR -lt 0 ]; then CURSOR=0; fi ;;
            '[B') ((CURSOR++)); if [ $CURSOR -ge $((TOTAL_LINES-1)) ]; then CURSOR=$((TOTAL_LINES-1)); fi ;;
        esac
    else
        case "$key" in
            "") # Enter (Navegar)
                line_data=$(sed -n "$((CURSOR + 1))p" "$TEMP_LIST")
                type=$(echo "$line_data" | cut -d'|' -f1)
                if [ "$type" == "D" ]; then
                    b64_path=$(echo "$line_data" | cut -d'|' -f3)
                    path=$(echo "$b64_path" | base64 -d)
                    if [ -d "$path" ]; then
                        CURRENT_DIR="$path"
                        CURSOR=0; OFFSET=0
                        load_directory
                    fi
                fi
                ;;
            " ") # Espaço (Marcar)
                line_data=$(sed -n "$((CURSOR + 1))p" "$TEMP_LIST")
                b64_name=$(echo "$line_data" | cut -d'|' -f2)
                name=$(echo "$b64_name" | base64 -d)
                if [ "$name" != ".." ]; then
                    b64_path=$(echo "$line_data" | cut -d'|' -f3)
                    if [[ -v "selected_b64[$b64_path]" ]]; then
                        unset "selected_b64[$b64_path]"
                    else
                        selected_b64["$b64_path"]=1
                    fi
                fi
                ;;
            "e"|"E") # Enviar
                if [ ${#selected_b64[@]} -gt 0 ]; then break; fi
                ;;
            "q"|"Q") exit 0 ;;
        esac
    fi
done

# --- FASE DE UPLOAD ---
cleanup # Restaura tela normal para ver o log do Rclone
clear
echo -e "${C_GREEN}:: INICIANDO UPLOAD ::${C_RESET}"

read -p "Banda (ex: 1M, Enter=Max): " bw
bw=${bw:-0}

for b64 in "${!selected_b64[@]}"; do
    file=$(echo "$b64" | base64 -d)
    fname=$(basename "$file")
    
    echo -e "${C_CYAN}>> Processando: $fname${C_RESET}"
    rclone move "$file" "$RCLONE_REMOTE:$REMOTE_PATH_ROOT/$fname" \
        --bwlimit "$bw" --checksum --stats-one-line -P
done
