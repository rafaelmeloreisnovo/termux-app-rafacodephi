# BETA_BUILD_REPORT

## Ambiente detectado
- Data auditoria: 2026-05-05 (UTC)
- SO runner: Linux container
- JDK: via Gradle wrapper (não bloqueante)
- Bloqueio principal: Android SDK não configurado (`ANDROID_HOME`/`local.properties`).

## Versões (fonte build scripts)
- Android Gradle Plugin: `8.13.2`.
- compileSdkVersion: `35`.
- targetSdkVersion: `34`.
- ndkVersion esperado: `26.3.11579264`.

## Comandos e resultados
- `./gradlew tasks` → **FALHA**: SDK location not found.
- `./gradlew clean` → **FALHA**: SDK location not found.
- `./gradlew assembleDebug` → **FALHA**: SDK location not found.
- `./gradlew test` → **FALHA**: SDK location not found.
- `./gradlew connectedAndroidTest` → **FALHA**: SDK location not found.

## Erro exato
`SDK location not found. Define a valid SDK location with an ANDROID_HOME environment variable or by setting the sdk.dir path in your project's local properties file.`

## Causa provável
Ambiente sem Android SDK instalado/configurado.

## Correção aplicada
Nenhuma alteração de código para mascarar erro. Mantido fail honesto.

## Artifact e hash
- Artifact debug APK: **NÃO GERADO (BLOCKED)**.
- SHA256: **N/A**.

## Status final
**BETA_BLOCKED** por ausência de SDK no ambiente de build.
