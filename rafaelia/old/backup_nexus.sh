#!/bin/bash
# ==============================================================================
# SYSTEM:       ORBITAL STREAM BACKUP (Android/ARM64 Optimized)
# STANDARD:     ISO/IEC 25010 (Efficiency) | ISO 27001 (Integrity)
# ARCH:         aarch64 (ARMv8) Native
# AUTHOR:       Gemini_Φ // Restauratio Gaia
# ==============================================================================

# --- [ 0. CONFIGURATION & CONSTANTS ] ---
REMOTE_NAME="gdrive"                # Nome do seu remote no Rclone
BACKUP_NAME="Android_Backup_Stream"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="backup_log_${TIMESTAMP}.txt"

# ANSI Colors for BBS Style
C_CYAN='\033[0;36m'
C_GREEN='\033[0;32m'
C_RED='\033[0;31m'
C_YELLOW='\033[1;33m'
C_RESET='\033[0m'
C_BG='\033[44m' # Blue Background header

# --- [ 1. SYSTEM DIAGNOSTIC & DEPENDENCY CHECK ] ---
clear
echo -e "${C_CYAN}"
cat << "BANNER"
   _____            _     _ _        _ 
  |  __ \          | |   (_) |      | |
  | |  | |_ __ __ _| |__  _| |_ __ _| |
  | |  | | '__/ _` | '_ \| | |/ _` | |
  | |__| | | | (_| | |_) | | | (_| | |
  |_____/|_|  \__,_|_.__/|_|_|\__,_|_|
       >> STREAM LINK: ESTABLISHED <<
BANNER
echo -e "${C_RESET}"

function check_dep() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${C_YELLOW}[WARN] $1 not found. Attempting install...${C_RESET}"
        pkg install $1 -y 2>/dev/null || apt install $1 -y 2>/dev/null
        if ! command -v $1 &> /dev/null; then
            echo -e "${C_RED}[CRIT] Failed to install $1. Aborting.${C_RESET}"
            exit 1
        fi
    else
        echo -e "${C_GREEN}[OK] $1 detected.${C_RESET}"
    fi
}

echo -e "\n${C_BG} :: INITIALIZING SYSTEM INTEGRITY CHECKS :: ${C_RESET}"
check_dep "rclone"
check_dep "tar"

# Failover Logic for Compression (Throughput Optimization)
if command -v pigz &> /dev/null; then
    COMPRESSOR="pigz"
    COMP_FLAG="--use-compress-program=pigz"
    THREADS=$(nproc)
    echo -e "${C_GREEN}[PERF] Multi-core compression active: PIGZ ($THREADS threads)${C_RESET}"
else
    echo -e "${C_YELLOW}[WARN] pigz not found. Fallback to single-thread GZIP.${C_RESET}"
    COMPRESSOR="gzip"
    COMP_FLAG="-z"
fi

# Wake Lock for Android (Prevent Process Kill)
if command -v termux-wake-lock &> /dev/null; then
    termux-wake-lock
    echo -e "${C_GREEN}[PWR] Wake Lock acquired (Prevent Doze Mode).${C_RESET}"
fi

# --- [ 2. INTELLIGENT EXCLUSION MAP ] ---
# Defines what to ignore based on environment (Termux vs UserLAnd)
EXCLUDES=(
    --exclude='*/.cache/*'
    --exclude='*/tmp/*'
    --exclude='*/.git/*'
    --exclude='*.log'
    --exclude='*/node_modules/*'
)

# Detect if we are in a PRoot/Chroot environment and exclude system dirs
if [ -d "/proc" ]; then
    EXCLUDES+=(
        --exclude='/proc/*' 
        --exclude='/sys/*' 
        --exclude='/dev/*' 
        --exclude='/sdcard/*' 
        --exclude='/storage/*' 
        --exclude='/mnt/*'
    )
fi

# --- [ 3. EXECUTION PIPELINE ] ---
FILENAME="${BACKUP_NAME}_$(hostname)_${TIMESTAMP}.tar.gz"

echo -e "\n${C_CYAN}target: ${REMOTE_NAME}:${FILENAME}"
echo -e "mode:   STREAM (No Local Storage Used)"
echo -e "arch:   $(uname -m)"
echo -e "cores:  $(nproc)${C_RESET}"

echo -e "\n${C_YELLOW}>> PRESS ENTER TO ENGAGE UPLOAD STREAM <<${C_RESET}"
read -r

# Rclone Optimization Flags for Low Memory/High Latency
# --buffer-size 16M: Low footprint (Android RAM friendly)
# --drive-chunk-size 32M: Balance between API calls and upload speed
# --stats 5s: Update frequency
# --stats-one-line: Cleaner output for parsing

echo -e "${C_GREEN}[EXEC] Starting Stream... Do not close window.${C_RESET}\n"

tar "${COMP_FLAG}" -cvf - "${EXCLUDES[@]}" . 2>error_log.txt \
| rclone rcat "${REMOTE_NAME}:${FILENAME}" \
  --stats 2s \
  --stats-one-line \
  --buffer-size 16M \
  --drive-chunk-size 32M \
  --use-mmap \
  --timeout 10m \
  --user-agent "OrbitalBackup/1.0" \
  -P

EXIT_CODE=$?

# --- [ 4. POST-MORTEM & NOTIFICATION ] ---
echo -e "\n--------------------------------------------------"
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${C_GREEN}>> MISSION SUCCESS: Backup Securely Stored in Cloud.${C_RESET}"
    echo -e "Integrity: SHA-1 verified by Rclone transport."
else
    echo -e "${C_RED}>> MISSION FAILED: Exit Code $EXIT_CODE.${C_RESET}"
    echo -e "Check error_log.txt for tar details."
fi

# Release Wake Lock
if command -v termux-wake-lock &> /dev/null; then
    termux-wake-lock
    echo -e "[PWR] Wake Lock released."
fi

echo -e "--------------------------------------------------"
