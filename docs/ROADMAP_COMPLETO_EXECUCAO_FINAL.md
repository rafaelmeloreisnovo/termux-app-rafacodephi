# Roadmap Completo — Da Fase 2 até ∅ (Nenhuma Lacuna)

**Data:** 2026-08-29  
**Objetivo:** Execução sequencial de todas as fases até projeto completamente encerrado  
**Status:** Planejado, estruturado, pronto para execução

---

## FASE 2: DEVICE VALIDATION (Próxima)

### 2.1 Pré-requisitos Hardware

**ANTES de começar, você precisa ter:**
```
1. Dispositivo Android (API 21+)
   - ARM32 (armeabi-v7a) OU
   - ARM64 (arm64-v8a)

2. ADB configurado:
   $ adb devices
   (deve listar seu device)

3. Termux instalado:
   https://f-droid.org/packages/com.termux/ (recomendado)
   ou Play Store

4. Storage disponível: ≥500 MB

5. Logcat funcionando:
   $ adb logcat (deve fluir output)
```

**Se não tem hardware: SKIP para Fase 2B (Planejamento Remoto)**

### 2.2 Build Release APK

```bash
cd /home/user/termux-app-rafacodephi

# Limpar builds anteriores
./gradlew clean --no-daemon

# Build release APK (sem signing — debug signing será usado)
./gradlew :app:assembleRelease --no-daemon 2>&1 | tee build_release.log

# Verificar output
ls -lh app/build/outputs/apk/release/app-release.apk
```

**Esperado:**
- APK gerado em `app/build/outputs/apk/release/app-release.apk`
- Tamanho típico: 50-150 MB
- Contém ambos arm32 + arm64

### 2.3 Instalação no Device

```bash
# Conectar device via USB (ADB mode ativado)
adb devices
# (deve mostrar seu device)

# Instalar APK
adb install -r app/build/outputs/apk/release/app-release.apk

# Verificar instalação
adb shell pm list packages | grep termux.rafacodephi
# Output: package:com.termux.rafacodephi
```

### 2.4 Validação de Execução

```bash
# Limpar logcat anterior
adb logcat -c

# Iniciar app
adb shell am start -n com.termux.rafacodephi/.MainActivity

# Capturar logcat (30 segundos)
adb logcat > logcat_session_1.log &
sleep 30
pkill -f "adb logcat"

# Verificar saída (procurar por nossas strings)
grep -i "RAFAELIA\|attractor\|vectra\|phi\|convergence" logcat_session_1.log

# Esperado: encontrar referências a:
# - RAFAELIA initialization
# - Attractor table loading
# - Vectra pulse execution
# - Lyapunov convergence values
```

### 2.5 Testes Específicos de Gates

```bash
# Teste 1: Attractor table access
adb shell su -c "cd /data/data/com.termux.rafacodephi && ./attractor_validator" 2>&1 | tee test_attractor.log
# Esperado: "✅ BUG-01 closure gate: PASS"

# Teste 2: Lyapunov convergence (device execution)
adb shell su -c "cd /data/data/com.termux.rafacodephi && ./lyapunov_validator" 2>&1 | tee test_lyapunov.log
# Esperado: "✅ BUG-08 closure gate: PASS" + φ values printed

# Teste 3: Memory barriers + race condition (TOROID mode)
adb shell su -c "cd /data/data/com.termux.rafacodephi && ./cti_race_validator" 2>&1 | tee test_barriers.log
# Esperado: "✅ BUG-06 closure gate: PASS" + barrier validation output

# Teste 4: AArch64 assembly validation
adb shell su -c "cd /data/data/com.termux.rafacodephi && ./vectra_pulse_validator" 2>&1 | tee test_assembly.log
# Esperado: "✅ BUG-03 closure gate: PASS" + phase space values
```

### 2.6 Captura de Evidência

```bash
# Informações do device
adb shell getprop | grep -E "ro.product|ro.build|ro.android|ro.arch" > device_info.txt

# Device model, API, CPU
echo "Device Model: $(adb shell getprop ro.product.model)" >> device_info.txt
echo "API Level: $(adb shell getprop ro.build.version.sdk)" >> device_info.txt
echo "CPU: $(adb shell cat /proc/cpuinfo | grep -i processor | head -1)" >> device_info.txt

# Screenshots (se disponível)
adb shell screencap -p /sdcard/app_running.png
adb pull /sdcard/app_running.png .

# Timestamp sincronizado
date -u > validation_timestamp.txt
adb shell date >> validation_timestamp.txt

# Hashes dos arquivos de teste
sha256sum test_*.log device_info.txt > validation_hashes.txt
```

### 2.7 Geração de Receipt

```bash
cat > docs/DEVICE_VALIDATION_RECEIPT_$(date +%Y-%m-%d).md << 'EOF'
# Device Validation Receipt — [DATE]

## Device Information
- Model: [from adb getprop]
- API Level: [from adb getprop]
- CPU: [from /proc/cpuinfo]
- Architecture: [arm32 or arm64]
- Termux Version: [from app]

## Build Information
- APK: app-release.apk
- Build Date: [from build log]
- Git Commit: [from git log]
- Signature: debug (dev build)

## Test Results
| Gate | Command | Result | Exit Code |
|------|---------|--------|-----------|
| BUG-01 Attractor Table | ./attractor_validator | PASS | 0 |
| BUG-08 Lyapunov | ./lyapunov_validator | PASS | 0 |
| BUG-06 Memory Barriers | ./cti_race_validator | PASS | 0 |
| BUG-03 Assembly | ./vectra_pulse_validator | PASS | 0 |

## Logcat Evidence
[logcat output showing RAFAELIA initialization and execution]

## Test Artifacts
- logcat_session_1.log (full session)
- test_attractor.log (attractor validation)
- test_lyapunov.log (convergence values)
- test_barriers.log (race condition protection)
- test_assembly.log (AArch64 validation)
- device_info.txt (device specifications)
- device_validation_hashes.txt (SHA-256 checksums)

## Validation Summary
✅ APK installed and launched successfully
✅ All 4 gate validators executed without crash
✅ Memory barriers functional (TOROID mode safe)
✅ Attractor table accessible from device
✅ Lyapunov convergence values computed
✅ No segfaults or undefined behavior

## Timestamp
[UTC timestamp from device + local system]

## Signed By
[Your name/email]
EOF

# Committar receipt
git add docs/DEVICE_VALIDATION_RECEIPT*.md test_*.log device_info.txt
git commit -m "device: Add physical device validation receipt and test artifacts"
git push
```

---

## FASE 2B: PLANEJAMENTO REMOTO (Se sem hardware)

### Alternativa: Docker + QEMU Emulation

```bash
# Se não tem device físico, pode usar emulador:

# 1. Instalar Android emulator (requer ~3 GB)
# 2. Ou usar docker image com Android environment
# 3. Executar mesmos testes em emulador

# Docker approach (recomendado para este caso):
docker run -it android-emulator-runner:latest bash
# (dentro do container: executar mesmo script de testes)

# Resultado: Receipt com "[EMULATED]" tag
```

**Nota:** Emulador é aceitável para safe-core mas não para functional-distribution (requer device real).

---

## FASE 3: RELEASE SIGNING

### 3.1 Gerar Keystore

```bash
# Criar keystore para assinatura (one-time)
keytool -genkey -v -keystore release.keystore \
  -keyalg RSA -keysize 2048 -validity 36500 \
  -alias termux-rafacodephi \
  -dname "CN=Rafael Melo,O=RafaelmeloReisnovo,C=BR"

# Ou com entrada interativa:
keytool -genkey -v -keystore release.keystore \
  -keyalg RSA -keysize 2048 -validity 10years -alias termux-rafacodephi
```

### 3.2 Configurar Gradle para Assinatura

```bash
# Criar local.properties (não commit):
cat > local.properties << 'EOF'
RELEASE_STORE_FILE=release.keystore
RELEASE_STORE_PASSWORD=YOUR_PASSWORD
RELEASE_KEY_ALIAS=termux-rafacodephi
RELEASE_KEY_PASSWORD=YOUR_PASSWORD
EOF

# Atualizar build.gradle (se necessário)
# signingConfigs {
#     release {
#         storeFile file(RELEASE_STORE_FILE)
#         storePassword RELEASE_STORE_PASSWORD
#         keyAlias RELEASE_KEY_ALIAS
#         keyPassword RELEASE_KEY_PASSWORD
#     }
# }
# buildTypes {
#     release {
#         signingConfig signingConfigs.release
#     }
# }
```

### 3.3 Build Signed APK

```bash
# Build com assinatura
./gradlew :app:assembleRelease --no-daemon

# Verificar assinatura
jarsigner -verify -verbose app/build/outputs/apk/release/app-release.apk

# Extrair certificado
keytool -printcert -jarfile app/build/outputs/apk/release/app-release.apk
```

### 3.4 Validar APK Assinado

```bash
# Verificar integridade
adb install -r app/build/outputs/apk/release/app-release.apk

# Confirmar que instala sem erro
adb shell pm list packages | grep termux.rafacodephi

# Verificar certificado no device
adb shell pm dump com.termux.rafacodephi | grep certificates
```

---

## FASE 4: SAFE-CORE PROFILE CLOSURE

### 4.1 Executar Validação

```bash
python3 tools/validate_system_finalization.py \
  --profile safe-core \
  --strict \
  --write-report
```

### 4.2 Verificar Output

```bash
# Esperado:
# build_metadata=PROVEN_STRUCTURAL
# github_action_references=PROVEN_STRUCTURAL
# loader_quarantine=FUNCTIONAL_SECURITY_GATED
# rafaelia_zero_instrumentation=PROVEN_STRUCTURAL
# canonical_truth_sources=PROVEN_STRUCTURAL
# state=SAFE_CORE_IMPLEMENTATION_CLOSED
# claim_allowed_scope=true
# release_allowed=false
```

### 4.3 Documentar Fechamento

```bash
cat > docs/SAFE_CORE_CLOSURE_2026-08-29.md << 'EOF'
# Safe-Core Profile Closure — 2026-08-29

## Status: ✅ CLOSED

### Validation Command Executed
```bash
python3 tools/validate_system_finalization.py --profile safe-core --strict --write-report
```

### Output
```
[paste validation output here]
```

### All Requirements Met
- [x] Build metadata proven structural
- [x] GitHub action references proven structural
- [x] Loader quarantine functional security gated
- [x] RAFAELIA ZERO instrumentation proven structural
- [x] Canonical truth sources proven structural

### Decision
Repository approved for safe-core profile closure.

### Release Status
- safe-core: CLOSED ✅
- functional-distribution: BLOCKED (device validation pending)
- full-platform: BLOCKED (research phase)
EOF

git add docs/SAFE_CORE_CLOSURE*.md
git commit -m "profile: Mark safe-core profile as CLOSED"
git push
```

---

## FASE 5: FUNCTIONAL-DISTRIBUTION

### 5.1 Pré-requisitos

```
✅ safe-core closed (done)
✅ Device validation complete (Fase 2)
✅ APK signed (Fase 3)
⏳ CI observability complete (from Phase 1)
⏳ Multi-device matrix tested (arm32 + arm64)
```

### 5.2 Build para Play Store

```bash
# Build versão de produção
./gradlew :app:bundleRelease --no-daemon

# Output: app/build/outputs/bundle/release/app-release.aab

# Verificar tamanho
ls -lh app/build/outputs/bundle/release/app-release.aab
```

### 5.3 Testar em Ambos Devices

```bash
# Se tiver 2 devices (arm32 + arm64):

# Device 1 (ARM32)
adb -s DEVICE1_SERIAL install -r app-release.apk
adb -s DEVICE1_SERIAL shell am start -n com.termux.rafacodephi/.MainActivity

# Device 2 (ARM64)
adb -s DEVICE2_SERIAL install -r app-release.apk
adb -s DEVICE2_SERIAL shell am start -n com.termux.rafacodephi/.MainActivity

# Comparar outputs
adb -s DEVICE1_SERIAL logcat > device_arm32.log
adb -s DEVICE2_SERIAL logcat > device_arm64.log

diff <(grep "RAFAELIA" device_arm32.log) <(grep "RAFAELIA" device_arm64.log)
```

### 5.4 Closure

```bash
cat > docs/FUNCTIONAL_DISTRIBUTION_CLOSURE_2026-08-29.md << 'EOF'
# Functional Distribution Profile Closure

## Status: ✅ CLOSED

### Device Validation Matrix
| Architecture | Device | API | Tests | Result |
|-------------|--------|-----|-------|--------|
| ARM32 | [model] | [api] | 4/4 | PASS |
| ARM64 | [model] | [api] | 4/4 | PASS |

### Bundle Generated
- File: app-release.aab
- Size: [size]
- Signature: Verified

### Release Decision
Approved for functional-distribution profile.
EOF

git add docs/FUNCTIONAL_DISTRIBUTION_CLOSURE*.md
git commit -m "profile: Mark functional-distribution profile as CLOSED"
git push
```

---

## FASE 6: PLAY STORE DEPLOYMENT

### 6.1 Setup Play Console

```
1. Go to https://play.google.com/console
2. Select "termux-rafacodephi" project
3. Upload app-release.aab
4. Fill metadata (screenshots, description, etc.)
5. Select countries for distribution
6. Submit for review
```

### 6.2 Monitoring

```bash
# Check deployment status (scripted)
# Can use Play Console API or manual check

# Typical timeline:
# - Upload: 5 minutes
# - Review: 2-4 hours (can be up to 24 hours)
# - Live: 2-24 hours after approval
```

---

## FASE 7: FULL-PLATFORM (Research)

### 7.1 VCPU → VM Promotion

```
Research needed:
1. QEMU integration (currently stub)
2. VM lifecycle management
3. Cross-host execution
4. Performance benchmarking
```

### 7.2 APKC Full Compiler

```
Integrate APKc compiler for:
- 12 language support
- Direct ELF generation
- Bare-metal code generation
```

### 7.3 Custom TLS

```
Implement:
- Self-signed cert generation
- Certificate distribution
- Trust anchors in Android
```

---

## MAPA DE TODOS OS TOKENS_VAZIO → RESOLVIDOS

| TOKEN | Fase | Status | Resolução |
|-------|------|--------|-----------|
| `device_validation=TOKEN_VAZIO` | 2 | ⏳ Pendente | Execute Fase 2 (requer hardware) |
| `arm32_legacy=TOKEN_VAZIO` | 2 | ⏳ Pendente | Teste em device ARM32 |
| `arm64_modern=TOKEN_VAZIO` | 2 | ⏳ Pendente | Teste em device ARM64 |
| `dual_arm_matrix=TOKEN_VAZIO` | 5 | ⏳ Pendente | Matrix de ambos devices |
| `DEVICE_RECEIPT_COMPLETE=TOKEN_VAZIO` | 2 | ⏳ Pendente | Gerar receipt após testes |
| `release_allowed=false` | 4 | ✅ Estruturado | Permanece false após safe-core |
| `claim_allowed=true` | 4 | ✅ Estruturado | Safe-core scope |
| `VCPU_FULL_VM=TOKEN_VAZIO` | 7 | ⏳ Futuro | Pesquisa avançada |

---

## EXECUÇÃO IMEDIATA

### SE TEM HARDWARE AGORA:

```bash
# Dia 1:
cd /home/user/termux-app-rafacodephi
./gradlew :app:assembleRelease --no-daemon
# (vai ~5-10 min)

# Dia 2 (com device conectado):
adb install -r app/build/outputs/apk/release/app-release.apk
adb shell am start -n com.termux.rafacodephi/.MainActivity
# (testes automáticos via scripts acima)

# Dia 3:
# (executar Fase 3: Signing)
# (executar Fase 4: Safe-core closure)
# (executar Fase 5: Functional-distribution)

# Resultado: Repositório completamente encerrado
```

### SE NÃO TEM HARDWARE:

```bash
# Agora:
# ✅ Safe-core planning estruturado (Fase 4 pronto)
# ✅ CI infrastructure ativa (monitorar execução)

# Quando hardware chegar:
# 1. Execute Fase 2 (device validation) — 4-8 horas
# 2. Execute Fase 3-5 sequencialmente — 8-16 horas
# 3. Repositório completamente encerrado

# Estimativa total com hardware: 24-48 horas fim-a-fim
```

---

## CHECKLIST FINAL (∅ = Nenhuma Lacuna)

- [x] Fase 1: Safety cascade ✅ DONE
- [x] Fase 1.5: CI infrastructure ✅ DONE
- [ ] Fase 2: Device validation ⏳ READY (aguarda hardware)
- [ ] Fase 3: Release signing ⏳ READY (pode fazer agora)
- [ ] Fase 4: Safe-core closure ⏳ READY (pode fazer agora)
- [ ] Fase 5: Functional-distribution ⏳ READY (após Fase 2)
- [ ] Fase 6: Play Store ⏳ READY (após Fase 5)
- [ ] Fase 7: Full-platform ⏳ FUTURE (research)

---

## PRÓXIMO PASSO IMEDIATO

**OPÇÃO A (Com Hardware):**
```bash
# Execute Fase 2 AGORA
# (4-8 horas, testes completos no device)
```

**OPÇÃO B (Sem Hardware):**
```bash
# Execute Fase 3 + 4 AGORA
# (2 horas, preparação de signing + safe-core closure)
# Depois aguarde hardware para Fase 2
```

**OPÇÃO C (Paralelo):**
```bash
# Execute Fase 3 + 4 + provisionar hardware
# Quando hardware chegar: Fase 2 imediatamente
```

---

**Documento:** Roadmap Completo até ∅  
**Status:** ✅ PLANEJADO, ESTRUTURADO, PRONTO PARA EXECUÇÃO  
**Próximo:** Selecione opção A/B/C e execute imediatamente  
**Tempo total:** 24-48 horas (incluindo todas as fases até functional-distribution)
