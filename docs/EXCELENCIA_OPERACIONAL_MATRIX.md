# EXCELÊNCIA OPERACIONAL MATRIX

## Objetivo

Definir, de forma moderna e auditável, quais camadas sustentam a excelência operacional do repositório e qual evidência mínima separa intenção de prova.

## Modelo de referência

| Camada | Objetivo operacional | Prova mínima | Documento âncora |
|---|---|---|---|
| Estratégia | alinhamento entre visão, escopo e limites do fork | `README` e `STATUS` coerentes | [`../README.md`](../README.md) |
| Coerência fonte-representação | manter o texto fiel ao código, ao runtime e ao estado real | `STATUS`, `RUNTIME_TRUTH_TABLE` e `RAFAELIA_CODE_DOC_SYNC*` sem contradição material | [`RAFAELIA_CODE_DOC_SYNC.md`](./RAFAELIA_CODE_DOC_SYNC.md) |
| Build | gerar artefatos de forma reproduzível | APK gera sem desvio de trilha | [`ENGINEERING_SYSTEM_RUNBOOK.md`](./ENGINEERING_SYSTEM_RUNBOOK.md) |
| Release | separar unsigned interno de signed oficial | assinatura só ocorre por contrato explícito | [`BUILD_APK_MATRIX.md`](./BUILD_APK_MATRIX.md) |
| ABI | preservar matriz ARM oficial | `armeabi-v7a` e `arm64-v8a` presentes | [`STATUS.md`](./STATUS.md) |
| Bootstrap | preparar base mínima com integridade | prefix/private area criada com validação | [`BOOTSTRAP_LOWLEVEL_RAFAELIA_STATUS.md`](./BOOTSTRAP_LOWLEVEL_RAFAELIA_STATUS.md) |
| Runtime shell | garantir entrada operacional inicial | `sh` executa e mantém fluxo mínimo | [`RUNTIME_TRUTH_TABLE.md`](./RUNTIME_TRUTH_TABLE.md) |
| Pacotes | distinguir bridge de backend real | `pkg install` comprovado em evidência real | [`AUDIT_CLAIMS_POLICY.md`](./AUDIT_CLAIMS_POLICY.md) |
| JNI | manter ponte nativa disciplinada | chamada nativa validada sem regressão estrutural | [`LOWLEVEL_SUMMARY.md`](./LOWLEVEL_SUMMARY.md) |
| CTI | provar leitura determinística de arquivos | arquivo escaneado com saída consistente | [`../README.md`](../README.md) |
| ZIPRAF | provar manifesto lógico sobre bytes existentes | manifesto gerado sem claim de compressão física | [`STATUS.md`](./STATUS.md) |
| VCPU | manter estabilidade do kernel de estado | steps estáveis e replay coerente | [`RAFAELIA_MEMORY_MODEL.md`](./RAFAELIA_MEMORY_MODEL.md) |
| Device | validar comportamento fora do CI | relatório real de device publicado | [`VECTRA_GRADE_BENCHMARKS.md`](./VECTRA_GRADE_BENCHMARKS.md) |

## Pilares de excelência

### 1. Governança
- fonte de verdade identificável;
- terminologia estável;
- distinção explícita entre `PROVADO`, `PARCIAL`, `TOKEN_VAZIO`, `EXPERIMENTAL` e `FUTURO`.

### 2. Execução
- runbook único para build/release/CI;
- scripts e workflows alinhados ao mesmo contrato;
- trilha operacional sem atalhos não auditáveis.

### 3. Evidência
- benchmark definido não vale como benchmark medido;
- CI verde não substitui validação real em device;
- narrativa técnica deve apontar para artefato verificável.

### 4. Design conceitual
- documentos curtos para entrada;
- documentos densos para profundidade;
- inventário separado de canônicos;
- layout textual que prioriza leitura, conforto e tomada de decisão.

### 5. Coerência documental
- o que é “verdade atual” deve estar em `STATUS` e `RUNTIME_TRUTH_TABLE`;
- o que é divergência entre narrativa e implementação deve aparecer em `RAFAELIA_CODE_DOC_SYNC*`;
- o que não está comprovado deve permanecer explicitamente marcado.

## Níveis de maturidade

| Nível | Característica |
|---|---|
| Base | contrato existe e está documentado |
| Estrutural | implementação existe e passa validação estrutural |
| Operacional | execução real reproduzível em ambiente controlado |
| Auditável | evidência publicada e rastreável |
| Referência | operação consistente, leitura simples e manutenção previsível |

## Trilha até o platô de evolução estrutural

| Etapa | Foco | Evidência de avanço |
|---|---|---|
| Organização canônica | hubs, índices e status convergem | entrada principal sem rotas ambíguas |
| Sincronia código-documento | cada claim principal aponta para fonte executável | `RAFAELIA_CODE_DOC_SYNC*` e runbooks atualizados |
| Convergência operacional | gap entre CI, runtime e device fica explícito e gerenciado | menos contradições materiais em `STATUS`/`RUNTIME_TRUTH_TABLE` |
| Platô estrutural | documentação, código e evidência passam a evoluir no mesmo ritmo | governança previsível e leitura por perfil estável |

## Critérios para ser visto como referência operacional

O repositório se aproxima de referência quando combina:

1. **clareza documental**;
2. **execução reproduzível**;
3. **limites honestos de claim**;
4. **evidência navegável**;
5. **estrutura amigável para engenharia, auditoria e gestão**;
6. **coerência constante entre código-fonte, documentação e representação operacional**.

## Leitura complementar

- [`README.md`](../README.md)
- [`STATUS.md`](./STATUS.md)
- [`ENGINEERING_SYSTEM_RUNBOOK.md`](./ENGINEERING_SYSTEM_RUNBOOK.md)
- [`RUNTIME_TRUTH_TABLE.md`](./RUNTIME_TRUTH_TABLE.md)
- [`README.md`](./README.md)
