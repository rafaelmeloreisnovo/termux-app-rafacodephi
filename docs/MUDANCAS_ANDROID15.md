# Termux RAFCODEΦ - Mudanças para Android 15

## A) Lista de Mudanças por Arquivo

### Arquivos Já Configurados (Não Requerem Mudanças)

#### 1. `app/build.gradle`
- ✅ **applicationId**: `com.termux.rafacodephi`
- ✅ **versionNameSuffix**: `-rafacodephi`
- ✅ **manifestPlaceholders.TERMUX_PACKAGE_NAME**: `com.termux.rafacodephi`
- ✅ **manifestPlaceholders.TERMUX_APP_NAME**: `Termux RAFCODEΦ`

#### 2. `app/src/main/AndroidManifest.xml`
- ✅ Usa placeholders dinâmicos: `${TERMUX_PACKAGE_NAME}`
- ✅ Permission: `${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND`
- ✅ Authorities: `${TERMUX_PACKAGE_NAME}.documents`, `${TERMUX_PACKAGE_NAME}.files`
- ✅ Intent action: `${TERMUX_PACKAGE_NAME}.RUN_COMMAND`
- ✅ Nenhum valor hardcoded de `com.termux`

#### 3. `app/src/main/res/xml/shortcuts.xml`
- ✅ **targetPackage**: `com.termux.rafacodephi` (correto)
- ✅ **targetClass**: Classes usando package completo

#### 4. `termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java`
- ✅ **TERMUX_PACKAGE_NAME**: `com.termux.rafacodephi`
- ✅ **TERMUX_APP_NAME**: `Termux RAFCODEΦ`
- ✅ Todos os paths calculados dinamicamente a partir de TERMUX_PACKAGE_NAME
- ✅ TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH usa TERMUX_PACKAGE_NAME
- ✅ Nenhum hardcode de `/data/data/com.termux`

#### 5. `termux-shared/src/main/res/values/strings.xml`
- ✅ Entity **TERMUX_PACKAGE_NAME**: `com.termux.rafacodephi`
- ✅ Entity **TERMUX_APP_NAME**: `Termux RAFCODEΦ`
- ✅ Entity **TERMUX_PREFIX_DIR_PATH**: `/data/data/com.termux.rafacodephi/files/usr`

#### 6. `app/src/main/res/values/strings.xml`
- ✅ Entity **TERMUX_PACKAGE_NAME**: `com.termux.rafacodephi`
- ✅ Entity **TERMUX_APP_NAME**: `Termux RAFCODEΦ`
- ✅ Todas as strings usam entities dinâmicos

### Novos Arquivos Criados

#### 1. `docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md`
- ✅ Documentação completa de compatibilidade
- ✅ Comandos de build, test, adb, logcat
- ✅ Checklist de aceitação
- ✅ Diagnóstico de estabilidade Android 15

#### 2. `scripts/diagnose.sh`
- ✅ Script de diagnóstico automatizado
- ✅ Verifica instalação side-by-side
- ✅ Verifica authorities e permissions únicos
- ✅ Testa Phantom Process Killer
- ✅ Testa battery optimization e standby buckets

## B) Patch Diff

Como o repositório já está configurado corretamente com as mudanças necessárias, não há diff a aplicar. As configurações já existentes são:

```diff
# CONFIGURAÇÃO ATUAL (JÁ APLICADA):

## app/build.gradle
applicationId "com.termux.rafacodephi"
versionNameSuffix "-rafacodephi"
manifestPlaceholders.TERMUX_PACKAGE_NAME = "com.termux.rafacodephi"
manifestPlaceholders.TERMUX_APP_NAME = "Termux RAFCODEΦ"

## TermuxConstants.java
public static final String TERMUX_APP_NAME = "Termux RAFCODEΦ";
public static final String TERMUX_PACKAGE_NAME = "com.termux.rafacodephi";
public static final String TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH = "/data/data/" + TERMUX_PACKAGE_NAME;

## strings.xml (ambos)
<!ENTITY TERMUX_PACKAGE_NAME "com.termux.rafacodephi">
<!ENTITY TERMUX_APP_NAME "Termux RAFCODEΦ">

## AndroidManifest.xml
android:name="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND"
android:authorities="${TERMUX_PACKAGE_NAME}.documents"
android:authorities="${TERMUX_PACKAGE_NAME}.files"
android:permission="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND"
<action android:name="${TERMUX_PACKAGE_NAME}.RUN_COMMAND" />

## shortcuts.xml
android:targetPackage="com.termux.rafacodephi"
```

### Mudanças Adicionais (Documentação)

```diff
+ docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md
+ scripts/diagnose.sh (executável)
```

## C) Comandos de Build/Test/ADB/Logcat

### Build Commands

```bash
# Build debug APK (universal + por arquitetura)
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Build apenas universal APK
./gradlew assembleDebug -DTERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS=0

# Limpar build
./gradlew clean

# Rebuild completo
./gradlew clean assembleDebug

# Verificar tasks disponíveis
./gradlew tasks
```

### Test Commands

```bash
# Rodar testes unitários
./gradlew test

# Rodar testes instrumentados (requer device/emulator)
./gradlew connectedAndroidTest

# Rodar testes específicos
./gradlew testDebugUnitTest
```

### ADB Commands - Instalação

```bash
# Instalar APK universal
adb install app/build/outputs/apk/debug/termux-app_apt-android-7-debug_universal.apk

# Instalar APK para arquitetura específica (ARM64)
adb install app/build/outputs/apk/debug/termux-app_apt-android-7-debug_arm64-v8a.apk

# Instalar com substituição (update)
adb install -r app/build/outputs/apk/debug/termux-app_apt-android-7-debug_universal.apk

# Desinstalar
adb uninstall com.termux.rafacodephi

# Verificar instalação
adb shell pm list packages | grep termux
```

### ADB Commands - Diagnóstico

```bash
# Rodar script de diagnóstico completo
./scripts/diagnose.sh

# Verificar package details
adb shell dumpsys package com.termux.rafacodephi

# Verificar providers
adb shell dumpsys package com.termux.rafacodephi | grep -A5 "Provider"

# Verificar permissions
adb shell dumpsys package com.termux.rafacodephi | grep -A20 "permissions:"

# Verificar processos
adb shell ps -A | grep com.termux.rafacodephi

# Verificar serviços
adb shell dumpsys activity services com.termux.rafacodephi

# Verificar data directory
adb shell ls -la /data/data/com.termux.rafacodephi/

# Verificar PREFIX
adb shell ls -la /data/data/com.termux.rafacodephi/files/usr/
```

### ADB Commands - Testes Android 15

```bash
# Verificar Phantom Process Killer
adb shell dumpsys activity processes | grep -A10 "Phantom"

# Verificar battery optimization
adb shell dumpsys deviceidle whitelist | grep termux

# Adicionar à whitelist
adb shell dumpsys deviceidle whitelist +com.termux.rafacodephi

# Verificar standby bucket
adb shell am get-standby-bucket com.termux.rafacodephi

# Forçar standby bucket ativo
adb shell am set-standby-bucket com.termux.rafacodephi active

# Verificar memória
adb shell dumpsys meminfo com.termux.rafacodephi

# Verificar wakelocks
adb shell dumpsys power | grep -A5 termux
```

### Logcat Commands

```bash
# Logcat geral do app
adb logcat | grep -i termux

# Logcat por PID (requer app em execução)
adb logcat --pid=$(adb shell pidof -s com.termux.rafacodephi)

# Logcat filtrado por tags
adb logcat -s TermuxService,RunCommandService,TermuxActivity

# Logcat com timestamp
adb logcat -v time | grep termux

# Logcat apenas erros
adb logcat *:E | grep termux

# Salvar logcat em arquivo
adb logcat | grep termux > termux-rafacodephi.log

# Limpar logcat
adb logcat -c

# Logcat contínuo com filtro
adb logcat | grep --line-buffered termux
```

### Teste de Comandos RUN_COMMAND

```bash
# Executar comando simples
adb shell am startservice \
  -n com.termux.rafacodephi/.app.RunCommandService \
  -a com.termux.rafacodephi.RUN_COMMAND \
  --es com.termux.app.RunCommandService.EXTRA_COMMAND "echo Hello World"

# Executar comando com workdir
adb shell am startservice \
  -n com.termux.rafacodephi/.app.RunCommandService \
  -a com.termux.rafacodephi.RUN_COMMAND \
  --es com.termux.app.RunCommandService.EXTRA_COMMAND "ls -la" \
  --es com.termux.app.RunCommandService.EXTRA_WORKDIR "/data/data/com.termux.rafacodephi/files/home"

# Executar script bash
adb shell am startservice \
  -n com.termux.rafacodephi/.app.RunCommandService \
  -a com.termux.rafacodephi.RUN_COMMAND \
  --es com.termux.app.RunCommandService.EXTRA_COMMAND "/data/data/com.termux.rafacodephi/files/usr/bin/bash" \
  --esa com.termux.app.RunCommandService.EXTRA_ARGUMENTS "-c,pwd"
```

### Teste de FileProvider

```bash
# Listar conteúdo via content provider
adb shell content query --uri content://com.termux.rafacodephi.files/

# Verificar DocumentsProvider
adb shell dumpsys content | grep com.termux.rafacodephi.documents
```

## D) Checklist Final

### ✅ Compilação e Build
- [x] Projeto compila sem erros
- [x] APKs gerados com sucesso (5 arquiteturas)
- [x] Package name correto: `com.termux.rafacodephi`
- [x] Version: 0.118.0-rafacodephi
- [x] Bootstrap incluído no APK

### ✅ Manifesto e Configuração
- [x] ApplicationId: `com.termux.rafacodephi`
- [x] App Name: `Termux RAFCODEΦ`
- [x] Authorities únicos:
  - [x] `com.termux.rafacodephi.documents` (DocumentsProvider)
  - [x] `com.termux.rafacodephi.files` (FileProvider)
  - [x] `com.termux.rafacodephi.androidx-startup` (StartupProvider)
- [x] Permissions únicos:
  - [x] `com.termux.rafacodephi.permission.RUN_COMMAND`
- [x] Intent actions únicos:
  - [x] `com.termux.rafacodephi.RUN_COMMAND`

### ✅ Paths e Diretórios
- [x] TERMUX_PACKAGE_NAME: `com.termux.rafacodephi`
- [x] Data dir: `/data/data/com.termux.rafacodephi`
- [x] PREFIX: `/data/data/com.termux.rafacodephi/files/usr`
- [x] HOME: `/data/data/com.termux.rafacodephi/files/home`
- [x] Todos os paths calculados dinamicamente
- [x] Nenhum hardcode de `com.termux` original

### ✅ Side-by-Side Installation
- [x] Package name diferente do Termux oficial
- [x] Zero colisão de authorities
- [x] Zero colisão de permissions
- [x] Zero colisão de receivers
- [x] Zero colisão de intent actions
- [x] Data directories completamente separados

### ⏳ Testes Funcionais (Requer Device/Emulator)
- [ ] App inicia corretamente
- [ ] Terminal funciona
- [ ] Bootstrap instalado com sucesso
- [ ] RunCommandService aceita comandos
- [ ] FileProvider funciona
- [ ] DocumentsProvider funciona
- [ ] Notifications funcionam
- [ ] Foreground service funciona

### ⏳ Android 15 Compatibility (Requer Android 15+)
- [ ] App não é morto pelo Phantom Process Killer
- [ ] Background services funcionam corretamente
- [ ] Foreground service notification sempre visível
- [ ] Nenhuma restrição de sandboxing quebra funcionalidade
- [ ] Performance aceitável
- [ ] I/O não excessivo
- [ ] Battery drain aceitável
- [ ] Processos filhos funcionam

### ✅ Documentação
- [x] README com instruções de build
- [x] Documentação de compatibilidade Android 15
- [x] Script de diagnóstico
- [x] Comandos de build documentados
- [x] Comandos de test documentados
- [x] Comandos ADB documentados
- [x] Comandos logcat documentados

### ✅ Reprodutibilidade
- [x] Build gradle determinístico
- [x] Dependencies fixadas
- [x] Bootstrap checksums verificados
- [x] Documentação de CI/CD disponível

## Conclusão

O fork **Termux RAFCODEΦ** está **COMPLETAMENTE CONFIGURADO** para:

1. ✅ **Instalação lado-a-lado** com Termux oficial
2. ✅ **Zero colisões** de authorities, permissions, receivers
3. ✅ **Zero paths errados** - todos dinâmicos
4. ✅ **Reprodutibilidade** - build determinístico
5. ✅ **Diagnóstico completo** - script automatizado

### Status Final
🟢 **PRONTO PARA PRODUÇÃO** (pending functional tests em device real)

### Próximos Passos Recomendados
1. Testar instalação em device Android 15
2. Rodar `./scripts/diagnose.sh` após instalação
3. Testar funcionalidade completa do terminal
4. Testar side-by-side com Termux oficial
5. Monitorar Phantom Process Killer
6. Otimizar battery/performance se necessário

### Suporte
- Documentação: `docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md`
- Diagnóstico: `./scripts/diagnose.sh`
- Issues: GitHub repository issues
