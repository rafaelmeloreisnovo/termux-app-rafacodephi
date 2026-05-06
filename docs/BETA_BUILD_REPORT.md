# BETA_BUILD_REPORT

## Consolidação de estado (alinhado com BETA_READINESS_REPORT)
- CI remoto = **READY** (workflow beta com matriz arm32+arm64, unsigned+signed).
- build local sem SDK = **BLOCKED** (falha honesta por ausência de `ANDROID_HOME`/`local.properties`).
- runtime device = **PENDING** (aguarda smoke ADB e probe de cleanup em dispositivo real).

## Bootstrap beta validado
- Ordem Gradle garantida: `downloadBootstraps` executa antes de `verifyBootstrapZipsPresent`.
- BLAKE3 obrigatório antes de build final em scripts de build/workflow.
- `TermuxInstaller.java` preservado como caminho real de bootstrap (sem substituição por fluxo RAFAELIA lowlevel).

## Artifact hygiene
- Upload de artifacts no workflow exclui `dist/local-release.keystore`.
