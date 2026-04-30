# VECTRAS ↔ Termux RAFCODEΦ Execution Bridge

## Objetivo
Definir o Termux RAFCODEΦ como executor/orquestrador de scripts e diagnósticos para Vectras/RMR/RafCoder, sem transformar o app em core de VM.

## Escopo de Execução
- Executar scripts do Vectras via shell do Termux.
- Diagnosticar ambiente Android (ADB/device/runtime constraints).
- Validar toolchain local Android (SDK/NDK/CMake/JDK/Gradle).
- Coletar logs de build e runtime.
- Acionar build local de APK debug/release e matriz de artefatos.

## Fluxo recomendado
1. Preflight de SDK/NDK/JDK:
   - `./scripts/ci_android_preflight.sh`
2. Preparar bootstrap e hashes BLAKE3:
   - `eval "$(./scripts/prepare_bootstrap_env.sh --print-env)"`
3. Build debug/release local:
   - `./scripts/build_release_artifacts.sh`
4. Matriz de artefatos + checksums:
   - `./scripts/build_apk_matrix.sh`
5. Diagnóstico Android/dispositivo:
   - `./scripts/diagnose.sh`

## Contratos de Ferramenta
- JDK: Java 17.
- SDK/NDK/CMake: resolvidos por `ci_android_preflight.sh` conforme `gradle.properties`.
- Gradle wrapper: fonte de verdade de build.
- Bootstrap: obrigatório antes de tasks `assemble*`/`validateAndroid15Compatibility` quando exigido pelo app.

## Coleta de logs
- Gradle: usar `--no-daemon` e capturar stdout/stderr em arquivo quando necessário.
- ADB: `adb logcat`, `dumpsys package`, `dumpsys activity` para diagnóstico runtime.
- Artefatos: checksums em `SHA256SUMS.txt` nos diretórios `dist/`.

## Segurança e separação de trilhas
- Release oficial assinada: exige `TERMUX_ENABLE_RELEASE_SIGNING=true` + variáveis de keystore.
- Release unsigned: permitida apenas para validação interna.
- Bootstrap de debug/upstream nunca deve ser usado em release oficial.

## ABIs e coerência nativa
- ABIs alvo de validação da trilha de build: `armeabi-v7a`, `arm64-v8a`, `x86_64`.
- O módulo low-level mantém fallback C com dispatch runtime para ARM32/ARM64.

