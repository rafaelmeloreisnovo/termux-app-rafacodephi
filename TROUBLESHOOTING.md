# Termux RAFCODEΦ - Troubleshooting Guide Android 15

## 🔧 Problemas Comuns e Soluções

### 1. App é morto inesperadamente (Phantom Process Killer)

**Sintoma:** Processos terminam sem aviso, especialmente em background.

**Diagnóstico:**
```bash
adb shell dumpsys activity processes | grep -A10 "Phantom"
adb logcat | grep "PhantomProcessKiller"
```

**Solução:**
```bash
# Adicionar à whitelist de battery
adb shell dumpsys deviceidle whitelist +com.termux.rafacodephi

# Desabilitar battery optimization (via Settings)
# Settings > Apps > Termux RAFCODEΦ > Battery > Unrestricted

# Forçar standby bucket ativo
adb shell am set-standby-bucket com.termux.rafacodephi active
```

---

### 2. App não inicia em background (Android 10+)

**Sintoma:** `Display over other apps permission not granted` error.

**Diagnóstico:**
```bash
adb logcat | grep "SYSTEM_ALERT_WINDOW"
```

**Solução:**
```bash
# Conceder permissão via ADB
adb shell cmd appops set com.termux.rafacodephi SYSTEM_ALERT_WINDOW allow

# Ou via Settings
# Settings > Apps > Termux RAFCODEΦ > Advanced > Display over other apps
```

---

### 3. Colisão com Termux oficial

**Sintoma:** Erro ao instalar ou providers não funcionam.

**Diagnóstico:**
```bash
# Verificar packages instalados
adb shell pm list packages | grep termux

# Verificar authorities
adb shell dumpsys package com.termux.rafacodephi | grep "authority="
adb shell dumpsys package com.termux | grep "authority="
```

**Verificação:**
- Authorities devem ser diferentes:
  - RAFCODEΦ: `com.termux.rafacodephi.*`
  - Oficial: `com.termux.*`

**Solução:** Se houver colisão, o build está incorreto. Re-build:
```bash
./gradlew clean assembleDebug
```

---

### 4. Bootstrap não instala

**Sintoma:** Erro ao iniciar pela primeira vez, PREFIX não existe.

**Diagnóstico:**
```bash
adb shell ls -la /data/data/com.termux.rafacodephi/files/usr/
adb logcat | grep "bootstrap"
```

**Causas Possíveis:**
- SELinux bloqueando
- Instalado em SD card externo
- Espaço insuficiente
- Não é usuário primário

**Solução:**
```bash
# Verificar SELinux
adb shell getenforce
# Se "Enforcing", pode ser a causa

# Verificar espaço
adb shell df -h | grep data

# Verificar usuário
adb shell dumpsys user | grep "UserInfo"
# Deve ser user 0 (primário)

# Reinstalar
adb uninstall com.termux.rafacodephi
adb install app/build/outputs/apk/debug/termux-app_apt-android-7-debug_universal.apk
```

---

### 5. FileProvider/DocumentsProvider não funciona

**Sintoma:** Não consegue compartilhar arquivos ou acessar via SAF.

**Diagnóstico:**
```bash
adb shell dumpsys package com.termux.rafacodephi | grep -A10 "Provider"
adb shell content query --uri content://com.termux.rafacodephi.files/
```

**Solução:**
```bash
# Verificar permission
adb shell dumpsys package com.termux.rafacodephi | grep "grantUriPermissions"

# Deve mostrar "grantUriPermissions=true"

# Se não funcionar, reinstalar
adb uninstall com.termux.rafacodephi
adb install -r app/build/outputs/apk/debug/termux-app_apt-android-7-debug_universal.apk
```

---

### 6. RunCommandService não aceita comandos

**Sintoma:** Intent não é processado, comando não executa.

**Diagnóstico:**
```bash
adb logcat | grep "RunCommandService"

# Testar intent
adb shell am startservice \
  -n com.termux.rafacodephi/.app.RunCommandService \
  -a com.termux.rafacodephi.RUN_COMMAND \
  --es com.termux.app.RunCommandService.EXTRA_COMMAND "echo test"
```

**Causas Possíveis:**
- Permission não concedida
- Action incorreto
- Package name incorreto

**Solução:**
```bash
# Verificar permission
adb shell dumpsys package com.termux.rafacodephi | grep "RUN_COMMAND"

# Deve mostrar:
# com.termux.rafacodephi.permission.RUN_COMMAND

# Verificar serviço
adb shell dumpsys activity services com.termux.rafacodephi | grep "RunCommand"
```

---

### 7. High battery drain

**Sintoma:** App consome muita bateria.

**Diagnóstico:**
```bash
# Verificar wakelocks
adb shell dumpsys power | grep -A5 termux

# Verificar processos
adb shell ps -A | grep com.termux.rafacodephi

# Verificar CPU usage
adb shell top -n 1 | grep termux
```

**Solução:**
```bash
# Verificar processos desnecessários
adb shell ps -A | grep com.termux.rafacodephi

# Kill processos órfãos se necessário
adb shell pkill -f com.termux.rafacodephi

# Otimizar:
# - Desabilitar terminal view key logging
# - Reduzir polling intervals
# - Usar foreground service apenas quando necessário
```

---

### 8. Permissions não funcionam (Android 13+)

**Sintoma:** POST_NOTIFICATIONS ou storage permissions negadas.

**Diagnóstico:**
```bash
adb shell dumpsys package com.termux.rafacodephi | grep "android.permission"
```

**Solução:**
```bash
# Conceder via ADB (desenvolvimento)
adb shell pm grant com.termux.rafacodephi android.permission.POST_NOTIFICATIONS
adb shell pm grant com.termux.rafacodephi android.permission.READ_EXTERNAL_STORAGE
adb shell pm grant com.termux.rafacodephi android.permission.WRITE_EXTERNAL_STORAGE

# Em produção: usuário deve conceder via Settings
# Settings > Apps > Termux RAFCODEΦ > Permissions
```

---

### 9. Build falha

**Sintoma:** Gradle build error.

**Causas Comuns:**
- NDK não instalado
- SDK version incorreta
- Dependencies não baixadas

**Solução:**
```bash
# Limpar build
./gradlew clean

# Verificar SDK/NDK
echo $ANDROID_HOME
ls $ANDROID_HOME/ndk/

# Instalar dependencies
./gradlew dependencies

# Re-build
./gradlew assembleDebug --stacktrace
```

---

### 10. App não aparece no launcher

**Sintoma:** App instalado mas não visível.

**Diagnóstico:**
```bash
# Verificar componentes
adb shell dumpsys package com.termux.rafacodephi | grep -A5 "Activity"

# Verificar launcher activity
adb shell cmd package query-activities \
  --components \
  -a android.intent.action.MAIN \
  -c android.intent.category.LAUNCHER
```

**Solução:**
```bash
# Verificar AndroidManifest tem:
# <action android:name="android.intent.action.MAIN" />
# <category android:name="android.intent.category.LAUNCHER" />

# Forçar re-scan launcher
adb shell am broadcast -a android.intent.action.PACKAGE_ADDED \
  -d package:com.termux.rafacodephi

# Ou reiniciar launcher
adb shell killall -9 launcher
```

---

## 📊 Logs Úteis

### Coletar Logs Completos
```bash
# Limpar logcat
adb logcat -c

# Reproduzir problema

# Salvar logs
adb logcat -d > termux-rafacodephi-$(date +%Y%m%d-%H%M%S).log

# Filtrar apenas Termux
cat termux-rafacodephi-*.log | grep -i termux > termux-only.log
```

### Logs Específicos
```bash
# TermuxService
adb logcat -s TermuxService:V

# RunCommandService  
adb logcat -s RunCommandService:V

# Bootstrap
adb logcat -s TermuxInstaller:V TermuxBootstrap:V

# Crash
adb logcat -s AndroidRuntime:E
```

---

## 🆘 Reportar Issues

Ao reportar um problema, inclua:

1. **Informações do Device:**
   ```bash
   adb shell getprop ro.build.version.release  # Android version
   adb shell getprop ro.build.version.sdk      # SDK version
   adb shell getprop ro.product.model          # Device model
   ```

2. **Informações do App:**
   ```bash
   adb shell dumpsys package com.termux.rafacodephi | grep versionName
   ```

3. **Resultado do Diagnóstico:**
   ```bash
   ./scripts/diagnose.sh > diagnostico.txt 2>&1
   ```

4. **Logs Relevantes:**
   ```bash
   adb logcat -d | grep -i termux > logs.txt
   ```

5. **Passos para Reproduzir:** Descrever claramente o problema.

---

## 📚 Ver Também

- [ANDROID15_AUDIT_REPORT.md](./ANDROID15_AUDIT_REPORT.md) - Relatório completo
- [docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md](./docs/RAFCODEPHI_ANDROID15_COMPATIBILITY.md) - Documentação técnica
- [docs/MUDANCAS_ANDROID15.md](./docs/MUDANCAS_ANDROID15.md) - Lista de mudanças
- [scripts/diagnose.sh](./scripts/diagnose.sh) - Script de diagnóstico
