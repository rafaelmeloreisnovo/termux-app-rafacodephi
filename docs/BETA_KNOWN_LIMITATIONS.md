# Beta Known Limitations — Termux Rafacodephi

Data-base: 2026-05-05.

## BLOCKER_BETA

Nenhum BLOCKER_BETA conhecido nesta etapa; riscos restantes estão listados como RISK_BETA.

## Limitações conhecidas

| Classificação | Limitação | Impacto |
| --- | --- | --- |
| RISK_BETA | Abertura do app e terminal interativo precisam validação em device/emulador Android. | Host JVM não executa pty/JNI Android real. |
| RISK_BETA | Bootstrap depende de rede/upstream/checksum. | Build pode falhar fora de cache ou com release upstream indisponível. |
| RISK_BETA | Keystore local de `scripts/build_apk_matrix.sh` é validação interna. | Não substitui assinatura oficial de produção. |
| RISK_BETA | Benchmarks de device não foram executados nesta etapa. | Não há claim de performance quantitativa. |
| OK_BETA | APKs arm32 e arm64 são critérios explícitos do script de matriz. | Falha se `armeabi-v7a` ou `arm64-v8a` não forem gerados. |

## Features RAFAELIA experimentais

Tudo abaixo é **EXPERIMENTAL_NOT_BLOCKING** para a beta Termux:

- sqrt(3)/2
- Fibonacci
- Mandelbrot
- Poincare
- 42K atratores
- matriz 10x10x10
- BitOmega avançado
- ZipRAF avançado
- performance extrema
- claims matemáticos fortes ainda não validados por prova/teste independente
- benchmarks ainda não executados em matriz reproduzível de device

## Regra de release honesto

- Não vender hipótese como prova.
- Não vender benchmark ausente como performance.
- Não transformar release oficial em unsigned por conveniência.
- Não usar keystore local de validação como assinatura oficial.
- Não bloquear a beta por teoria que não é necessária para terminal abrir/executar/fechar.

## Próximos passos

1. Executar `./scripts/build_apk_matrix.sh` em runner com Android SDK/NDK.
2. Instalar APK arm64 em device/emulador e validar abertura do app.
3. Executar comando simples no terminal: `printf beta-ok`.
4. Fechar sessão e verificar ausência de processo órfão/zumbi via ferramentas disponíveis no device.
5. Registrar hashes e anexar artifacts no release/CI.
