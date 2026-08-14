# RAFAELIA Workflow Control Plane V1

## Objetivo

Transformar a coleção de workflows em uma operação previsível para dois públicos ao mesmo tempo:

1. **usuário leigo:** escolhe uma intenção e acompanha uma sequência curta, numerada e legível;
2. **mantenedor técnico:** continua tendo workflows especialistas, artefatos, gates, hashes, logs e fail-closed.

O ponto de entrada humano é:

`Actions → 🧭 RAFAELIA — Executar / Diagnosticar → Run workflow`

Não é necessário escolher entre dezenas de arquivos YAML para o fluxo principal.

## Quatro opções humanas

| Opção | Quando usar | O que executa |
|---|---|---|
| `diagnostico` | “Quero saber se o software básico está saudável” | inventário dos workflows + testes/smoke |
| `arm32-v7` | “Quero gerar/validar o APK do aparelho ARM32” | inventário + lane `armeabi-v7a` |
| `bootstrap-arm32` | “Quero avançar o Bootstrap Wizard do ARM32” | inventário + ARM32 + contrato do bootstrap + gate de evidência APK |
| `completo-seguro` | “Quero a cobertura principal antes de avançar” | inventário + testes + ARM32 + bootstrap + evidência |

Para ARM32 existem duas lanes preservadas:

- `canonical`: contrato atual do projeto;
- `ndk29`: compatibilidade explícita com NDK 29.

## Arquitetura

```text
                         usuário
                            │
                            ▼
             00-rafaelia-control-plane.yml
                            │
             ┌──────────────┼──────────────┐
             ▼              ▼              ▼
       workflow map      software       ARM32 v7
             │           tests              │
             │                         reusable core
             │                         /           \
             │                canonical           NDK29
             │
             ├──────────── bootstrap contract
             └──────────── APK evidence contract
                            │
                            ▼
                       Ω summary
```

### Pilar ARM32 reutilizável

A duplicação histórica entre `compatibility-arm32.yml` e `compatibility-arm32-ndk29.yml` foi removida do caminho pesado. Ambos são wrappers compatíveis sobre:

`_reusable-arm32-compat.yml`

Assim, build, inspeção ELF, verificação de assinatura, presença de `bootstrap-arm.zip`, checksums e receipt são mantidos em um único lugar.

## Governança de todos os YML

`scripts/ci/workflow_control_plane.py` percorre **todo** `.github/workflows/*.yml` e `.yaml` e gera:

- `reports/workflow-control-plane.json` — inventário auditável;
- `reports/workflow-control-plane.md` — visão humana;
- SHA-256 por workflow;
- `ci_track` e `ci_abis` ou `TOKEN_VAZIO` quando ausentes;
- triggers observados;
- capacidade `workflow_call`/`workflow_dispatch`;
- presença de `permissions`, `concurrency` e `timeout-minutes`;
- classificação operacional: `orchestratable`, `specialist-manual`, `autonomous-specialist`, `legacy-compatibility`.

Nada ausente é convertido silenciosamente em PASS. Lacuna permanece `TOKEN_VAZIO`/warning até ser tratada.

## Regra de não regressão

A migração é **compatível e incremental**:

- workflows antigos não são apagados em lote;
- nomes de checks importantes são preservados por wrappers quando possível;
- o control plane não transforma CI em prova física;
- falha de especialista exigido faz o resumo Ω falhar;
- `skipped` significa “não solicitado”, não PASS;
- `device_runtime_proof` permanece `TOKEN_VAZIO` até existir receipt real do Android.

## UX do resultado

O usuário não precisa ler logs inteiros primeiro. A última etapa `Ω Resultado simples` mostra:

- o que foi pedido;
- o que executou;
- `success`, `failure` ou `skipped` por pilar;
- qual etapa vermelha deve ser aberta;
- lembrete de que CI não substitui o aparelho físico.

## Próxima fase de refatoração

Depois que V1 estiver verde, a migração dos especialistas restantes pode seguir por famílias, sem big-bang:

1. build/release;
2. bootstrap/package;
3. benchmarks;
4. Vectras/IPC/provider;
5. auditoria/governança;
6. legados/deprecated.

Para cada família: `inventariar → extrair reusable → converter wrappers → provar equivalência → deprecar duplicata → remover somente após janela definida`.

## Claim boundary

`workflow orchestration != build proof != APK proof != device proof != release certification`.

O control plane melhora operação e rastreabilidade; não altera essa fronteira.
