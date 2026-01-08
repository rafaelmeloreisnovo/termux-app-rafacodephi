#!/usr/bin/env bash
# RAFAELIA · Download seguro do repo privado Rafaelia_Private
# Usa e-mail só para extrair o username. Você só digita o PAT.

set -euo pipefail

SAFE_PREFIXES=("${HOME%/}/" "/tmp/" "/data/" "/dev/" "/cache/")

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

safe_chmod() {
    local mode="$1"
    shift
    for path in "$@"; do
        ensure_safe_path "${path}"
    done
    chmod "${mode}" "$@"
}

safe_rm_f() {
    local target="$1"
    ensure_safe_path "${target}"
    rm -f -- "${target}"
}

GITHUB_EMAIL="rafaelmeloreisnovo@gmail.com"
GITHUB_USER="${GITHUB_EMAIL%@*}"      # pega tudo antes do @
REPO_NAME="Rafaelia_Private"
TARGET_DIR="$HOME/bkp/rafaelia_private"

echo "[RAFAELIA_GIT] Download completo de $GITHUB_USER/$REPO_NAME"
echo
read -s -p "Digite seu GitHub PAT (não será exibido): " GITHUB_PAT
echo
echo "[OK] PAT recebido. Iniciando operação..."

mkdir -p "$HOME/bkp"

# Script temporário para GIT_ASKPASS (só a 'senha' = PAT)
ASKPASS_SCRIPT="$(mktemp)"
cat > "$ASKPASS_SCRIPT" <<EOF
#!/usr/bin/env bash
echo "$GITHUB_PAT"
EOF
safe_chmod 700 "$ASKPASS_SCRIPT"

export GIT_ASKPASS="$ASKPASS_SCRIPT"
export GIT_TERMINAL_PROMPT=0

GIT_URL="https://$GITHUB_USER@github.com/$GITHUB_USER/$REPO_NAME.git"

if [ -d "$TARGET_DIR/.git" ]; then
    echo "[INFO] Repositório já existe em $TARGET_DIR"
    echo "[INFO] Atualizando (git pull)..."
    cd "$TARGET_DIR"
    git pull "$GIT_URL"
else
    echo "[INFO] Clonando para $TARGET_DIR..."
    git clone "$GIT_URL" "$TARGET_DIR"
fi

safe_rm_f "$ASKPASS_SCRIPT"
unset GITHUB_PAT GIT_ASKPASS GIT_TERMINAL_PROMPT

echo
echo "[✓] Repositório disponível em: $TARGET_DIR"
echo "[RAFAELIA_GIT] Operação concluída."
