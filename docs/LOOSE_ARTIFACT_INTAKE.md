# RAFCODE-Φ — Entrada de Artefatos Soltos

## Objetivo

Transformar arquivos dispersos em um inventário auditável sem apagar história, sem mover
conteúdo automaticamente e sem promover presença a runtime.

## Comando

```bash
python3 scripts/index_loose_operational_artifacts.py \
  --validate \
  --summary
```

Para produzir o inventário completo:

```bash
python3 scripts/index_loose_operational_artifacts.py \
  --validate \
  --output build/reports/loose-artifact-inventory.json
```

## Escopo inicial

```text
Arme/
BugOrAdd/
rafaelia/old/
arquivos documentais e fontes selecionadas na raiz
```

A política canônica está em `configs/loose-artifact-policy.json`.

## Registro produzido

Cada arquivo recebe:

```yaml
artifact_id:
path:
object_type:
content_sha256:
size_bytes:
status:
origin: TOKEN_VAZIO
 author: TOKEN_VAZIO
license: TOKEN_VAZIO
references: []
reference_count: 0
review_flags:
  references_reviewed: false
  integration_target_approved: false
  consumer_identified: false
  tests_identified: false
promotion_blockers: []
promotion_ready: false
build_consumer: TOKEN_VAZIO
integration_target:
evidence_state: SOURCE_PRESENT_ONLY
claim_allowed: false
next_action:
```

> A indentação visual de `author` acima representa o mesmo nível de `origin` e `license`;
> o JSON gerado é a fonte normativa.

## Referências

Para arquivos textuais de até 1 MiB, o indexador coleta candidatos de referência:

- URLs `http://` e `https://`;
- destinos de links Markdown.

A detecção não equivale a revisão. `references_reviewed` continua `false` até decisão
explícita e rastreável.

## Duplicatas

Conteúdos com o mesmo SHA-256 são agrupados. Nenhuma cópia é apagada automaticamente.
O estado passa a `DUPLICATE_CONTENT`; `canonical_duplicate_selection` é incluído nos
bloqueadores até a escolha documentada da fonte canônica.

## Requisitos de promoção

Um item só pode se tornar candidato real a patch quando deixar de possuir bloqueadores:

1. origem identificada;
2. autoria identificada;
3. licença identificada e compatível;
4. referências revisadas;
5. destino de integração aprovado;
6. consumidor identificado;
7. testes identificados;
8. duplicidade resolvida, quando aplicável.

Mesmo sem bloqueadores, a promoção continua manual e revisável.

## Invariante

```text
INDEXING_DOES_NOT_PROMOTE_TO_BUILD_OR_RUNTIME
AUTOMATIC_MOVE            = false
AUTOMATIC_DELETE          = false
AUTOMATIC_CLAIM_PROMOTION = false
```

## Estados

| Estado | Significado |
|---|---|
| `CANDIDATE_SOURCE` | fonte potencial, ainda não integrada |
| `CANDIDATE_DOCUMENT` | conteúdo potencial para documentação canônica |
| `REVIEW_REQUIRED` | precisa de leitura e classificação humana |
| `DUPLICATE_CONTENT` | mesmo conteúdo em múltiplos caminhos |
| `HISTORICAL` | memória preservada, fora da árvore operacional |
| `QUARANTINE` | tipo/origem insuficientes |
| `CANONICAL` | somente após revisão, destino, consumidor, licença e evidência explícitos |

## Fluxo de completude documental

```text
artefato solto
-> hash e classificação
-> referências candidatas
-> revisão de proveniência/autoria/licença
-> comparação com fonte ativa
-> decisão CANONICAL/HISTORICAL/DUPLICATE/QUARANTINE
-> destino e consumidor
-> patch revisável
-> testes
-> atualização dos mapas
```

Assim, um documento solto pode completar um documento canônico ou revelar código
candidato, mas não entra no build apenas porque foi localizado.
