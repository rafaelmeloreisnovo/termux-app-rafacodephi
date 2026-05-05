# Beta Readiness Report — Termux Rafacodephi

Data-base: 2026-05-05.

## BLOCKER_BETA

Nenhum BLOCKER_BETA conhecido nesta etapa; riscos restantes estão listados como RISK_BETA.

## Resultado

**Status atual:** BETA_READY para trilha de engenharia local quando `./scripts/beta_selftest.py`, `./gradlew :app:testDebugUnitTest --no-daemon` e `./scripts/build_apk_matrix.sh` passam no ambiente Android configurado.

Este relatório separa código operacional de teoria experimental. Documentação é mapa; código é verdade operacional; teste valida função; benchmark valida quantidade.

## Inventário de achados

| Classificação | Área | Achado | Ação |
| --- | --- | --- | --- |
| BUG_BETA | Terminal lifecycle | `finishIfRunning()` matava apenas o pid do shell. Processos filhos simples poderiam sobreviver se o shell tivesse criado um grupo/sessão. | Corrigido para enviar `SIGKILL` ao grupo `-pid` e depois ao pid do shell como fallback. |
| BUG_BETA | Cleanup terminal | `mTerminalFileDescriptor` iniciava como `0`; em falha antes de abrir pty, cleanup poderia tentar fechar fd inválido/standard input. | Corrigido para iniciar em `-1`, fechar apenas fd `>= 0` e tornar cleanup idempotente. |
| BUG_BETA | JNI wait | `waitpid()` não repetia em `EINTR` e poderia ler `status` não inicializado em erro. | Corrigido com loop em `errno == EINTR` e retorno anormal limpo em falha. |
| BUG_BETA | Startup terminal | Exceção de `JNI.createSubprocess()` podia atravessar inicialização da sessão. | Corrigido com `try`/log/cleanup/notificação de sessão finalizada. |
| OK_BETA | Build | Gradle possui módulos app, terminal, shared, rafaelia e rmr integrados. | Manter `scripts/build_apk_matrix.sh` como caminho beta para signed+unsigned local. |
| OK_BETA | Artifact | Script de matriz valida arm32 (`armeabi-v7a`) e arm64 (`arm64-v8a`) e assina cópias release locais. | Hashes ficam em `dist/apk-matrix/SHA256SUMS.txt` após build. |
| RISK_BETA | Device runtime | App abrir e comando interativo real exigem device/emulador Android com bootstrap válido. | Validar manualmente antes de tag pública. |
| RISK_BETA | Bootstrap | Downloads/checksums dependem do bootstrap Termux e do ambiente SDK/NDK. | Usar `scripts/prepare_bootstrap_env.sh` e registrar hashes. |
| EXPERIMENTAL_NOT_BLOCKING | RAFAELIA | sqrt(3)/2, Fibonacci, Mandelbrot, Poincare, 42K, matriz 10x10x10, BitOmega avançado, ZipRAF avançado e performance extrema não são critérios de aceite beta. | Mantidos como pesquisa/roadmap; sem claim de prova ou performance. |

## O que funciona

- Build Gradle Android multi-módulo.
- Fluxo de terminal com pty nativo e filas Java.
- Encerramento defensivo da sessão terminal.
- Cleanup idempotente de filas e fd de pty.
- Script de matriz para APKs unsigned e signed local.
- Selftest beta host-side para contratos críticos e documentação.

## O que foi corrigido

- Tratamento de falha no início do subprocesso terminal.
- Proteção contra fechamento de fd inválido no cleanup.
- Cleanup idempotente para evitar dupla liberação de recursos.
- Encerramento do grupo de processos no fechamento da sessão.
- `waitpid()` robusto contra interrupção por sinal.

## Riscos restantes

- RISK_BETA: validação de abertura do app e execução de comando real precisa de device/emulador Android.
- RISK_BETA: bootstrap remoto pode falhar por rede, upstream ou checksum divergente.
- RISK_BETA: signed local é apenas validação interna; release oficial precisa secrets oficiais.

## Deixado para depois

- Benchmarks reproduzíveis de performance.
- Provas matemáticas ou claims fortes RAFAELIA.
- Integração avançada BitOmega/ZipRAF.
- Validação científica de Mandelbrot, Poincare, 42K e matriz 10x10x10.

## Como gerar build

```bash
./scripts/build_apk_matrix.sh
```

Saídas esperadas:

- `dist/apk-matrix/unsigned/*.apk`
- `dist/apk-matrix/signed/*-signed.apk`
- `dist/apk-matrix/SHA256SUMS.txt`
- `dist/apk-matrix/ARTIFACT_MANIFEST.txt`

## Como testar

```bash
./scripts/beta_selftest.py
./gradlew :app:testDebugUnitTest --no-daemon
```

## Hash/artifact esperado

O hash não é fixo antes do build local. Após executar `./scripts/build_apk_matrix.sh`, use:

```bash
cat dist/apk-matrix/SHA256SUMS.txt
```
