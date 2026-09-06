# ATLAS → NOVO → L → RMRCTI → ContextBundle: adapter local V1

Estado: `IMPLEMENTED / MEASURED_LOCAL / claim_allowed=false`.
Este é o sucessor executável do contrato em PR #422. A orquestração pertence
ao Termux; extração e ranking CTI continuam no produtor `llamaRafaelia`.

## O que executa

1. Confere a rota do Mapa por commit Git e bytes.
2. Lê uma conversa JSON delimitada, com identidade e SHA-256.
3. Confere o registro longitudinal e conserva os predecessores.
4. Executa `novoexport_chat_adapter_v2.py` do produtor fixado, em diretório temporário.
5. Chama `cti_memory::retrieve` e o gate `cti_privacy` desse mesmo produtor.
6. Liga cada hit a uma única mensagem de origem e valida envelope/chunks/ContextBundle.
7. Grava um novo diretório privado de resultado e um delta `LEARN`, sem sobrescrever fontes.

O adapter não chama o modelo, não extrai IntentIR e não concede capacidades.
O próximo consumidor ainda precisa passar pelos contratos de IntentIR e Governance.
`G3_CTI_CAUSAL_USE` continua aberto para **geração LLaMA**; os controles aqui
demonstram recuperação e presença/ausência de contexto.

## Dependências e limites

| Elemento | Limite ou autoridade |
|---|---|
| Python | 3.10 ou posterior, biblioteca padrão |
| Ponte nativa | C++17 e biblioteca padrão; compilação local com warnings como erros |
| Produtor | `llamaRafaelia@3dac5fd2c23a9c55361672d5d593d3e875146da3` |
| Fonte | Uma conversa JSON, até 8 MiB e 10.000 nós/mensagens |
| Consulta | 1–4.096 bytes UTF-8; top-k entre 1 e 20 |
| Pesquisa | `scan_fallback` do produtor, até três palavras-chave, sem curadores adicionais |
| Contexto | Até 400 bytes por trecho e 2.500 bytes totais, após gate de privacidade |
| Processo CTI | Timeout de 15 segundos; sem shell nem executável vindo do corpus |
| Saída | Diretório novo, modo 0700, fora das fontes, do produtor e do repo de trabalho |

Este adapter hospedado não é um núcleo freestanding. O código de extração,
ranking, privacidade e JSON do produtor não é recopiado para este repositório.
`atlas_cti_producer_pin.json` fixa os oito blobs exigidos, inclusive o header
nlohmann já versionado pelo produtor. O build registra também SHA-256, flags e binário.

## Preparação e execução

Use um checkout autorizado do produtor, com os blobs fixados. O build não faz downloads.

```bash
python3 tools/build_atlas_cti_bridge.py \
  --producer-root ../llamaRafaelia \
  --output-dir ../atlas-native-run-001

python3 tools/atlas_novo_context_adapter.py \
  --manifest ../atlas-input/source_manifest.json \
  --query-file ../atlas-input/query.txt \
  --producer-root ../llamaRafaelia \
  --native ../atlas-native-run-001/atlas-cti-bridge \
  --working-directory "$PWD" \
  --output-dir ../atlas-runs/interaction-001
```

O manifesto local tem estes campos. Substitua os placeholders por identidades
e hashes medidos; os placeholders não são uma fixture válida.

```json
{
  "schema": "rafaelia.atlas_novo_source.v1",
  "claim_allowed": false,
  "disclosure": "LOCAL_PRIVATE_CONTEXT",
  "route": {
    "route_id": "ATLAS:X-NOVO-RMRCTI-LLM-NAV-20260906",
    "path": "../Mapa/indices/deltas/ATLAS_X_NOVO_RMRCTI_LLM_NAV_20260906.md",
    "commit": "<commit Git contendo os mesmos bytes da rota>",
    "content_sha256": "<SHA-256 da rota>"
  },
  "source": {
    "source_id": "<identidade estável da fonte>",
    "authority": "GoogleDrive/NOVOexport",
    "provider_id": "<ID observado no provider>",
    "path": "conversation.json",
    "content_sha256": "<SHA-256 da conversa>"
  },
  "longitudinal": {
    "path": "lineage.json",
    "content_sha256": "<SHA-256 do registro longitudinal>"
  }
}
```

`lineage.json` deve conter `predecessor_ids` como lista não vazia.
A fonte precisa conter `id` ou `conversation_id`, `mapping`, IDs únicos de
mensagens e papéis `user`, `assistant`, `system` ou `tool`.
Para um export anexado, declare `UserProvided/Export`; para fixture, `Synthetic/Fixture`.
Um ponteiro Drive no manifesto não prova a identidade corrente do provider.

## Estados observáveis

| Resultado | Efeito |
|---|---|
| `ok` | Envelope, chunks e ContextBundle com hashes e fonte |
| `no_hits` | Zero chunks; nenhum ContextBundle inventado |
| `disabled` | Controle CTI desligado; nenhum contexto recuperado |
| `privacy_blocked` | Contexto retido; status explícito |
| `FAIL_CLOSED`, exit 2 | Entrada, hash, identidade ou binding inválido; sem receipt de sucesso |

Todo resultado concluído inclui `receipt.json` e `learn.json`.
`model_id=null`, `response_hash=null`, `model_executed=false` e
`weights_modified=false` preservam a diferença entre recuperação e geração.
Os diretórios locais podem conter texto privado já tratado pelo gate; publique
somente receipts minimizados e identidades autorizadas.

## Validação e reversão

```bash
python3 tools/validate_atlas_llm_navigation_contract.py
PYTHONPATH=tools python3 -m unittest discover -s tests \
  -p test_atlas_novo_context_adapter.py -v
```

Para os dez testes nativos, forneça `ATLAS_CTI_NATIVE`, `ATLAS_CTI_SOURCE_ROOT`
e `ATLAS_MAPA_ROOT`. Sem essas entradas eles são explicitamente skipped;
os oito testes do envelope continuam independentes do produtor privado.

Os receipts registram cada execução e seus hashes. A reversão consiste em
descartar apenas o novo diretório derivado; o rollback de código é o commit
predecessor da PR. Nunca apagar receipts anteriores para declarar um gate fechado.

## Limites que permanecem

`TOKEN_VAZIO`: provider NOVOexport corrente, cobertura integral do corpus,
geração LLaMA off/on, Android físico, GAIA, Private/Voynich, RLL imagem e Vectras.
O ZIP real usado nesta sessão é uma fonte anexada, com membro e recorte hash-bound;
ele não foi reclassificado como snapshot atual do Drive.

F_ok: adapter executável, validação fail-closed e controles locais.
F_gap: geração e providers/runtimes ainda sem o receipt correspondente.
F_next: consumir o ContextBundle em LLaMA fixado e repetir geração off/on/no-hit.

## Receipt desta implementação

`docs/receipts/ATLAS_NOVO_CTI_LOCAL_20260906.v1.json` registra os hashes do
código, build nativo, 18 testes executados e três controles com recorte real.
O log público contém somente nomes de testes. Consulta e texto do export não
integram os artefatos públicos; os hashes permitem conferir a fonte autorizada.
