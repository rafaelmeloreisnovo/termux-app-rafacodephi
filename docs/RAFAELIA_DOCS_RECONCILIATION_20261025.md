# RAFAELIA — Reconciliação documental de termux-app-rafacodephi — 2026-10-25

> **DRAFT / HOLD · Não promove claims, releases, versões nem direitos.** Documento adicional, não substitui predecessores. A única verdade do inventário de arquivos aqui é a árvore Git no HEAD exato descrito abaixo; o conteúdo operacional exige revisão específica.

## Corte e escopo

- Projeto: `rafaelmeloreisnovo/termux-app-rafacodephi`.
- HEAD ancestral: `e722ac6bb53e6a0d4b749e5add65ea36062059ba`.
- Árvore Git completa: **2.618 arquivos, 346 diretórios**, `truncated=false`.
- Documentos/textos por extensão: **473**; isto inclui documentação potencial, não necessariamente páginas publicadas.
- Diretórios raiz: **46**, dos quais **39 sem README/INDEX/FILES_MAP local direto** (critério heurístico, não prova de orfandade).
- Escopo de interesse `tools/`: **43 arquivos, 4 subdiretórios** — são arquivos, não aplicativos únicos.
- Função técnica: aplicação Android/Termux, bootstrap, APT, ABI e bridges de contexto.

## Divergência já observada

INDICE_DOCUMENTACAO.md registra revisão de julho/2026; comparar com os contratos APT/Android atuais e não alegar instalação física sem receipt.

O anterior não será apagado, renomeado ou sobrescrito silenciosamente. Quando houver conflito, prevalecem **fonte versionada → teste do SHA → recibo verificável**, não um selo de "completo" herdado de documento antigo.

## Roteamento atualizado

| Referência existente (verificada no snapshot) | Papel |
| --- | --- |
| [`README.md`](../README.md) | guia ou índice histórico |
| [`INDICE_DOCUMENTACAO.md`](../INDICE_DOCUMENTACAO.md) | guia ou índice histórico |
| [`docs/README.md`](../docs/README.md) | guia ou índice histórico |
| [`docs/STATUS.md`](../docs/STATUS.md) | fonte ou contrato em versão Git |
| [`tools/atlas_novo_context_adapter.py`](../tools/atlas_novo_context_adapter.py) | fonte ou contrato em versão Git |
| [`tools/raf_bootstrap_profile.py`](../tools/raf_bootstrap_profile.py) | fonte ou contrato em versão Git |
| [`tools/validate_bootstrap_package_install_contract.py`](../tools/validate_bootstrap_package_install_contract.py) | fonte ou contrato em versão Git |
| [`scripts/device_signed_apt_smoke.sh`](../scripts/device_signed_apt_smoke.sh) | fonte ou contrato em versão Git |
| [`docs/RAFCODEPHI_SIGNED_APT_HANDOFF.md`](../docs/RAFCODEPHI_SIGNED_APT_HANDOFF.md) | fonte ou contrato em versão Git |

**Áreas prioritárias de classificação e dono:** `app/`, `terminal-emulator/`, `terminal-view/`, `termux-shared/`, `bootstrap/`, `scripts/`, `tools/`, `contracts/`, `receipts/`, `docs/`. Estes caminhos são agrupamentos propostos, sem mudança física na árvore.

## Pastas sem índice local (amostra para triagem)

`Arme/`, `COMP/`, `bootstrap_src/`, `federation/`, `governance/`, `plugins/`, `tools/bootstrap/`, `tools/rafaelia-q16/`, `scripts/ci/`, `scripts/verification/`.

Um diretório listado não é automaticamente redundante, abandonado ou livre para remoção. Identificar dono, consumidores, licença e predecessores antes de qualquer reorganização.

## Formato mínimo para cada arquivo / aplicação

`PATH | GIT_BLOB_SHA | PRODUCER | CLASS | CAPABILITY | INPUT | OUTPUT | SIDE_EFFECT | DEPENDENCIES | LICENSE | TEST_SHA | EVIDENCE | OWNER_ROLE | GAP | ROLLBACK`.

Estados: `OBSERVED_SOURCE`, `DOCUMENTED`, `TESTED_SCOPED`, `RUNTIME_PROVEN`, `TOKEN_VAZIO_NOT_TESTED`, `SUPERSEDED`. Ausência não é zero; script listado não equivale a execução.

## Refatoração sem regressão

1. Inventário recursivo pinado ao HEAD e com preservação do blob SHA (não copiar dados privados na documentação pública).
2. Verificar se referências relativas apontam para caminhos realmente existentes; documentos históricos com links quebrados recebem registro de divergência.
3. Priorizar pastas sem índice local de acordo com impacto, não apenas ordem alfabética.
4. Criar `README` por diretório funcional somente após leitura de fontes e consumidores; não inventar interfaces ou capacidades.
5. Separar o que é código próprio, upstream e wrapper, preservando licenças e direitos de terceiros.
6. Atualizar índice de entrada por ponteiro para o documento sucessor; nunca apagar históricos, artefatos autorais ou branches.
7. Exigir revisão humana, CI exata e gate do provedor para autorizar merge; `claim_allowed=false` até a evidência aplicável.

## Falsificadores e fechamento R3

- **F_ok**: árvore recursiva consultada em HEAD exato; alvos da tabela de links checados na árvore base.
- **F_gap**: auditoria semântica total, teste de build/ABI/hardware, checagem exaustiva de links históricos, inventário jurídico e proteção de merge ainda NÃO realizados.
- **F_next**: link-checker negativo (alvo faltante), manifesto canônico por arquivo, revisão de licenças e prioridade P0 das pastas operacionais.
- **Rollback**: reverter somente commits de documentação do PR; preservar todos os predecessores.
