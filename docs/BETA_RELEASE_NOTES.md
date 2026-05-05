# BETA_RELEASE_NOTES

## Entrega desta beta
- Gate inicial de build/CI documentado.
- Workflow beta para gerar APK debug e SHA256.
- Auditorias de lifecycle terminal, cleanup e módulos experimentais.

## Não entrega ainda
- Provação matemática RAFAELIA.
- Benchmark pesado no pipeline principal.
- Garantia de comportamento em todos OEMs/versões Android sem teste físico.

## Como testar
1. Configurar Android SDK/NDK local.
2. Rodar `./gradlew assembleDebug`.
3. Instalar APK debug no dispositivo.
4. Abrir app, executar `echo rafcodephi_beta_ok`, `exit 0`, `exit 7`.

## Como reportar bug
- Abrir issue com passos de reprodução, versão Android, ABI, logcat, logs do TermuxService.

## Logs para anexar
- `logcat` filtrado por `TermuxService`, `TermuxActivity`, `termux`.
- saída completa do Gradle com stacktrace quando build falhar.
