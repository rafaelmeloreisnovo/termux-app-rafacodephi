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
  --output reports/loose-artifact-inventory.json
```

## Escopo inicial

```text
Arme/
BugOrAdd/
rafaelia/old/
arquivos documentais e fontes selecionadas na raiz
```

A política está em:

```text
configs/loose-artifact-policy.json
```

## Registro produzido

Cada arquivo recebe:

```yaml
artifact_id:
path:
object_type:
content_sha256:
size_bytes:
status:
build_consumer: TOKEN_VAZIO
integration_target:
evidence_state: SOURCE_PRESENT_ONLY
claim_allowed: false
next_action:
```

## Duplicatas

Conteúdos com o mesmo SHA-256 são agrupados. Nenhuma cópia é apagada automaticamente.
O estado passa a `DUPLICATE_CONTENT`, e a próxima ação é escolher a fonte canônica ou
arquivar a cópia preservando proveniência.

## Invariante

```text
INDEXING_DOES_NOT_PROMOTE_TO_BUILD_OR_RUNTIME
```

O mapa serve para:

- localizar material que completa documentação;
- identificar implementações candidatas;
- separar histórico de build ativo;
- encontrar duplicatas;
- atribuir destino e consumidor;
- abrir tarefas de licença e proveniência;
- impedir que arquivos soltos sejam contados como função integrada.

## Estados

| Estado | Significado |
|---|---|
| `CANDIDATE_SOURCE` | fonte potencial, ainda não integrada |
| `CANDIDATE_DOCUMENT` | conteúdo potencial para documentação canônica |
| `REVIEW_REQUIRED` | precisa de leitura e classificação humana |
| `DUPLICATE_CONTENT` | mesmo conteúdo em múltiplos caminhos |
| `HISTORICAL` | memória preservada, fora da árvore operacional |
| `QUARANTINE` | tipo/origem insuficientes |
| `CANONICAL` | somente após revisão, destino e evidência explícitos |

## Próxima etapa

O inventário completo deve alimentar um segundo processo:

```text
artefato solto
-> revisão de proveniência/licença
-> comparação com fonte ativa
-> decisão CANONICAL/HISTORICAL/DUPLICATE/QUARANTINE
-> patch revisável
-> testes
-> atualização do mapa
```
