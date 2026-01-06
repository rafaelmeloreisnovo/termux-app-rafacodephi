# 🎯 Termux RAFCODEΦ - Relatório de Compatibilidade Android 15

## 📋 Resumo Executivo

**Status:** ✅ **PRONTO PARA ANDROID 15 (API 35)**

Este fork do Termux foi atualizado com sucesso para o Android 15 (API 35) com suporte completo para:
- ✅ Acesso moderno ao armazenamento (sem APIs legadas)
- ✅ Tipos de serviço em primeiro plano adequados
- ✅ Instalação lado a lado com Termux original
- ✅ Permissões privilegiadas removidas para conformidade com Play Store
- ✅ Flags de mutabilidade PendingIntent para API 31+

---

## 🔄 Mudanças Implementadas

### A) Atualização de Versões SDK

**Arquivo:** `gradle.properties`

```diff
- targetSdkVersion=28
+ targetSdkVersion=35
```

**Impacto:** O app agora tem como alvo Android 15, garantindo compatibilidade com APIs modernas do Android e requisitos da Play Store.

---

### B) Modernização do Acesso ao Armazenamento

**Arquivo:** `app/src/main/AndroidManifest.xml`

#### Mudanças Feitas:

1. **Flag de Armazenamento Legado Removida:**
```diff
- android:requestLegacyExternalStorage="true"
```

2. **Permissões de Armazenamento Escopo:**
```diff
- <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
- <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
+ <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" android:maxSdkVersion="32" />
+ <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" android:maxSdkVersion="32" />
```

3. **Acesso Opcional a Todos os Arquivos:**
```xml
<!-- MANAGE_EXTERNAL_STORAGE é opcional para Android 11+ -->
<uses-permission android:name="android.permission.MANAGE_EXTERNAL_STORAGE" tools:ignore="ScopedStorage" />
```

**Por quê:** Android 11+ impõe armazenamento com escopo. READ/WRITE_EXTERNAL_STORAGE não são mais usados no Android 13+. O app pode solicitar MANAGE_EXTERNAL_STORAGE para acesso completo ao sistema de arquivos, mas é opcional.

**Como Testar:**
- No Android 15, o app usará armazenamento com escopo por padrão
- Usuários podem conceder "Acesso a todos os arquivos" via Configurações → Apps → Termux RAFCODEΦ → Permissões
- Verificar com: `Environment.isExternalStorageManager()` no código

---

### C) Permissões Privilegiadas Removidas

**Arquivo:** `app/src/main/AndroidManifest.xml`

#### Removidas (Comentadas):
```xml
<!-- Permissões privilegiadas - não necessárias para funcionalidade básica -->
<!-- <uses-permission android:name="android.permission.READ_LOGS" /> -->
<!-- <uses-permission android:name="android.permission.DUMP" /> -->
<!-- <uses-permission android:name="android.permission.WRITE_SECURE_SETTINGS" /> -->
<!-- <uses-permission android:name="android.permission.PACKAGE_USAGE_STATS" tools:ignore="ProtectedPermissions" /> -->
```

**Por quê:** Estas permissões:
- Não são concedíveis em dispositivos não-root
- Não são necessárias para funcionalidade principal do Termux
- Podem causar rejeição na Play Store

**Impacto:** App agora pode ser distribuído via Play Store sem exigir privilégios especiais.

---

### D) Tipos de Serviço em Primeiro Plano

**Arquivo:** `app/src/main/AndroidManifest.xml`

```diff
<service
    android:name=".app.TermuxService"
-   android:exported="false" />
+   android:exported="false"
+   android:foregroundServiceType="dataSync" />

<service
    android:name=".app.RunCommandService"
    android:exported="true"
+   android:foregroundServiceType="dataSync"
    android:permission="${TERMUX_PACKAGE_NAME}.permission.RUN_COMMAND">
```

**Permissão Adicionada:**
```xml
<uses-permission android:name="android.permission.FOREGROUND_SERVICE_DATA_SYNC" />
```

**Por quê:** Android 14+ requer tipos explícitos de serviço em primeiro plano. `dataSync` é apropriado para sessões de terminal e execução de comandos.

---

### E) Flags de Mutabilidade PendingIntent

**Arquivo:** `app/src/main/java/com/termux/app/TermuxService.java`

```diff
- PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
+ int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0;
+ PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, flags);
```

**Por quê:** Android 12+ requer flags de mutabilidade PendingIntent. Usamos `FLAG_IMMUTABLE` por segurança.

---

## 🛠️ Validação e Testes

### Tarefas de Validação Automatizadas

Adicionamos tarefas Gradle para garantir conformidade com Android 15:

#### 1. Validação de Nome de Pacote
```bash
./gradlew :app:validatePackageNames
```
✅ Garante que não há strings "com.termux" fixas (exceto declarações de pacote e referências de classe)

#### 2. Validação de Flags de Armazenamento
```bash
./gradlew :app:validateStorageFlags
```
✅ Garante que requestLegacyExternalStorage não é usado ao ter como alvo SDK >= 30

#### 3. Validação de Authorities
```bash
./gradlew :app:validateAuthorities
```
✅ Garante que todas as authorities de content provider usam placeholder `${TERMUX_PACKAGE_NAME}`

#### 4. Validação Mestra
```bash
./gradlew :app:validateAndroid15Compatibility
```
✅ Executa todas as tarefas de validação juntas

### Teste de Build
```bash
./gradlew :app:assembleDebug
```
✅ **BUILD SUCCESSFUL** - APK compila sem erros

---

## 📦 Instalação Lado a Lado

### Configuração do Pacote

**ID de Aplicação:** `com.termux.rafacodephi` (definido em `app/build.gradle`)

**Constante de Nome de Pacote:** `TERMUX_PACKAGE_NAME = "com.termux.rafacodephi"` (em `TermuxConstants.java`)

### Diferenças do Termux Original:

| Componente | Termux Original | Termux RAFCODEΦ |
|-----------|----------------|-----------------|
| ID de Aplicação | `com.termux` | `com.termux.rafacodephi` |
| Nome do App | Termux | Termux RAFCODEΦ |
| Authority de Arquivos | `com.termux.files` | `com.termux.rafacodephi.files` |
| Authority de Documentos | `com.termux.documents` | `com.termux.rafacodephi.documents` |
| Permissão RUN_COMMAND | `com.termux.permission.RUN_COMMAND` | `com.termux.rafacodephi.permission.RUN_COMMAND` |
| Ação RUN_COMMAND | `com.termux.RUN_COMMAND` | `com.termux.rafacodephi.RUN_COMMAND` |
| Diretório de Dados | `/data/data/com.termux/` | `/data/data/com.termux.rafacodephi/` |

✅ **Sem conflitos** - Ambos os apps podem ser instalados simultaneamente sem colisões de permissão ou authority.

---

## 🧪 Testando no Android 15

### Teste de Instalação
```bash
# Compilar APK
./gradlew :app:assembleDebug

# Instalar
adb install app/build/outputs/apk/debug/termux-app_*.apk
```

### Testes de Runtime

#### 1. Verificar Acesso ao Armazenamento
```bash
# No terminal Termux RAFCODEΦ
ls $HOME
ls /sdcard  # Pode requerer MANAGE_EXTERNAL_STORAGE
```

#### 2. Verificar Status do Serviço
```bash
# Serviço deve mostrar tipo "dataSync"
adb shell dumpsys activity services com.termux.rafacodephi
```

#### 3. Verificar Permissões
```bash
# Ver permissões concedidas
adb shell dumpsys package com.termux.rafacodephi | grep permission
```

#### 4. Verificar Lado a Lado
```bash
# Ambos devem estar instalados
adb shell pm list packages | grep termux
# Saída deve mostrar:
# package:com.termux
# package:com.termux.rafacodephi
```

---

## 🔐 Considerações de Segurança

### Abordagem de Permissões

1. **Permissões Mínimas:** Apenas permissões essenciais são solicitadas por padrão
2. **Acesso Completo Opcional:** MANAGE_EXTERNAL_STORAGE está disponível mas opcional
3. **Sem Acesso Privilegiado:** Removidas READ_LOGS, DUMP, WRITE_SECURE_SETTINGS, PACKAGE_USAGE_STATS
4. **Controle do Usuário:** SYSTEM_ALERT_WINDOW e REQUEST_IGNORE_BATTERY_OPTIMIZATIONS permanecem disponíveis mas devem ser solicitadas explicitamente

### Segurança de Armazenamento

- Armazenamento com escopo é imposto no Android 13+
- Armazenamento específico do app é sempre acessível
- Armazenamento compartilhado requer permissão explícita
- SAF (Storage Access Framework) pode ser usado para acesso a pastas

---

## 📋 Notas de Migração

### Para Usuários Atualizando de Versões Anteriores

1. **Acesso ao Armazenamento:** Se você estava usando armazenamento compartilhado, pode precisar conceder permissão "Acesso a todos os arquivos"
2. **Permissões:** Algumas permissões foram removidas - elas não eram funcionais em dispositivos não-root
3. **Serviços:** Notificações em primeiro plano agora mostram tipo de serviço adequado
4. **Lado a Lado:** Você pode manter o Termux original instalado junto com este fork

### Para Desenvolvedores

1. **Referências de Pacote:** Sempre use `TermuxConstants.TERMUX_PACKAGE_NAME` ao invés de strings fixas
2. **Authorities:** Use placeholders `${TERMUX_PACKAGE_NAME}` em AndroidManifest.xml
3. **PendingIntents:** Sempre defina flags de mutabilidade para API 23+
4. **Validação:** Execute `./gradlew :app:validateAndroid15Compatibility` antes de commitar

---

## 🐛 Limitações Conhecidas

1. **Implementação SAF:** Fluxo de UI ACTION_OPEN_DOCUMENT_TREE ainda não implementado (concessão manual de permissão necessária)
2. **Otimização de Bateria:** Permissão REQUEST_IGNORE_BATTERY_OPTIMIZATIONS está declarada mas não solicitada programaticamente
3. **Overlay de Janela:** Permissão SYSTEM_ALERT_WINDOW está declarada mas não solicitada programaticamente

Estas limitações não afetam a funcionalidade principal e podem ser abordadas em atualizações futuras.

---

## ✅ Checklist de Conformidade

- [x] Tem como alvo Android 15 (API 35)
- [x] Sem APIs de armazenamento legadas
- [x] Tipos de serviço em primeiro plano declarados
- [x] Flags PendingIntent definidas
- [x] Sem permissões privilegiadas
- [x] ID de aplicação único
- [x] Authorities únicos
- [x] Permissões únicas
- [x] Build bem-sucedido
- [x] Testes de validação aprovados

---

## 📚 Referências

- [Mudanças de Comportamento Android 15](https://developer.android.com/about/versions/15/behavior-changes-15)
- [Armazenamento com Escopo](https://developer.android.com/training/data-storage#scoped-storage)
- [Serviços em Primeiro Plano](https://developer.android.com/develop/background-work/services/foreground-services)
- [Segurança PendingIntent](https://developer.android.com/privacy-and-security/risks/pending-intent)

---

## 📝 Resumo do Changelog

**Versão: 0.118.0-rafacodephi**

### Adicionado
- Suporte para Android 15 (API 35)
- Tarefas de validação Gradle para conformidade Android 15
- Tipos de serviço em primeiro plano (dataSync)
- Flags de mutabilidade PendingIntent

### Alterado
- Atualizado targetSdkVersion de 28 para 35
- READ/WRITE_EXTERNAL_STORAGE com escopo para maxSdkVersion 32
- Removida flag requestLegacyExternalStorage

### Removido
- Permissões privilegiadas (READ_LOGS, DUMP, WRITE_SECURE_SETTINGS, PACKAGE_USAGE_STATS)

### Corrigido
- Criação de PendingIntent para API 31+
- Permissões de armazenamento para Android 13+

---

**Última Atualização:** 2026-01-06
**Build Testado:** ✅ Bem-sucedido
**Validação:** ✅ Todos os testes aprovados
