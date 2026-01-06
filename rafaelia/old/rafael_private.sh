#!/usr/bin/env bash
# RAFAELIA · Download seguro do repo privado Rafaelia_Private
# Usa e-mail só para extrair o username. Você só digita o PAT.

set -euo pipefail

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
chmod 700 "$ASKPASS_SCRIPT"

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

rm -f "$ASKPASS_SCRIPT"
unset GITHUB_PAT GIT_ASKPASS GIT_TERMINAL_PROMPT

echo
echo "[✓] Repositório disponível em: $TARGET_DIR"
echo "[RAFAELIA_GIT] Operação concluída."
