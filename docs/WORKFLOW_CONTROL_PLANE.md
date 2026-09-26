# RAFCODEΦ Workflow Control Plane V3 — START HERE

## Objetivo

Transformar a superfície de GitHub Actions em uma operação previsível para dois públicos simultaneamente:

1. **iniciante/operador:** escolhe uma rota humana pronta e recebe um resultado curto;
2. **engenharia/auditoria:** mantém especialistas isolados, gates independentes, hashes, logs, receipts e fronteiras epistemológicas.

Entrada humana canônica:

`.github/workflows/00_START_HERE.yml`

Nome visível: **00 START HERE — RAFCODEΦ Enterprise**.

## UX: um combo, somente combinações válidas

O GitHub Actions oferece inputs `choice`, mas não oferece dependência dinâmica entre dois dropdowns que desabilite valores incompatíveis. Por isso V3 evita a combinação livre `mission × ndk_lane`.

O operador escolhe **uma rota pronta**:

| Rota | Uso humano | Especialistas exigidos |
|---|---|---|
| `01_DIAGNOSTICO` | “O básico está saudável?” | inventário + política + testes |
| `02_ARM32_CANONICO` | “Validar/build ARM32 normal” | ARM32 canonical |
| `03_ARM32_NDK29` | “Validar/build ARM32 com NDK29” | ARM32 NDK29 |
| `04_BOOTSTRAP_CANONICO` | “Avançar bootstrap ARM32 normal” | ARM32 + bootstrap + evidence |
| `05_BOOTSTRAP_NDK29` | “Avançar bootstrap via NDK29” | ARM32 NDK29 + bootstrap + evidence |
| `06_EVIDENCIAS` | “Auditar artefato/integração” | APK evidence + E2E |
| `07_E2E` | “Executar apenas contrato E2E” | E2E |
| `08_VECTRA_V3` | “Validar metrologia/benchmark em CI” | Vectra V3 |
| `09_ENTERPRISE` | “Preflight principal amplo” | provider + tests + ARM32 + bootstrap + evidence + E2E + Vectra |\n| `10_PROVIDER` | “A proteção live do repositório está adequada?” | provider ruleset gate |

`strict_governance` é ortogonal: quando ligado, metadados ausentes deixam de ser warning e passam a bloquear a rodada.

## Regra do gate final

O gate final não pergunta “todos os jobs ficaram verdes?”. Ele pergunta:

`quais jobs esta rota declarou obrigatórios e qual foi o resultado observado de cada um?`

Isso evita o bug clássico em que um job opcional legitimamente `skipped` é tratado como falha obrigatória.

Semântica V3:

- `success` = etapa solicitada executou e passou;
- `failure` = etapa solicitada executou e falhou;
- `skipped` = **NOT_REQUESTED**, salvo se a rota explicitamente exigir a etapa;
- `TOKEN_VAZIO` = evidência ausente/desconhecida; nunca zero e nunca PASS.

O receipt é escrito **antes** do passo que encerra o workflow com falha. Assim, uma rodada bloqueada continua auditável.

## Responsabilidades

| Papel | Pilar |
|---|---|
| Operador | resolve uma rota válida |
| Auditor/CI | inventário de workflows, metadados e referências |
| Segurança/Arquitetura | ABI, pure-core, runtime collector read-only |\n| Provider security | ruleset live do `master`, status checks e política de PR |
| QA | testes de software |
| Build engineer | ARM32 canonical/NDK29 |
| Bootstrap engineer | contrato de bootstrap |
| Evidence/custody | APK evidence gate |
| Integration engineer | E2E proof contract |
| Metrologia | Vectra V3 CI |
| Auditor final | receipt + fail-closed gate |

Falha de um pilar requerido não é compensada por PASS de outro.

## Correções estruturais desta versão

### 1. Trigger inline

O scanner anterior reconhecia majoritariamente:

```yaml
on:
  workflow_dispatch:
```

mas podia perder:

```yaml
on: [push, pull_request, workflow_dispatch]
```

Isso fazia um workflow manual poder ser inventariado como autônomo. O scanner V2 reconhece formas block, scalar, sequence e flow-map sem depender de parser YAML 1.1, que pode interpretar `on` como boolean.

### 2. `ci_track=artifact`

O validador shell já aceitava `artifact`, enquanto o scanner Python não aceitava. V3 alinha ambos para o mesmo contrato:

`debug | internal | official | artifact | ops | deprecated`.

### 3. strict consistente

`strict_governance=true` é propagado para:

- inventário Python;
- auditoria de referências Actions;
- validador de metadados shell.

Não existe mais uma parte da rodada “strict” executando silenciosamente em compatibilidade.

## Vectra V3

`.github/workflows/vectra-grade-benchmarks.yml` agora é também `workflow_call`, permitindo orquestração tipada pelo START HERE.

A geração/upload de artefatos continua `always()` para preservar diagnóstico após falha. Para evitar ambiguidade, o workflow emite:

`dist/vectra-benchmarks/CI_EVIDENCE_ENVELOPE.json`

com:

- resultados observados de toolchain/contratos/build/geração;
- gate `PASS|FAIL`;
- `claim_allowed=false`;
- `pa_physical_execution=TOKEN_VAZIO`;
- `energy_validity=TOKEN_VAZIO`;
- `cross_device_comparability=TOKEN_VAZIO`;
- fronteira `CI artifacts != PA physical device measurement`.

Portanto:

`artifact diagnostic != PA receipt != governed n>=30 series != cross-device claim`.

## Governança do conjunto inteiro

`scripts/ci/workflow_control_plane.py` percorre todo `.github/workflows/*.yml|*.yaml` e gera:

- `reports/workflow-control-plane.json`;
- `reports/workflow-control-plane.md`;
- SHA-256 por workflow;
- `ci_track` e `ci_abis` ou `TOKEN_VAZIO`;
- triggers;
- `workflow_call` / `workflow_dispatch`;
- presença de `permissions`, `concurrency`, `timeout-minutes`;
- papel operacional.

Descoberta ou callability não é execução.

## Fronteira de evidência

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`

Em particular:

- CI PASS não é receipt físico Android;
- build ARM32 PASS não prova runtime no handset;
- Vectra CI PASS não prova PA física;
- `n>=30` não prova sozinho estabilidade ambiental ou comparabilidade;
- urgência não autoriza bypass de gate;
- `claim_allowed=false` permanece até o gate correspondente existir.

## Receipt START HERE

Toda rodada produz `rafcodephi-start-here-receipt.json` contendo:

- repositório, SHA, run id e attempt;
- route id / mission / lane;
- jobs exigidos;
- resultados observados;
- jobs obrigatórios que falharam;
- boundary flags;
- estados físicos `TOKEN_VAZIO`;
- gate final.

## Migração

A refatoração é incremental:

`inventariar → extrair reusable → orquestrar → provar equivalência → deprecar duplicata → remover somente com evidência`.

Workflows especialistas continuam úteis para diagnóstico focal. O START HERE é o **front door**, não um monólito que reimplementa especialistas.

## R3

`F_ok`: rota única, combos inválidos removidos, gate route-aware, scanner corrigido, Vectra callable/evidence-envelope e provider protection tipado como especialista.

`F_gap`: o CI desta mudança precisa executar; PA físico, série n>=30, energia calibrada e comparabilidade entre aparelhos continuam fora do alcance do runner hospedado.

`F_next`: observar CI do PR; só depois considerar integração em `master`.
