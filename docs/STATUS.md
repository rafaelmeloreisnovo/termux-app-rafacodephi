# STATUS (Fonte de Verdade de Build/Release)

> Última revisão: 2026-07-20 (UTC)

Este documento consolida o estado **real e verificável** do pipeline Android (Gradle + NDK + CI) desta fork. A regra é separar promessa de prova: quando não houver backend ou teste real, o estado fica marcado como `TOKEN_VAZIO`, `PARCIAL`, `EXPERIMENTAL` ou `FUTURO`.

## Histórico de Revisões

| Data | Mudanças Principais |
|------|---------------------|
| 2026-07-20 | **P0 CI RESOLVIDO**: `actions/checkout@v6`/`@v7` → `@v4` em 28 workflows; `actions/upload-artifact@v7` → `@v4`; apksigner PATH fix (commits 74ecbd69, 99a6d0f, fccb442, 037efb4, 7c9ceb7). Criação de `docs/CANONICAL_INDEX.html` (índice canônico interativo). Atualização de AUDITORIA_TAREFAS (Tarefa 2 resolvida). |
| 2026-07-12 | Estabelecimento da fonte de verdade de build/release. |

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

## Estado CI/Pipeline (2026-07-20)

- **actions/checkout**: PROVADO — todos os 28 workflows usam `@v4` (estável). Bloqueador P0 resolvido.
- **actions/upload-artifact**: PROVADO — todos os workflows usam `@v4`.
- **apksigner PATH**: PROVADO — `compatibility-arm32.yml` e `compatibility-arm32-ndk29.yml` incluem step de PATH fix para build-tools.
- **RAFAELIA pipeline**: 8 jobs todos com checkout corrigido (`rafaelia_pipeline.yml`).
- **Workflows ativos (37 total)**:
  - 9 workflows de build principal (beta-build, android15_arm64_build, debug_build, apk_matrix_build, apk_matrix_artifacts_variants, apk_arm32_signed_unsigned, apk_arm32_signed_unsigned_target29, attach_debug_apks_to_release, run_tests)
  - 8 workflows de bootstrap/CI (bootstrap-arm64-asm-sanity, bootstrap-rewrite-all-abis, bootstrap-rewrite-arm32, bootstrap-rafaelia-selftest, rafaelia-generated, rafaelia_pipeline, rafcodephi-auditor, arme-add-governance)
  - 7 workflows de compatibilidade (compatibility-arm32, compatibility-arm32-ndk29, device-runtime-smoke, abi_policy_consistency, pss3-failure-lab, audit-benchmark-contract, arme-benchmark)
  - 6 workflows de qualidade/validação (vectra-grade-benchmarks, top42_bench, gradle-wrapper-validation, dependency-submission, validate-real-pkg-promotion-contract, validate-bootstrap-package-install-contract)
  - 4 workflows de release (sign-release, manual_release_cleanup, trigger_library_builds_on_jitpack, rafaelia-native-safety, rafaelia-runtime-runner-ci)

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
- **Índice canônico interativo (novo)**: `docs/CANONICAL_INDEX.html`.

## Rota mínima para coerência operacional

Para manter a representação documental fiel ao estado real do repositório, a leitura mínima deve seguir esta ordem:

1. `README.md` — entrada institucional e contrato público do fork.
2. `docs/README.md` — hub canônico de navegação e leitura por objetivo.
3. `docs/ENGINEERING_SYSTEM_RUNBOOK.md` — execução real de build/release/CI.
4. `docs/RUNTIME_TRUTH_TABLE.md` — verdade operacional detalhada e limites de runtime.
5. `docs/RAFAELIA_CODE_DOC_SYNC.md` e `docs/RAFAELIA_CODE_DOC_SYNC_REPORT.md` — camada de coerência entre narrativa, código, teste e evidência.
6. `docs/CANONICAL_INDEX.html` — mapa visual interativo de workflows, bugs, gaps e mapa econômico.

Se algum desses documentos divergir do código, do script ou do workflow correspondente, a documentação deve ser corrigida ou rebaixada para `PARCIAL`, `TOKEN_VAZIO` ou gap explícito.
