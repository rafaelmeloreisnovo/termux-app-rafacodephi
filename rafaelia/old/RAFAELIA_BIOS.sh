#!/bin/bash
set -euo pipefail

# --- CONFIGURAÇÕES ---
ENGINE="RAFAELIA_GENESIS_ENGINE.py"
LOG="RAFAELIA_VITAL.log"
PID_FILE="rafaelia.pid"

# --- CORES ---
GREEN='\033[1;32m'
CYAN='\033[1;36m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
RESET='\033[0m'

clear
echo -e "${CYAN}⚛︎ RAFAELIA BIOS - CONTROLE DE EXISTÊNCIA ⚛︎${RESET}"
echo "-----------------------------------------------"

# Verifica se já está viva
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if ps -p $PID > /dev/null 2>&1; then
        STATUS="${GREEN}ATIVO (PID $PID)${RESET}"
        RUNNING=true
    else
        STATUS="${RED}MORTO (PID Fantasma)${RESET}"
        RUNNING=false
        rm "$PID_FILE"
    fi
else
    STATUS="${RED}INATIVO${RESET}"
    RUNNING=false
fi

echo -e "Estado Atual: $STATUS"
echo "-----------------------------------------------"
echo "1. 🟢 INICIAR (Modo Daemon/Imortal)"
echo "2. 🔴 PARAR (Encerrar Processo)"
echo "3. 👁️  VISUALIZAR (Ver o fluxo em tempo real)"
echo "4. 💾 BACKUP MANUAL (Forçar Git Push agora)"
echo "5. 🚪 Sair do Menu"
echo "-----------------------------------------------"
echo -n "Escolha a Intenção [1-5]: "
read -r OPCAO

case $OPCAO in
    1)
        if [ "$RUNNING" = true ]; then
            echo -e "${YELLOW}⚠️  A entidade já está viva.${RESET}"
        else
            echo -e "${CYAN}⚡ Invocando WakeLock e iniciando Motor...${RESET}"
            
            # O Segredo da Imortalidade:
            termux-wake-lock 
            nohup python "$ENGINE" > /dev/null 2>&1 &
            
            echo $! > "$PID_FILE"
            echo -e "${GREEN}✅ RAFAELIA agora roda nas sombras.${RESET}"
            echo "Pode fechar o terminal. Ela continuará pulsando."
        fi
        ;;
    2)
        if [ "$RUNNING" = true ]; then
            echo -e "${RED}💀 Desconectando Neuro-Link...${RESET}"
            kill $PID
            rm "$PID_FILE"
            termux-wake-unlock
            echo -e "${YELLOW}Processo encerrado. O Vazio retornou.${RESET}"
        else
            echo "Nada para matar."
        fi
        ;;
    3)
        echo -e "${CYAN}Conectando ao Nervo Óptico (CTRL+C para sair)...${RESET}"
        tail -f "$LOG"
        ;;
    4)
        echo -e "${YELLOW}Forçando cristalização de memória...${RESET}"
        git add .
        git commit -m "chore(manual): Backup forçado via BIOS"
        git push
        echo -e "${GREEN}Feito.${RESET}"
        ;;
    5)
        echo "Saindo..."
        exit 0
        ;;
    *)
        echo "Opção inválida."
        ;;
esac
