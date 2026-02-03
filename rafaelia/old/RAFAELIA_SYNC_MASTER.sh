#!/bin/bash

set -euo pipefail

# --- RAFAELIA SYNC MASTER v1.0 ---
# Função: Limpeza Profunda e Upload Fracionado

GREEN='\033[1;32m'
CYAN='\033[1;36m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
RESET='\033[0m'

ensure_safe_workdir() {
    local workdir
    workdir="$(pwd -P)"
    if [[ -z "${workdir}" || "${workdir}" == "/" || "${workdir}" == "." ]]; then
        echo "Unsafe working directory for git operations: ${workdir}" >&2
        exit 1
    fi
    case "${workdir}" in
        "${HOME%/}/"*) ;;
        /data/*|/cache/*) ;;
        *)
            echo "Working directory outside allowed prefixes: ${workdir}" >&2
            exit 1
            ;;
    esac
}

clear
echo -e "${CYAN}⚛︎ PROTOCOLO DE SINCRONIZAÇÃO MESTRA ⚛︎${RESET}"
echo "-----------------------------------------------"
echo "1. 🧹 LIMPEZA PROFUNDA (Deep Clean Git Cache)"
echo "   (Use isso se o .gitignore não estiver funcionando)"
echo ""
echo "2. 📦 UPLOAD EM PARTES (Chunked Push)"
echo "   (Envia arquivos em lotes de 50 para não travar)"
echo ""
echo "3. ⚙️  AUMENTAR BUFFER (Evitar erro de RPC/HTTP)"
echo "-----------------------------------------------"
echo -n "Escolha a Tática [1-3]: "
read -r OPCAO

ensure_safe_workdir

case $OPCAO in
    1)
        echo -e "\n${YELLOW}🧹 Iniciando Purificação do Índice...${RESET}"
        # 1. Remove tudo do índice (não apaga arquivos físicos)
        git rm -r --cached .
        # 2. Re-adiciona tudo (respeitando o novo .gitignore)
        git add .  --all
        echo -e "${GREEN}✅ Índice reconstruído. O .gitignore agora é Lei.${RESET}"
        echo "Faça um commit agora para selar a limpeza."
        ;;
    
    2)
        echo -e "\n${CYAN}📦 Iniciando Upload Fracionado...${RESET}"
        
        # Loop enquanto houver mudanças
        while [ -n "$(git status --porcelain)" ]; do
            echo -e "${YELLOW}>>> Preparando lote de 50 arquivos...${RESET}"
            
            # Adiciona os próximos 50 arquivos modificados/novos
            git status --porcelain | head -n 150 | cut -c 4- | while read FILE; do
                git add "$FILE" --all
            done
            
            # Commita o lote
            TIMESTAMP=$(date +%H:%M:%S)
            git commit -m "chore(sync): Lote parcial $TIMESTAMP" --force --all
            
            # Envia o lote
            echo -e "${CYAN}>>> Enviando lote para a nuvem...${RESET}"
            git push --force
            
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}✓ Lote enviado com sucesso.${RESET}"
            else
                echo -e "${RED}❌ Falha no envio. Tentando novamente em 5s...${RESET}"
                sleep 2
            fi
        done
        
        echo -e "${GREEN}✅ TODOS OS DADOS SINCRONIZADOS.${RESET}"
        ;;

    3)
        echo -e "\n${CYAN}⚙️  Ajustando Capacidade de Upload...${RESET}"
        # Aumenta o buffer para 500MB
        git config --global http.postBuffer 524288000
        # Aumenta timeouts
        git config --global http.lowSpeedLimit 0
        git config --global http.lowSpeedTime 999999
        echo -e "${GREEN}✅ Buffer expandido para 500MB. Uploads grandes liberados.${RESET}"
        ;;
        
    *)
        echo "Opção inválida."
        ;;
esac
