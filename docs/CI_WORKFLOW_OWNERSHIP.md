# CI Workflow Ownership Matrix

## Entrada humana canônica

Ponto preferencial para operação manual:

`.github/workflows/00_START_HERE.yml`

Nome visível: **00 START HERE — RAFCODEΦ Enterprise**.

O operador escolhe uma rota válida, em vez de combinar livremente parâmetros de dezenas de especialistas. Os especialistas permanecem isolados e auditáveis.

Rotas humanas:

- `01_DIAGNOSTICO`
- `02_ARM32_CANONICO`
- `03_ARM32_NDK29`
- `04_BOOTSTRAP_CANONICO`
- `05_BOOTSTRAP_NDK29`
- `06_EVIDENCIAS`
- `07_E2E`
- `08_VECTRA_V3`
- `09_ENTERPRISE`
- `10_PROVIDER`

## Workflows canônicos por trilha

| Trilha | Workflow | Autoridade | Claim boundary |
|---|---|---|---|
| `operator` | `.github/workflows/00_START_HERE.yml` | entrada humana, roteamento, receipt final | não cria prova física |
| `official` | `.github/workflows/apk_matrix_build.yml` | matriz de build/release | official exige assinatura/gates próprios |
| `artifact` | `.github/workflows/rafcodephi-usable-apk.yml` | materialização de APK candidato | artifact != device proof |
| `debug` | `.github/workflows/run_tests.yml` | testes/smoke de software | test PASS != physical runtime |
| `arm32` | `.github/workflows/_reusable-arm32-compat.yml` | build/ABI ARM32 | physical Android continua TOKEN_VAZIO |
| `bootstrap` | `.github/workflows/beta-real-bootstrap-contract.yml` | contrato estrutural do bootstrap | contract PASS != handset execution |
| `evidence` | `.github/workflows/apk-evidence-gate.yml` | parser/contrato de evidência APK | evidence contract != supplied physical evidence |
| `integration` | `.github/workflows/rafaelia_e2e_product_proof.yml` | integração E2E | CI integration != device proof |
| `metrology` | `.github/workflows/vectra-grade-benchmarks.yml` | contratos/artefatos Vectra em CI | PA physical execution permanece TOKEN_VAZIO |
| `provider-security` | `.github/workflows/provider-protection-gate.yml` | ruleset live do `master` | configuração live pode bloquear o Enterprise preflight |
| `governance` | `scripts/ci/workflow_control_plane.py` | inventário de todos os YML/YAML | discovery != execution |

## Dependências / imutabilidade do front door

Mapa de reconstrução: `docs/WORKFLOW_DEPENDENCY_BOUNDARY_V1.md`.

O fecho transitivo de reusable workflows a partir de `00_START_HERE.yml` é
auditado com `--reachable-from ... --require-immutable`: toda Action externa
nessa rota deve usar SHA completo. Isto endurece custódia/supply-chain; não
transforma GitHub Actions, runner, JDK ou Android SDK em código freestanding.

`FRONT_DOOR_IMMUTABLE != REPOSITORY_WIDE_IMMUTABLE != PHYSICAL_EXECUTION`.

## Provider protection — autoridade

A trilha `provider-security` usa a cadeia:

`governance/provider/PROVIDER_RULESET_TARGET.v2.json` → `scripts/ci/provider_protection_contract.py` → `.github/workflows/provider-protection-gate.yml` → receipt publicado → `00_START_HERE` rota `10_PROVIDER`/`09_ENTERPRISE`.

O JSON é a **fonte desejada**; o estado live do GitHub é uma observação separada. O evaluator não altera o ruleset administrativo e nunca converte ausência de permissão em PASS.

## Metadados obrigatórios

Todo workflow ativo deve declarar:

- `ci_track: <debug|internal|official|artifact|ops|deprecated>`
- `ci_abis: <csv ou n/a>`

Direção para workflows ativos:

- `permissions` mínimos explícitos;
- `concurrency` para execução manual quando aplicável;
- `timeout-minutes`;
- `persist-credentials: false` no checkout quando aplicável;
- `workflow_call` para pilares reutilizáveis;
- resumo humano;
- receipt/evidence envelope quando produz evidência.

Ausência histórica entra como `TOKEN_VAZIO`/warning, ou bloqueia quando `strict_governance=true`.

## Semântica de artifact

`source/build PASS → artifact bytes + digest + receipt`

não implica:

`install PASS → device runtime PASS → release certification`.

A trilha `artifact` pode produzir material instalável/candidato sem alterar `PHYSICAL_ANDROID=TOKEN_VAZIO`.

## Separação de responsabilidades

- START HERE escolhe e exige;
- especialista executa seu domínio;
- scanner inventaria;
- gate final avalia apenas os especialistas requeridos;
- receipt registra o que ocorreu;
- evidência física vem de execução física correspondente.

Nenhum pilar herda PASS de outro.

## Deprecated

Workflows marcados `ci_track: deprecated` não são fonte de verdade. Remoção deve ocorrer somente depois de:

`substituto canônico + equivalência observada + janela/rollback + ausência de dependência ativa`.

Não realizar big-bang delete só para reduzir a quantidade de YAML.

## Regra de segurança

`workflow_discovered != workflow_executed != build_pass != apk_proof != device_proof != release_certification`.

`TOKEN_VAZIO != 0 != PASS`.

`URGENCY != GATE_BYPASS`.

## Evolução 2026-11-11 — CI HEAD e ruleset aditivo

- **Pasta de execução:** manter os 85+ workflows de topo em `.github/workflows/`; mover para subpastas desativaria descoberta de Actions e quebraria referências `workflow_call`. Organização é **lógica**, via inventário e famílias, não por mover ou excluir YAML.
- **Entrada manual única:** `00_START_HERE.yml`, com rotas 01–11; a rota `11_REAL_PKG_TOOLCHAIN` é a trilha explícita de APK com APT real. Outras trilhas não herdam seu PASS.
- **Classificação gerada:** `scripts/ci/workflow_control_plane.py` acrescenta `family` e `family_counts`; classificação por nome é **heurística**, não equivalência de trabalhos. Relatório JSON/Markdown é gerado na CI; não manter um segundo catálogo manual concorrente.
- **Contrato obrigatório barato:** `.github/workflows/ci-head-contract.yml` tem job chamado exatamente `ci-head-contract`, gatilho `pull_request` para master sem `paths` e teste por `push` no master e na branch de canário de revisão. Verifica sintaxe YAML, Python, Bash, inventário, testes de contrato e payload sem build Android. A prova física permanece `TOKEN_VAZIO`.
- **Deduplicação segura:** apenas marcar `REVIEW_ONLY` quando existir assinatura estrutural repetida e evento automático compartilhado; NDK canônico ≠ NDK29; não eliminar um produtor sem equivalência comprovada.
- **Ruleset adicional (não substitui o 21908888):** `governance/provider/RAFCODEPHI_MASTER_RULESET_APPLY_V1.json` define proteção só do default branch, PR obrigatório, zero aprovações humanas (trabalho solo), status check estrito `ci-head-contract`, squash e nenhum bypass próprio. O ruleset antigo continua cobrindo `~ALL`. A presença deste JSON **não** altera as regras do GitHub.
- **Operação administrativa guardada:** `python3 scripts/ci/master_ruleset_gate.py` valida localmente; `--verify` somente lê o provider; `--apply --expected-head=<SHA_EXATO>` faz POST apenas quando houver check GitHub Actions verde no HEAD da branch de canário e um token de administração. Se existir ruleset de mesmo nome divergente, falha sem sobrescrever. Não automatizar esse POST em GitHub Actions nem expor credenciais.
- **Limite da proteção:** status check de `workflow_dispatch` sozinho não satisfaz necessariamente a verificação exigida pelo PR; o check obrigatório usa `pull_request` e jamais filtros de caminho. Antes do rollout, confirmar check concluído no HEAD e após aplicar verificar que PR com check ausente/falho não consegue mesclar. `READBACK_MATCH_SCOPED != NEGATIVE_ENFORCEMENT_TEST`.
- **Separação de evidências:** `ci-head-contract` confirma apenas contratos hospedados. APK e Bash físico dependem de build assinado e execução Android ARMv7/ARM64, com recibos distintos.
- **Rollout:** `LOCAL_VALIDATE → BRANCH_PUSH_CI_HEAD → PROVIDER_CHECKS_READ → APPLY_EXPLICIT_ADMIN → PROVIDER_READBACK → NEGATIVE_FALSIFIER → PR_REVIEW → PROMOTION`. Em qualquer falha, `BLOCKED` e não mesclar. Sem exclusão nem force push. O alvo `PROVIDER_RULESET_TARGET.v3.json` é um contrato aspiracional mais amplo, não se confunde com o aditivo inicial, portanto seu gate pode continuar FAIL.
- **Rollback:** não mesclar a branch em caso de regressão; revisão administrativa do novo ruleset por ID com readback e preservação do histórico, sem alterar o ruleset legado. Qualquer alteração de segurança após o POST requer ato administrativo explícito.

### Procedimento Termux (sem guardar PAT em arquivo)

```sh
python3 scripts/ci/master_ruleset_gate.py
python3 scripts/ci/master_ruleset_gate.py --verify
# Somente depois de ci-head-contract:success no SHA exato:
read -r -s -p 'GitHub PAT (repo administration:write): ' GH_TOKEN; printf '\n'
export GH_TOKEN
python3 scripts/ci/master_ruleset_gate.py --apply --expected-head="$(git rev-parse HEAD)"
unset GH_TOKEN
python3 scripts/ci/master_ruleset_gate.py --verify
```

A operação precisa ser executada em um checkout da branch de revisão na revisão exata. O token não deve ser publicado em YAML, issues, logs ou receipts. O CLI conectado à conversa atualmente não expõe a operação de escrita administrativa do ruleset.
