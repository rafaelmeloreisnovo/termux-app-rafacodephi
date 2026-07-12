# Centro de Documentação — RAFCODEΦ / RAFAELIA

> Ponto de entrada moderno para navegação, governança documental e excelência operacional.

## Propósito

Este hub reorganiza a leitura do repositório em uma estrutura mais clara, formal e confortável, com foco em:

- **entrada rápida** para diferentes perfis;
- **layout de navegação previsível**;
- **separação entre estratégia, operação, evidência e histórico**;
- **design conceitual consistente** para posicionar o projeto como referência operacional.

## Princípios de Design da Documentação

1. **Clareza antes de volume** — o leitor deve saber onde começar em menos de 1 minuto.
2. **Fonte de verdade explícita** — documentos canônicos devem ser fáceis de identificar.
3. **Separação de camadas** — visão executiva, operação, runtime, evidência e arquivos históricos não devem competir entre si.
4. **Elegância operacional** — documentação com linguagem objetiva, hierarquia visual estável e links curtos.
5. **Modernidade com rastreabilidade** — toda narrativa importante deve apontar para um contrato, runbook, status ou evidência.

## Leitura por Objetivo

| Objetivo | Documento inicial | Complemento |
|---|---|---|
| Entender o projeto rapidamente | [`../README.md`](../README.md) | [`STATUS.md`](./STATUS.md) |
| Operar build/release/CI | [`ENGINEERING_SYSTEM_RUNBOOK.md`](./ENGINEERING_SYSTEM_RUNBOOK.md) | [`RUNTIME_TRUTH_TABLE.md`](./RUNTIME_TRUTH_TABLE.md) |
| Avaliar maturidade operacional | [`EXCELENCIA_OPERACIONAL_MATRIX.md`](./EXCELENCIA_OPERACIONAL_MATRIX.md) | [`AUDIT_CLAIMS_POLICY.md`](./AUDIT_CLAIMS_POLICY.md) |
| Verificar se a documentação reflete o código-fonte | [`RAFAELIA_CODE_DOC_SYNC.md`](./RAFAELIA_CODE_DOC_SYNC.md) | [`RAFAELIA_CODE_DOC_SYNC_REPORT.md`](./RAFAELIA_CODE_DOC_SYNC_REPORT.md) |
| Navegar por toda a documentação | [`../INDICE_DOCUMENTACAO.md`](../INDICE_DOCUMENTACAO.md) | [`RAFAELIA_5_LEVEL_DOCUMENTATION_NAVIGATION.md`](./RAFAELIA_5_LEVEL_DOCUMENTATION_NAVIGATION.md) |
| Entender RAFAELIA e seus conceitos | [`rafaelia/README.md`](./rafaelia/README.md) | [`RAFAELIA_CONCEPT_CARRY_MAP.md`](./RAFAELIA_CONCEPT_CARRY_MAP.md) |
| Revisar auditorias e gaps | [`RAFAELIA_GAP_MATRIX.md`](./RAFAELIA_GAP_MATRIX.md) | [`RAFAELIA_CODE_DOC_SYNC_REPORT.md`](./RAFAELIA_CODE_DOC_SYNC_REPORT.md) |

## Arquitetura de Navegação

### 1. Camada Institucional
- [`../README.md`](../README.md)
- [`STATUS.md`](./STATUS.md)
- [`../DOCUMENTACAO.md`](../DOCUMENTACAO.md)

Uso: visão geral, posicionamento do fork, verdade operacional e entrada principal.

### 2. Camada Operacional
- [`ENGINEERING_SYSTEM_RUNBOOK.md`](./ENGINEERING_SYSTEM_RUNBOOK.md)
- [`EXCELENCIA_OPERACIONAL_MATRIX.md`](./EXCELENCIA_OPERACIONAL_MATRIX.md)
- [`RUNTIME_TRUTH_TABLE.md`](./RUNTIME_TRUTH_TABLE.md)

Uso: execução, controle, critérios de qualidade e governança de build/runtime.

### 3. Camada de Evidência
- [`AUDIT_CLAIMS_POLICY.md`](./AUDIT_CLAIMS_POLICY.md)
- [`VECTRA_GRADE_BENCHMARKS.md`](./VECTRA_GRADE_BENCHMARKS.md)
- [`BETA_RUNTIME_TEST_PLAN.md`](./BETA_RUNTIME_TEST_PLAN.md)

Uso: distinguir prova real, plano de teste, benchmark definido e claim ainda não comprovado.

### 4. Camada Conceitual RAFAELIA
- [`rafaelia/README.md`](./rafaelia/README.md)
- [`RAFAELIA_CONCEPT_CARRY_MAP.md`](./RAFAELIA_CONCEPT_CARRY_MAP.md)
- [`RAFAELIA_SESSION_TRUTH_NAVIGATION.md`](./RAFAELIA_SESSION_TRUTH_NAVIGATION.md)

Uso: semântica, metodologia, contratos conceituais e leitura disciplinada dos conceitos.

### 5. Camada de Inventário e Histórico
- [`../INDICE_DOCUMENTACAO.md`](../INDICE_DOCUMENTACAO.md)
- [`RAFAELIA_5_LEVEL_DOCUMENTATION_NAVIGATION.md`](./RAFAELIA_5_LEVEL_DOCUMENTATION_NAVIGATION.md)
- [`RAFAELIA_LOOSE_FILES_MAP.md`](./RAFAELIA_LOOSE_FILES_MAP.md)

Uso: localizar acervo amplo sem confundir inventário com fonte canônica.

### 6. Camada de Coerência Fonte ↔ Representação
- [`STATUS.md`](./STATUS.md)
- [`RUNTIME_TRUTH_TABLE.md`](./RUNTIME_TRUTH_TABLE.md)
- [`RAFAELIA_CODE_DOC_SYNC.md`](./RAFAELIA_CODE_DOC_SYNC.md)
- [`RAFAELIA_CODE_DOC_SYNC_REPORT.md`](./RAFAELIA_CODE_DOC_SYNC_REPORT.md)

Uso: garantir que o que está escrito continue compatível com o que o código, os scripts e a evidência realmente sustentam.

## Excelência Operacional no Contexto Deste Repositório

Excelência operacional, aqui, significa manter uma cadeia coerente entre:

- **intenção** (`README`, visão e contrato do fork);
- **governança** (`STATUS`, matrizes e políticas);
- **execução** (runbooks, scripts e workflows);
- **evidência** (auditorias, benchmarks, relatórios e testes);
- **recuperação e continuidade** (documentação de gaps, restrições e próximos passos).

Um documento elegante não é apenas “bonito”; ele reduz ambiguidade, melhora decisão técnica e acelera auditoria.

## Regra de Coerência Operacional

Cada afirmação documental relevante deve responder a três perguntas:

1. **onde isso existe no código-fonte?**
2. **onde isso está validado operacionalmente?**
3. **em que estado epistêmico isso se encontra hoje?**

Se a documentação não consegue apontar para código, contrato, script, teste, workflow ou evidência, ela deve ser rebaixada para hipótese, plano, gap ou `TOKEN_VAZIO`.

## Próxima Etapa até o Platô de Evolução Estrutural

| Etapa | Objetivo | Condição de saída |
|---|---|---|
| 1. Consolidar fontes canônicas | reduzir duplicidade e ambiguidade entre hubs, índices e status | `README`, `docs/README`, `STATUS` e `INDICE_DOCUMENTACAO` sem contradição material |
| 2. Sincronizar documento ↔ código | tornar explícita a ligação entre narrativa e implementação | `RAFAELIA_CODE_DOC_SYNC*` cobrindo os pontos operacionais prioritários |
| 3. Fechar gaps de evidência | aproximar documentação de testes, benchmarks e runtime real | `RUNTIME_TRUTH_TABLE` e relatórios com menos itens críticos em `TOKEN_VAZIO`/`PARCIAL` |
| 4. Atingir platô estrutural | estabilizar uma arquitetura documental coerente, realista e auditável | leitura por perfil estável, claims honestos e rastreabilidade clara até código/evidência |

Este “platô” não significa congelar o projeto. Significa chegar a uma estrutura em que evolução, manutenção e auditoria possam acontecer sem perder coerência entre representação documental e realidade operacional.

## Sequência Recomendada

1. Leia [`../README.md`](../README.md).
2. Confirme a verdade atual em [`STATUS.md`](./STATUS.md).
3. Consulte a malha de operação em [`EXCELENCIA_OPERACIONAL_MATRIX.md`](./EXCELENCIA_OPERACIONAL_MATRIX.md).
4. Verifique a coerência com [`RAFAELIA_CODE_DOC_SYNC.md`](./RAFAELIA_CODE_DOC_SYNC.md).
5. Execute processos a partir de [`ENGINEERING_SYSTEM_RUNBOOK.md`](./ENGINEERING_SYSTEM_RUNBOOK.md).
6. Use [`../INDICE_DOCUMENTACAO.md`](../INDICE_DOCUMENTACAO.md) para expansão temática.

## Resultado Esperado desta Estrutura

- melhor orientação para novos leitores;
- menor dispersão entre documentos críticos;
- hierarquia documental mais moderna e formal;
- leitura mais amigável para engenharia, auditoria e gestão;
- base mais sólida para o repositório ser percebido como referência operacional.
