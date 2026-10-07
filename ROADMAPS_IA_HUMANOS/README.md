# ROADMAPS_IA_HUMANOS — RAFCODEΦ | 7 μpassos

> **Ponto de entrada para humanos e IAs.** Índice curto; não é fonte substituta, prova de execução, licença nem autorização de release. Atualize por sucessor com referência, sem apagar histórico.

**Estado editorial deste índice:** 2026-10-07 · `ROUTE_STATE=OPEN_DIAGNOSIS` · `claim_allowed=false` · `release_allowed=false` · `physical_android=TOKEN_VAZIO`.
**Escopo:** `rafaelmeloreisnovo/termux-app-rafacodephi` (consumidor de APK) ↔ `rafaelmeloreisnovo/termux-packages` (produtor de bootstrap e pacotes). O repositório produtor governa a correção do produtor.
**Invariantes:** SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM · TOKEN_VAZIO ≠ 0 · IMPLEMENTED_UNTESTED ≠ PASS. Nunca deduzir PASS de um PR aberto, de um log antigo ou de outro ABI.

## Comece aqui (profundidade 1)

| Destino | Quando abrir | Autoridade |
| --- | --- | --- |
| [Runbook por μpassos](./CHECKPOINTS_7MU.md) | Executar, auditar, parar ou retomar | Este índice; evidência sempre no produtor |
| [Índice de máquina](./index.v1.json) | Navegação por IA/script; IDs e estados tipados | Ponteiros e estados, não prova de CI |
| [Falha histórica: run 37660128059, job 112925303754](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/actions/runs/37660128059/job/112925303754) | Reconstruir causalidade dos steps 7 e 15 | GitHub Actions imutável por execução |
| [Workflow consumidor](../.github/workflows/freestanding-enterprise-closure.yml) | Conferir step 7, custódia, matriz APK e gates | Código vivo no HEAD acessado |
| [PR #484: symlink, importer/wizard e direitos](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/484) | Ver candidato de consumo amplo | Candidato, não promoção |
| [PR #487: upload bounded de receipts](https://github.com/rafaelmeloreisnovo/termux-app-rafacodephi/pull/487) | Isolar falha de transporte dos artefatos | Correção somente do step 15 |
| [Produtor termux-packages](https://github.com/rafaelmeloreisnovo/termux-packages) | Corrigir `bin/sh`, `libexec/termux-api`, launcher e .deb | Fonte produtora |
| [START HERE Ω V2.2](https://docs.google.com/document/d/1T4OIN2RC1I-KFsdaQTCqieeZUGYBXissWrdrZMG0DN8/edit) → [HOTSTATE V4.5](https://docs.google.com/document/d/1d0J5SkF2S2emBq6jLuTkhYVaSOQiWazJWyRCTMJ-iRI/edit) | Resolver estado longitudinal atual antes de agir | Drive memória/índice |
| [μWRITE LEDGER Ω V2](https://docs.google.com/document/d/1NEU7lg7iUZf1u7lX-SEKuc-knSDGlF5gOx7qzXcJ1yc/edit) | Registrar um delta material com receipt | Drive append-only |

**Não varrer o corpus inteiro:** CURRENT_STATE → este índice → 1–3 fontes mínimas; expandir só se faltar fonte, houver contradição, autoridade indefinida ou evidência ausente.

## Percurso rastreável

1. [μ1 — Estado e autoridade](./CHECKPOINTS_7MU.md#μ1--estado-e-autoridade)
2. [μ2 — Fronteiras causais](./CHECKPOINTS_7MU.md#μ2--fronteiras-causais)
3. [μ3 — Falsificadores baratos](./CHECKPOINTS_7MU.md#μ3--falsificadores-baratos)
4. [μ4 — Um produtor, um consumidor](./CHECKPOINTS_7MU.md#μ4--um-produtor-um-consumidor)
5. [μ5 — Gates exatos de CI](./CHECKPOINTS_7MU.md#μ5--gates-exatos-de-ci)
6. [μ6 — Custódia, direitos e dispositivo](./CHECKPOINTS_7MU.md#μ6--custódia-direitos-e-dispositivo)
7. [μ7 — Decisão, rollback e μWRITE](./CHECKPOINTS_7MU.md#μ7--decisão-rollback-e-μwrite)

## Incidente que iniciou a rota

- **Step 7 — falha primária**: `cannot seal profile; missing installed entries: bin/sh,libexec/termux-api`; `build_exit=1`. Houve construção parcial de pacotes `aarch64`, mas não bootstrap dual-ABI validado.
- **Step 15 — falha independente**: upload-artifact encontrou nome de pacote Debian com caractere `:` no caminho; modificar somente o transporte não corrige a falha primária do perfil.
- **A âncora `#step:7:260681` aponta a uma linha de listagem ZIP**, não à linha da falha; consultar o final do step 7.
- O job **Evidence and custody contracts** concluiu `success`, mas o job **Source-built gate-bearing APK candidate** concluiu `failure`. Não equivalem.
- **Estado vivo posterior:** PRs #481–#484 sobrepostos; #487 focado em upload; produtor também teve falha diferente de launcher `exit 126` e patch preparado. Não executar novamente um SHA obsoleto, não disparar builds pesados duplicados, não presumir estado atual de runs pelo que este índice observou em 2026-10-07.

## Definição de pronto

`DONE` exige: fonte/autoridade resolvidas; testes baratos no HEAD exato; build produtor ARM32 + ARM64 completo; ZIPs/manifests/hash de ambos; upload comprovado; APK matrix e hashes embutidos; avaliação de direitos/licenças; proteção provider validada; e receipts físicos dos **mesmos APKs** instalados em ARM32 e ARM64. Se faltar qualquer gate exigido pelo claim, permanecer `HOLD`.

**R3:** F_ok = incidente decomposto e rota reconstruível; F_gap = seleção autoritativa do consumidor + CI/custódia/direitos/dispositivos; F_next = observar falsificador barato mais informativo e somente então consumir um build terminal.
