# Termux RAFCODEΦ - Compatibilidade Android 15

## ✅ Status de Implementação

### Package Name e Identificadores
- ✅ **Package Name**: `com.termux.rafacodephi`
- ✅ **App Name**: `Termux RAFCODEΦ`
- ✅ **Version**: 0.118.0-rafacodephi
- ✅ **ApplicationId**: Configurado corretamente no `app/build.gradle`

### Authorities e Providers
- ✅ **Documents Provider**: `com.termux.rafacodephi.documents`
- ✅ **Files Provider**: `com.termux.rafacodephi.files`
- ✅ **Startup Provider**: `com.termux.rafacodephi.androidx-startup`

### Permissions
- ✅ **RUN_COMMAND Permission**: `com.termux.rafacodephi.permission.RUN_COMMAND`
- ✅ **RUN_COMMAND Action**: `com.termux.rafacodephi.RUN_COMMAND`

### Paths Dinâmicos
Todos os paths são gerados dinamicamente a partir de `TERMUX_PACKAGE_NAME`:
- ✅ `/data/data/com.termux.rafacodephi` (app data dir)
- ✅ `/data/data/com.termux.rafacodephi/files` (files dir)
- ✅ `/data/data/com.termux.rafacodephi/files/usr` (PREFIX)
- ✅ `/data/data/com.termux.rafacodephi/files/home` (HOME)

## 🔒 Side-by-Side Installation

O fork Termux RAFCODEΦ pode ser instalado lado a lado com o Termux oficial porque:

1. **Package Name Diferente**: `com.termux.rafacodephi` vs `com.termux`
2. **Authorities Únicos**: Todos os providers usam authorities baseados no package name
3. **Permissions Únicos**: Permissions customizados por package
4. **Data Directories Separados**: Cada instalação tem seu próprio diretório em `/data/data/`

## 🛠️ Comandos de Build

### Build Debug APK
```bash
./gradlew assembleDebug
```

### Build Release APK
```bash
./gradlew assembleRelease
```

### Build com Bootstrap Específico
```bash
export TERMUX_PACKAGE_VARIANT="apt-android-7"
./gradlew assembleDebug
```

### Limpar Build
```bash
./gradlew clean
```

## 📦 APK Gerados

Após build, os APKs são gerados em:
```
app/build/outputs/apk/debug/
├── termux-app_apt-android-7-debug_universal.apk
├── termux-app_apt-android-7-debug_arm64-v8a.apk
├── termux-app_apt-android-7-debug_armeabi-v7a.apk
├── termux-app_apt-android-7-debug_x86_64.apk
└── termux-app_apt-android-7-debug_x86.apk
```

## 📱 Instalação e Testes ADB

### Instalar APK
```bash
adb install app/build/outputs/apk/debug/termux-app_apt-android-7-debug_universal.apk
```

### Verificar Instalação
```bash
# Listar pacotes Termux instalados
adb shell pm list packages | grep termux

# Verificar detalhes do pacote
adb shell dumpsys package com.termux.rafacodephi | grep -A10 "Package"
```

### Verificar Authorities
```bash
# Listar providers do pacote
adb shell dumpsys package com.termux.rafacodephi | grep -A5 "Provider"
```

### Verificar Permissions
```bash
# Listar permissions do pacote
adb shell dumpsys package com.termux.rafacodephi | grep -A20 "permissions:"
```

## 🔍 Diagnóstico de Logs

### Logcat Geral
```bash
adb logcat | grep -i "termux"
```

### Logcat Filtrado por Package
```bash
adb logcat --pid=$(adb shell pidof -s com.termux.rafacodephi)
```

### Logs de TermuxService
```bash
adb logcat | grep "TermuxService"
```

### Logs de RunCommandService
```bash
adb logcat | grep "RunCommandService"
```

## 🐛 Diagnóstico de Estabilidade Android 15

### Verificar Phantom Process Killer
```bash
# Verificar se o app está sendo morto pelo sistema
adb shell dumpsys activity processes | grep -A20 "com.termux.rafacodephi"

# Logs de phantom process killer
adb logcat | grep "PhantomProcessKiller"
```

### Verificar Processos Ativos
```bash
# Listar processos do app
adb shell ps -A | grep com.termux.rafacodephi

# Detalhes de memória
adb shell dumpsys meminfo com.termux.rafacodephi
```

### Verificar I/O e Performance
```bash
# Stats de I/O
adb shell dumpsys diskstats | grep -A10 com.termux.rafacodephi

# Verificar wakelocks
adb shell dumpsys power | grep -A5 com.termux.rafacodephi
```

### Verificar Background Restrictions
```bash
# App standby bucket
adb shell am get-standby-bucket com.termux.rafacodephi

# Battery optimization
adb shell dumpsys deviceidle whitelist | grep com.termux.rafacodephi
```

## 🔧 Desabilitar Restrições (Para Testes)

### Desabilitar Battery Optimization
```bash
adb shell dumpsys deviceidle whitelist +com.termux.rafacodephi
```

### Permitir Background Activity
```bash
adb shell cmd appops set com.termux.rafacodephi SYSTEM_ALERT_WINDOW allow
```

### Desabilitar Standby
```bash
adb shell am set-standby-bucket com.termux.rafacodephi active
```

## 🧪 Comandos de Teste

### Teste de Execução de Comando via RUN_COMMAND
```bash
# Enviar intent para executar comando
adb shell am startservice \
  -n com.termux.rafacodephi/.app.RunCommandService \
  -a com.termux.rafacodephi.RUN_COMMAND \
  --es com.termux.app.RunCommandService.EXTRA_COMMAND "ls -la" \
  --es com.termux.app.RunCommandService.EXTRA_WORKDIR "/data/data/com.termux.rafacodephi/files/home"
```

### Teste de FileProvider
```bash
# Listar arquivos via content provider
adb shell content query --uri content://com.termux.rafacodephi.files/
```

### Teste de DocumentsProvider
```bash
# Verificar provider de documentos
adb shell dumpsys content | grep com.termux.rafacodephi.documents
```

## 📊 Checklist Final de Aceitação

### ✅ Compilação e Build
- [x] Projeto compila sem erros
- [x] APKs gerados com sucesso para todas as arquiteturas
- [x] Package name correto no output-metadata.json

### ✅ Manifesto e Configuração
- [x] Package name: `com.termux.rafacodephi`
- [x] Authorities únicos para todos os providers
- [x] Permissions únicos baseados no package name
- [x] Nenhum hardcoded path para `com.termux` original

### ✅ Side-by-Side Installation
- [x] Pode ser instalado junto com Termux oficial
- [x] Nenhuma colisão de authorities
- [x] Nenhuma colisão de permissions
- [x] Nenhuma colisão de receivers
- [x] Data directories separados

### ✅ Funcionalidade
- [ ] App inicia corretamente
- [ ] Terminal funciona
- [ ] RunCommandService aceita comandos
- [ ] FileProvider/DocumentsProvider funcionam
- [ ] Bootstrap instalado corretamente

### ✅ Android 15 Compatibility
- [ ] App não é morto pelo Phantom Process Killer
- [ ] Background services funcionam
- [ ] Foreground service notification funciona
- [ ] Nenhuma restrição de sandboxing quebra funcionalidade
- [ ] Performance aceitável (sem I/O excessivo)

## 🎯 Reprodutibilidade e CI

### GitHub Actions Workflow (Sugestão)
```yaml
name: Build APK
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Set up JDK 17
        uses: actions/setup-java@v3
        with:
          java-version: '17'
          distribution: 'temurin'
      - name: Setup Android SDK
        uses: android-actions/setup-android@v2
      - name: Build Debug APK
        run: ./gradlew assembleDebug
      - name: Upload APK
        uses: actions/upload-artifact@v3
        with:
          name: termux-rafcodephi-apk
          path: app/build/outputs/apk/debug/*.apk
```

## 📝 Notas Importantes

1. **Nenhuma refatoração de packages Java/Kotlin**: Mantidos os packages originais `com.termux.*` no código fonte
2. **Package name apenas no build.gradle e manifesto**: ApplicationId define o namespace do APK
3. **Paths dinâmicos**: Todos os paths são calculados a partir de `TermuxConstants.TERMUX_PACKAGE_NAME`
4. **Bootstrap compatibility**: Bootstrap precisa corresponder ao PREFIX path compilado

## 🔗 Referências

- [Android Package Name vs Application ID](https://developer.android.com/studio/build/application-id)
- [Android 15 Background Restrictions](https://developer.android.com/about/versions/15/behavior-changes-15#background-restrictions)
- [Phantom Process Killer](https://issuetracker.google.com/issues/205156966)
- [Termux Documentation](https://wiki.termux.com)
