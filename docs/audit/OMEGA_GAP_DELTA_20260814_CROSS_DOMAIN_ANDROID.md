# OMEGA GAP DELTA — Cross-Domain Android / Termux / Licenses / Runtime

- timestamp_local: `2026-08-14T00:53:00-03:00`
- mode: `APPEND_ONLY | EVIDENCE_FIRST | ANTI_REGRESSION`
- claim_allowed: `false`
- promotion: `false`
- release: `false`

## Invariantes anti-regressão

1. `RUNTIME_EVIDENCE > REPOSITORY_DECLARATION > DOCUMENTATION > HYPOTHESIS`.
2. Estados históricos não são apagados; correções entram como `ERRATA` append-only.
3. `CI != DEVICE_RUNTIME`, `SPEC != RECEIPT`, `TARGET_PROFILE != OBSERVED_DEVICE`.
4. `Open Source Licenses/About != SBOM completo != runtime reachability`.
5. Evidência ausente permanece `TOKEN_VAZIO`.
6. Atualização de framework/SO não implica atualização simultânea de `vendor`, `vendor_dlkm`, HAL, APEX/Mainline, apps, JNI ou bibliotecas nativas.

## Evidência já localizada

- Drive `SENSOR_ANDROID_PROP.txt`, provider ID `1xm0IFBCMZNHBS7jqeAhtyFl-4mp_9ZOj`: snapshot do RMX3834/RE5C9F em Android 14 / SDK 34 / `RMX3834export_14_C.74`; OTA `RMX3834export_14.C.74_3740_202504101711`; security patch `2025-04-01`; `ro.product.cpu.pagesize.max=4096`; variantes ART A75/A55.
- Drive `firmware_variantes.txt`, provider ID `1zX9XMusX00-YrlTo3QwSDQ1Z5dUztCrp`: C.74 + kernel `5.4.254-android12-9-g46eed20d7a9b-ab708`.
- `rafaelmeloreisnovo/Rafaelia_Private/Low-level/README.md`, commit-base observado `e34b80d939c8f38a7a10850f2a15697b1b37f760`: target posterior declarado `RMX3834export_15_F.94`, Android 15/API 35, kernel `5.15.178-android13-8-gabf75819a85e-ab569`.
- Drive `NOVOexport`, provider ID `1P7hJq5R4fgYGEQIVNgRvllAad2lGxWEv`: rota de reconstrução cronológica das conversas/receipts do aparelho perdido.
- Este repositório já possui `docs/EXTERNAL_INTEGRATION_MAP.md`, `docs/BETA_KNOWN_LIMITATIONS.md` e `rmr/docs/LICENCIAMENTO_RAFAELIA.md`, que permanecem autoridades locais para integrações, limitações e licenciamento.

## P0 — urgentes e necessários

### GAP-RMX3834-001 — proveniência física F.94

- state: `TOKEN_VAZIO_DEVICE_PROVENANCE_F94`
- ausência: nenhum `getprop`/fingerprint/OTA/kernel bruto do exato aparelho destruído foi localizado ainda comprovando execução física F.94.
- gate de fechamento: artefato bruto timestampado + source pointer + identidade do device + hash/revision quando disponível.

### GAP-RMX3834-002 — ponte temporal C.74 → F.94

- state: `OPEN_P0`
- ação: varrer NOVOexport/índices/conversas por `RMX3834`, `RE5C9F`, `C.74`, `F.91`, `F.94`, `F.95`, `F.96`, `5.4.254`, `5.15.178`, `getprop`, `uname -a`, `OTA`, `4096`, `16384`.
- saída obrigatória: último estado C-series observado, primeiro F-series observado e intervalo `TOKEN_VAZIO` entre ambos quando não houver evidência contínua.

### GAP-RMX3834-003 — perfil CPU documental inconsistente

- state: `ERRATUM_REQUIRED`
- `Low-level/README.md` declara Cortex-A76, enquanto o snapshot físico preservado aponta variantes A75/A55.
- ação: registrar errata no repositório de origem; não reescrever silenciosamente o README histórico.

### GAP-RMX3834-004 — kernel por época

- state: `OPEN`
- separar: kernel físico C.74 observado, kernel F.94 declarado em documentação e eventual kernel físico F.94 quando encontrado.
- proibição: não derivar versão do framework Android apenas do nome da família do kernel.

## P0 — gates anti-retração

- bloquear claim `F.94 físico` enquanto `GAP-RMX3834-001` estiver aberto;
- bloquear equivalência `README target == aparelho observado`;
- bloquear claim físico de ABI/API/benchmark sem receipt de hardware quando o claim exigir hardware;
- preservar resultados negativos e buscas sem resultado como evidência de busca, não como prova de inexistência universal.

## P1 — Cross-Domain Dependency Attention Matrix

### GAP-XDOM-001
`OPEN`: falta grafo unificado `componente -> versão -> upstream -> licença -> API/ABI -> SO/kernel -> runtime -> segurança -> evidência`.

### GAP-XDOM-002
`PARTIALLY_CLOSED_THIS_BRANCH`: materializar contrato de nós/arestas tipadas em `docs/contracts/cross_domain_dependency_graph.schema.json`; execução/validação dinâmica ainda é `TOKEN_VAZIO_RUNTIME`.

### GAP-XDOM-003
`OPEN`: falta separar sistematicamente `inventariado`, `instalado`, `carregado`, `runtime-reachable`, `executado`, `medido` e `reproduzido`.

### GAP-XDOM-004
`OPEN`: falta promotion gate executável de upgrade. Regra mínima: `compatibility + tests + license + security + provenance + runtime evidence` para claims dependentes de hardware.

### GAP-XDOM-005
`OPEN`: falta cruzamento automatizado entre dependência fixada, upstream, advisories, licença e compatibilidade Android/API/ABI/kernel.

## P1 — licenças / About / OSS

- construir manifesto `component -> license -> obligation -> provenance`;
- preservar separação já documentada entre infraestrutura MIT e núcleo autoral RAFAELIA sob termos próprios;
- detectar conflitos/obrigações por componente e aresta;
- não usar tela About/Open Source Licenses como evidência de versão atual, uso em runtime ou inventário completo.

## P1 — estado multitemporal

Representar separadamente `system`, `vendor`, `vendor_dlkm`, `APEX/Mainline`, `HAL`, app, JNI/native libs, Termux packages e toolchains. Cada nó carrega sua própria versão/época/proveniência.

## P2 — pesquisa / anterioridade

- formalizar `Attention-Directed Cross-Domain Maintenance Graph` como arquitetura de pesquisa;
- comparar com SBOM/SCA, Dependabot/Renovate, agentes LLM de manutenção, dependency graphs e graph/cross-attention;
- `TOKEN_VAZIO_NOVELTY`: nenhum claim de novidade antes de revisão de anterioridade reproduzível e documentada.

## Fila executável

1. **P0.1** localizar evidência bruta F.94 no NOVOexport/índices/conversas.
2. **P0.2** materializar errata RMX3834 CPU/profile no `Rafaelia_Private`.
3. **P1.1** materializar schema cross-domain nesta branch.
4. **P1.2** gerar inventário inicial de Gradle/native/OSS/licenses/ABI/API com source pointers.
5. **P1.3** criar validator fail-closed: claim físico sem receipt => rejeição.

## Gate Ω

Uma lacuna só muda para `CLOSED` mediante identidade + fonte + timestamp + classificação `OBSERVED|INFERRED` + proveniência + teste/receipt quando aplicável. O que não satisfaz o gate permanece `OPEN` ou `TOKEN_VAZIO`; nenhum estado histórico é apagado.

## Delta de execução — 2026-08-14 / search pass 1

### C.74 source anchor resolved

`MESSAGES-00018.jsonl.txt` contains a `SOURCE_OBSERVED` message mapped back to:

- `source_path`: `conversations-012.json`
- `source_pointer`: `conversations-012.json#conversation[9].mapping["bbb21be2-9656-4287-8ba6-ec7970e74e8b"]`
- `message_id`: `bbb21be2-9656-4287-8ba6-ec7970e74e8b`
- `create_time`: `1749097001.42102`
- `claim_allowed`: `false`

The message preserves RMX3834/RE5C9F device information including Android 14/API 34, `RMX3834export_14_C.74`, the Android-14 fingerprint, kernel `5.4.254-android12-9-g46eed20d7a9b-ab708`, T612, Cortex-A75/A55, ABIs, Termux/UserLAnd versions, and Treble support.

Interpretation: **observed C.74 anchor**, not automatically “last C-series ever observed”.

### Negative observations — preserved, not overclaimed

- `INDEX__CODEX-00001.jsonl.txt`: exact `RMX3834export_15_F.94` search produced no match in the inspected resource; `5.15.178` matches were documentation/WIP mentions and did not establish physical runtime.
- `MESSAGES-00005.jsonl.txt`: inspected pass produced no `F.94` or `Android 15` match.
- `MESSAGES-00018.jsonl.txt`: physical-style C.74 data was found; an F.94 raw physical receipt was not established in this pass.

No negative result is interpreted as universal nonexistence.

### Gap-state delta

- `GAP-RMX3834-001`: unchanged — `TOKEN_VAZIO_DEVICE_PROVENANCE_F94`.
- `GAP-RMX3834-002`: `OPEN_P0 -> PARTIAL_SOURCE_ANCHOR`; one C.74 anchor is source-pointer resolved, F-series physical anchor remains missing.
- `GAP-RMX3834-003`: `ERRATUM_RECORDED` in `rafaelmeloreisnovo/Rafaelia_Private`, branch `audit/rmx3834-provenance-20260814`.
- `GAP-XDOM-002`: remains `PARTIALLY_CLOSED_THIS_BRANCH`; schema exists, dynamic validation/fixtures/CI remain `TOKEN_VAZIO_RUNTIME`.

No merge, release, promotion or physical-runtime claim was performed.
