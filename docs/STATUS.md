# STATUS (Fonte de Verdade de Build/Release)

> Última revisão: 2026-07-12 (UTC)

Este documento consolida o estado **real e verificável** do pipeline Android (Gradle + NDK + CI) desta fork. A regra é separar promessa de prova: quando não houver backend ou teste real, o estado fica marcado como `TOKEN_VAZIO`, `PARCIAL`, `EXPERIMENTAL` ou `FUTURO`.

## Verdade canônica atual

- `compileSdkVersion=35`
- `targetSdkVersion=28`
- `minSdkVersion=21`
- ABIs obrigatórias: `armeabi-v7a`, `arm64-v8a`
- `universalApk=true`
- package/applicationId: `com.termux.rafacodephi`

## Estado epistêmico fixo

- **PROVADO**: evidência executável/CI/local confirma o contrato.
- **PROVADO ESTRUTURAL**: código/contrato existe e é validável estruturalmente, mas ainda pede benchmark/device real para produção.
- **PARCIAL**: parte funciona, mas falta validação de ambiente real ou dependência externa.
- **TOKEN_VAZIO**: wrapper/ponte/nome existe, mas backend real ainda não foi entregue; é melhor explicitar isso do que simular verdade.
- **EXPERIMENTAL**: implementação em exploração, sem contrato de release.
- **FUTURO**: item planejado, não pronto.

## Runtime e bootstrap

O bootstrap atual fornece uma base mínima guardada para instalação e diagnóstico, mas ainda não equivale a uma distribuição Termux completa com backend apt real.

- `bin/sh`: existe como wrapper/base mínima quando presente no payload.
- `bin/pkg`: existe como bridge operacional.
- `bin/apt` e `bin/apt-get`: dependem de backend real (`apt`, `dpkg`, `libapt`, repositório e certificados) para instalação real.
- `bin/busybox`: deve delegar para `toybox`/`toolbox` quando possível, ou ser substituído por busybox real.
- `bin/proot`: precisa de `proot.real` ou equivalente para ser considerado pronto.

## Zero-malloc: limite honesto

Zero-malloc confirmado:

- RAFAELIA Direct JNI arena.
- CTI scanner.
- ZIPRAF manifest quando usado estaticamente.
- VCPU state kernel.

Não zero-malloc:

- `baremetal.c` default em matrizes/arena.
- Java side.
- `TermuxInstaller`.
- bootstrap extraction.

## ZIPRAF

ZIPRAF não comprime fisicamente. ZIPRAF cria endereçamento lógico multirresolução sobre bytes existentes. Portanto, a forma correta de documentar é: **1 GB físico pode ser exposto como 264 GB de espaço lógico endereçável, sem aumentar os bytes físicos armazenados.**

## VCPU

Nome técnico atual: **RAFAELIA deterministic VCPU state kernel** / **VCPU telemétrica determinística**. Ainda não é VM completa. Para virar VM completa faltam bytecode, registradores, memória, instruções, loader, executor, syscall table, testes, dump de estado e replay determinístico.

## Fonte de verdade (arquivos canônicos)

- Build e versões Android/NDK: `gradle.properties`.
- Matriz signed/unsigned: `scripts/build_apk_matrix.sh`.
- Contrato operacional: `docs/RUNTIME_TRUTH_TABLE.md`.
- Runbook operacional atual: `docs/ENGINEERING_SYSTEM_RUNBOOK.md`.
- Runbook legado complementar: `docs/ENGINEERING_RUNBOOK_RAFCODEPHI.md`.
- Visão macro do projeto: `README.md`.

## Rota mínima para coerência operacional

Para manter a representação documental fiel ao estado real do repositório, a leitura mínima deve seguir esta ordem:

1. `README.md` — entrada institucional e contrato público do fork.
2. `docs/README.md` — hub canônico de navegação e leitura por objetivo.
3. `docs/ENGINEERING_SYSTEM_RUNBOOK.md` — execução real de build/release/CI.
4. `docs/RUNTIME_TRUTH_TABLE.md` — verdade operacional detalhada e limites de runtime.
5. `docs/RAFAELIA_CODE_DOC_SYNC.md` e `docs/RAFAELIA_CODE_DOC_SYNC_REPORT.md` — camada de coerência entre narrativa, código, teste e evidência.

Se algum desses documentos divergir do código, do script ou do workflow correspondente, a documentação deve ser corrigida ou rebaixada para `PARCIAL`, `TOKEN_VAZIO` ou gap explícito.
