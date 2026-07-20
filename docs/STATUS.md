# STATUS (Fonte de Verdade de Build/Release)

> Última revisão: 2026-07-20 (UTC)

Este documento consolida o estado **real e verificável** do pipeline Android (Gradle + NDK + CI) desta fork. A regra é separar promessa de prova: quando não houver backend ou teste real, o estado fica marcado como `TOKEN_VAZIO`, `PARCIAL`, `EXPERIMENTAL`, `NÃO_EXECUTADO`, `FALHA_SEM_ETAPA` ou `FUTURO`.

## Histórico de Revisões

| Data | Mudanças Principais |
|------|---------------------|
| 2026-07-20 | **CORREÇÃO OPERACIONAL DO PR #289**: inventário do `master` encontrou `actions/checkout@v7` em 33 workflows. Todas essas referências foram substituídas por `actions/checkout@v6`; `upload-artifact@v7`, `download-artifact@v8`, `setup-java@v5` e `gradle/actions@v6` foram preservados. |
| 2026-07-20 | **CORREÇÃO EPISTÊMICA**: retirada a afirmação de que `checkout@v6`, `upload-artifact@v7` e `gradle/actions/*@v6` eram inexistentes. A branch foi sincronizada com `master`; política versionada, auditor estático e índice canônico foram adicionados. |
| 2026-07-20 | Runs intermediários falharam sem steps/logs suficientes para atribuir causa-raiz. O sucesso do HEAD final continua não provado até existir run conclusivo. |
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
- **PROVADO ESTRUTURAL**: código/contrato existe e é validável estruturalmente, mas ainda pede execução, benchmark ou device real para produção.
- **VERSÃO_PUBLICADA**: o major/tag existe na fonte oficial; não prova execução neste repositório.
- **COMPATÍVEL_DECLARADO**: a política local aceita a referência; não prova sucesso do workflow.
- **NÃO_EXECUTADO**: não há run conclusivo associado ao commit analisado.
- **FALHA_SEM_ETAPA**: um run/job foi criado, mas não forneceu steps/logs suficientes para atribuir causa-raiz.
- **PARCIAL**: parte funciona, mas falta validação de ambiente real ou dependência externa.
- **TOKEN_VAZIO**: wrapper/ponte/nome existe, mas backend real ainda não foi entregue; é melhor explicitar isso do que simular verdade.
- **EXPERIMENTAL**: implementação em exploração, sem contrato de release.
- **FUTURO**: item planejado, não pronto.

## Estado CI/Pipeline (2026-07-20)

- **Correção `checkout@v7 → @v6`**: PROVADO ESTRUTURAL — os 33 arquivos encontrados no inventário da base estão no delta corretivo do PR.
- **Política de GitHub Actions**: PROVADO ESTRUTURAL — `docs/CI_ACTION_VERSION_POLICY.md` registra majors atuais, compatíveis e limites de inferência.
- **Auditor de referências**: PROVADO ESTRUTURAL — `scripts/audit_github_actions_refs.py` classifica SHA fixo, major atual, major compatível, major não permitido e referência flutuante.
- **Workflow de auditoria**: FALHA_SEM_ETAPA/NÃO PROVADO — runs intermediários foram criados, mas não expuseram steps/logs úteis; o HEAD final precisa de execução conclusiva.
- **actions/checkout**: COMPATÍVEL_DECLARADO — `v6` é o major publicado adotado nas 33 correções; `v4` permanece compatível onde ainda existir por decisão explícita.
- **actions/upload-artifact**: COMPATÍVEL_DECLARADO — `v7` foi preservado porque é publicado para GitHub.com; compatibilidade GHES/runner deve ser tratada separadamente.
- **actions/download-artifact**: COMPATÍVEL_DECLARADO — `v8` foi preservado; sua versão não deve ser inferida a partir de `upload-artifact`.
- **gradle/actions**: COMPATÍVEL_DECLARADO — `v6` foi preservado nos fluxos que já o utilizavam.
- **apksigner PATH**: PROVADO ESTRUTURAL — os workflows de compatibilidade contêm a configuração; a execução no HEAD deve ser confirmada por run próprio.
- **RAFAELIA pipeline**: PROVADO ESTRUTURAL/PARCIAL — as fases ψ→χ→ρ→Δ→Σ→Ω foram preservadas e oito referências de checkout foram corrigidas; funcionalidade completa depende de CI, segredos, toolchain e artefatos.

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
- Política de versões CI: `docs/CI_ACTION_VERSION_POLICY.md`.
- Auditor estático: `scripts/audit_github_actions_refs.py`.
- Workflow de auditoria: `.github/workflows/action-reference-audit.yml`.
- Índice canônico interativo: `docs/CANONICAL_INDEX.html`.

## Rota mínima para coerência operacional

Para manter a representação documental fiel ao estado real do repositório, a leitura mínima deve seguir esta ordem:

1. `README.md` — entrada institucional e contrato público do fork.
2. `docs/README.md` — hub canônico de navegação e leitura por objetivo.
3. `docs/ENGINEERING_SYSTEM_RUNBOOK.md` — execução real de build/release/CI.
4. `docs/RUNTIME_TRUTH_TABLE.md` — verdade operacional detalhada e limites de runtime.
5. `docs/CI_ACTION_VERSION_POLICY.md` — diferença entre versão publicada, compatibilidade e execução.
6. `scripts/audit_github_actions_refs.py --strict` — inventário estático determinístico.
7. Run do workflow `GitHub Actions Reference Audit` associado ao SHA analisado.
8. `docs/RAFAELIA_CODE_DOC_SYNC.md` e `docs/RAFAELIA_CODE_DOC_SYNC_REPORT.md` — coerência entre narrativa, código, teste e evidência.
9. `docs/CANONICAL_INDEX.html` — mapa visual do estado corrigido.

Se algum documento divergir do código, do script, do workflow, do log ou do artefato correspondente, a documentação deve ser corrigida ou rebaixada para `PARCIAL`, `NÃO_EXECUTADO`, `FALHA_SEM_ETAPA`, `TOKEN_VAZIO` ou gap explícito.
