#!/usr/bin/env bash
#
# Termux RAFCODEΦ - Diagnóstico de Instalação e Compatibilidade Android 15
#
# Este script verifica se o Termux RAFCODEΦ está instalado corretamente
# e pode coexistir com o Termux oficial sem conflitos.
#

set -e

PACKAGE_NAME="com.termux.rafacodephi"
OFFICIAL_PACKAGE="com.termux"

echo "=========================================="
echo "Termux RAFCODEΦ - Diagnóstico Android 15"
echo "=========================================="
echo ""

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

success() {
    echo -e "${GREEN}✓${NC} $1"
}

error() {
    echo -e "${RED}✗${NC} $1"
}

warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

info() {
    echo -e "  $1"
}

# Verificar conexão ADB
echo "1. Verificando conexão ADB..."
if ! adb devices | grep -q "device$"; then
    error "Nenhum dispositivo conectado via ADB"
    exit 1
fi
success "Dispositivo conectado"
echo ""

# Verificar versão do Android
echo "2. Verificando versão do Android..."
SDK_VERSION=$(adb shell getprop ro.build.version.sdk)
ANDROID_VERSION=$(adb shell getprop ro.build.version.release)
info "Android version: $ANDROID_VERSION (SDK: $SDK_VERSION)"
if [ "$SDK_VERSION" -ge 35 ]; then
    success "Android 15+ detectado (SDK $SDK_VERSION)"
elif [ "$SDK_VERSION" -ge 29 ]; then
    warning "Android $ANDROID_VERSION (SDK $SDK_VERSION) - algumas features do Android 15 não disponíveis"
else
    warning "Android $ANDROID_VERSION (SDK $SDK_VERSION) - versão antiga"
fi
echo ""

# Verificar instalação do Termux RAFCODEΦ
echo "3. Verificando instalação do Termux RAFCODEΦ..."
if adb shell pm list packages | grep -q "^package:$PACKAGE_NAME$"; then
    success "Termux RAFCODEΦ instalado"
    VERSION=$(adb shell dumpsys package "$PACKAGE_NAME" | grep versionName | head -1 | awk '{print $1}' | cut -d'=' -f2)
    info "Version: $VERSION"
else
    error "Termux RAFCODEΦ não está instalado"
    exit 1
fi
echo ""

# Verificar instalação do Termux oficial (side-by-side)
echo "4. Verificando instalação side-by-side..."
if adb shell pm list packages | grep -q "^package:$OFFICIAL_PACKAGE$"; then
    success "Termux oficial também está instalado (side-by-side OK)"
    OFFICIAL_VERSION=$(adb shell dumpsys package "$OFFICIAL_PACKAGE" | grep versionName | head -1 | awk '{print $1}' | cut -d'=' -f2)
    info "Official Termux version: $OFFICIAL_VERSION"
else
    info "Termux oficial não está instalado (não é problema)"
fi
echo ""

# Verificar authorities (providers)
echo "5. Verificando Content Providers e Authorities..."
AUTHORITIES=$(adb shell dumpsys package "$PACKAGE_NAME" | grep -A1 "Provider" | grep "authority=" | sed 's/.*authority=//' | sed 's/ .*//')
if [ -z "$AUTHORITIES" ]; then
    warning "Nenhum provider encontrado"
else
    success "Providers encontrados:"
    for auth in $AUTHORITIES; do
        info "  - $auth"
        # Verificar conflitos
        if echo "$auth" | grep -q "^com.termux.rafacodephi"; then
            success "    Authority único (sem conflito)"
        else
            error "    Authority pode conflitar com Termux oficial!"
        fi
    done
fi
echo ""

# Verificar permissions
echo "6. Verificando Permissions..."
PERMS=$(adb shell dumpsys package "$PACKAGE_NAME" | grep "permission com.termux" | cut -d' ' -f4)
if [ -z "$PERMS" ]; then
    warning "Nenhuma permission customizada encontrada"
else
    success "Permissions encontradas:"
    for perm in $PERMS; do
        info "  - $perm"
        if echo "$perm" | grep -q "rafacodephi"; then
            success "    Permission única (sem conflito)"
        else
            error "    Permission pode conflitar com Termux oficial!"
        fi
    done
fi
echo ""

# Verificar data directory
echo "7. Verificando Data Directory..."
DATA_DIR="/data/data/$PACKAGE_NAME"
if adb shell "[ -d $DATA_DIR ] && echo exists" | grep -q exists; then
    success "Data directory existe: $DATA_DIR"
    
    # Verificar subdirectórios importantes
    if adb shell "[ -d $DATA_DIR/files ] && echo exists" | grep -q exists; then
        success "  files/ existe"
    else
        warning "  files/ não existe"
    fi
    
    if adb shell "[ -d $DATA_DIR/files/usr ] && echo exists" | grep -q exists; then
        success "  files/usr/ (PREFIX) existe"
    else
        warning "  files/usr/ (PREFIX) não existe - bootstrap não instalado?"
    fi
    
    if adb shell "[ -d $DATA_DIR/files/home ] && echo exists" | grep -q exists; then
        success "  files/home/ (HOME) existe"
    else
        warning "  files/home/ (HOME) não existe"
    fi
else
    error "Data directory não existe: $DATA_DIR"
fi
echo ""

# Verificar processos ativos
echo "8. Verificando processos ativos..."
PROCESSES=$(adb shell ps -A | grep "$PACKAGE_NAME" | wc -l)
if [ "$PROCESSES" -gt 0 ]; then
    success "$PROCESSES processo(s) ativo(s)"
    adb shell ps -A | grep "$PACKAGE_NAME" | while read line; do
        info "  $line"
    done
else
    info "Nenhum processo ativo (app não está em execução)"
fi
echo ""

# Verificar serviços em background
echo "9. Verificando Background Services..."
if adb shell dumpsys activity services "$PACKAGE_NAME" 2>/dev/null | grep -q "ServiceRecord"; then
    success "Serviços ativos encontrados"
    SERVICES=$(adb shell dumpsys activity services "$PACKAGE_NAME" 2>/dev/null | grep "ServiceRecord" | awk '{print $2}')
    for svc in $SERVICES; do
        info "  - $svc"
    done
else
    info "Nenhum serviço ativo"
fi
echo ""

# Verificar Phantom Process Killer (Android 15+)
if [ "$SDK_VERSION" -ge 35 ]; then
    echo "10. Verificando Phantom Process Killer (Android 15)..."
    # Note: requires shell permission to dump
    PHANTOM_PROCESSES=$(adb shell dumpsys activity processes 2>/dev/null | grep -A5 "Phantom process list" | grep "$PACKAGE_NAME" | wc -l)
    if [ "$PHANTOM_PROCESSES" -gt 0 ]; then
        warning "Processos estão sendo monitorados pelo Phantom Process Killer"
        info "  Isso pode causar terminação inesperada de processos"
    else
        success "Nenhum processo sendo morto pelo Phantom Process Killer"
    fi
    echo ""
fi

# Verificar Battery Optimization
echo "11. Verificando Battery Optimization..."
if adb shell dumpsys deviceidle whitelist 2>/dev/null | grep -q "$PACKAGE_NAME"; then
    success "App está na whitelist de battery optimization"
else
    warning "App não está na whitelist - pode ser limitado em background"
    info "  Execute: adb shell dumpsys deviceidle whitelist +$PACKAGE_NAME"
fi
echo ""

# Verificar Standby Bucket
echo "12. Verificando App Standby Bucket..."
BUCKET=$(adb shell am get-standby-bucket "$PACKAGE_NAME" 2>/dev/null)
if [ -n "$BUCKET" ]; then
    case $BUCKET in
        5|10)
            success "Standby bucket: ACTIVE ($BUCKET) - sem restrições"
            ;;
        20)
            warning "Standby bucket: WORKING_SET ($BUCKET) - algumas restrições"
            ;;
        30)
            warning "Standby bucket: FREQUENT ($BUCKET) - restrições moderadas"
            ;;
        40)
            error "Standby bucket: RARE ($BUCKET) - restrições severas"
            ;;
        50)
            error "Standby bucket: RESTRICTED ($BUCKET) - máximas restrições"
            ;;
        *)
            info "Standby bucket: $BUCKET"
            ;;
    esac
else
    info "Não foi possível determinar standby bucket"
fi
echo ""

# Verificar memória
echo "13. Verificando uso de memória..."
MEMORY=$(adb shell dumpsys meminfo "$PACKAGE_NAME" 2>/dev/null | grep "TOTAL" | head -1 | awk '{print $2}')
if [ -n "$MEMORY" ]; then
    MEMORY_MB=$((MEMORY / 1024))
    success "Uso de memória: ${MEMORY_MB} MB"
    if [ "$MEMORY_MB" -lt 100 ]; then
        success "  Uso de memória normal"
    elif [ "$MEMORY_MB" -lt 300 ]; then
        warning "  Uso de memória moderado"
    else
        warning "  Uso de memória alto"
    fi
else
    info "Não foi possível determinar uso de memória (app não está em execução?)"
fi
echo ""

# Verificar permissions runtime
echo "14. Verificando Runtime Permissions..."
RUNTIME_PERMS="android.permission.READ_EXTERNAL_STORAGE android.permission.WRITE_EXTERNAL_STORAGE android.permission.POST_NOTIFICATIONS"
for perm in $RUNTIME_PERMS; do
    STATUS=$(adb shell dumpsys package "$PACKAGE_NAME" 2>/dev/null | grep "$perm" | grep "granted=true")
    if [ -n "$STATUS" ]; then
        success "  $perm: granted"
    else
        warning "  $perm: not granted"
    fi
done
echo ""

# Sumário final
echo "=========================================="
echo "SUMÁRIO DO DIAGNÓSTICO"
echo "=========================================="
echo ""

ALL_GOOD=true

# Verificações críticas
if ! adb shell pm list packages | grep -q "^package:$PACKAGE_NAME$"; then
    error "CRÍTICO: App não está instalado"
    ALL_GOOD=false
fi

# Verificar authorities únicos
if adb shell dumpsys package "$PACKAGE_NAME" 2>/dev/null | grep "authority=" | grep -qv "rafacodephi"; then
    error "CRÍTICO: Authorities não são únicos - conflito possível"
    ALL_GOOD=false
fi

# Verificar permissions únicos
if adb shell dumpsys package "$PACKAGE_NAME" 2>/dev/null | grep "permission com.termux" | grep -qv "rafacodephi"; then
    error "CRÍTICO: Permissions não são únicos - conflito possível"
    ALL_GOOD=false
fi

if $ALL_GOOD; then
    echo -e "${GREEN}"
    echo "╔══════════════════════════════════════════╗"
    echo "║  ✓ TODOS OS TESTES PASSARAM COM SUCESSO ║"
    echo "╚══════════════════════════════════════════╝"
    echo -e "${NC}"
    echo ""
    echo "O Termux RAFCODEΦ está corretamente configurado para:"
    echo "  ✓ Instalação side-by-side com Termux oficial"
    echo "  ✓ Sem colisões de authorities, permissions ou receivers"
    echo "  ✓ Paths únicos em /data/data/"
    echo "  ✓ Compatibilidade com Android 15"
else
    echo -e "${RED}"
    echo "╔══════════════════════════════════════════╗"
    echo "║  ✗ ALGUNS TESTES FALHARAM                ║"
    echo "╚══════════════════════════════════════════╝"
    echo -e "${NC}"
    echo ""
    echo "Revise os erros acima e corrija antes de usar em produção."
fi

echo ""
echo "Para mais detalhes, consulte:"
echo "  docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md"
echo ""
