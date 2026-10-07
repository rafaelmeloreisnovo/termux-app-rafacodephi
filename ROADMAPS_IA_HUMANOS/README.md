# ROADMAPS_IA_HUMANOS — entrada 7M

**Objetivo:** reconstruir decisões e testar o menor próximo passo com evidência verificável. Escopo inicial: **RAFCODEΦ / Termux Android**. Este índice é uma rota de trabalho, **não** um relatório de APK aprovado.

## Escolha a navegação

- **Humano:** [7M Termux — roteiro com checklists e checkpoints](./TERMUX_RAFCODEPHI_7M.md).
- **IA/automação:** [7M_INDEX.json — passos, dependências, estados e evidências](./7M_INDEX.json).
- **Comprovante de campo (2026-10-07, não promovido):** [Control Center / Vectra V3 — bootstrap bloqueado, PA não medido, diagnóstico source-versus-device](./20261007_CONTROL_CENTER_V3_DEVICE_GAP.md) · [receipt tipado](./receipts/20261007_RAFCODEPHI_CONTROL_CENTER_V3.json).
- **Fonte canônica de estado:** [CURRENT_STATE Ω — HOTSTATE V4.5](https://docs.google.com/document/d/1d0J5SkF2S2emBq6jLuTkhYVaSOQiWazJWyRCTMJ-iRI/edit).
- **Protocolo de despacho:** [START HERE Ω V2.2](https://docs.google.com/document/d/1T4OIN2RC1I-KFsdaQTCqieeZUGYBXissWrdrZMG0DN8/edit).
- **História append-only:** [μWRITE LEDGER Ω V2](https://docs.google.com/document/d/1NEU7lg7iUZf1u7lX-SEKuc-knSDGlF5gOx7qzXcJ1yc/edit).
- **Planilha operacional existente:** [RAFCODEPHI — Matriz de Gates de Serviço](https://docs.google.com/spreadsheets/d/1m_QeBOrEAsX8UGMeaLADjwnmZvsbDHRM6ZYay9nss7Y/edit).

**7M (convenção deste diretório)** = **Meta → Mapa → Medida → Mudança mínima → Matriz de testes → Manifesto/receipts → Memória/controle**. Cada M tem um `checkpoint_id`, predicados de entrada/saída, dono da execução e ponte para evidência. Não é padrão externo nem métrica Six Sigma.

## Regra de reconstrução sem fricção

1. Leia o `CURRENT_STATE` antes do histórico; abra apenas 1–3 roots por hipótese.
2. Selecione o M mais cedo ainda bloqueado e verifique evidência **fresca**, **exata no HEAD**; nunca assuma que `QUEUED` virou `PASS`.
3. Verifique autoridade do repo produtor: `termux-packages` implementa o bootstrap/.deb; `termux-app-rafacodephi` integra e prova APK.
4. Aplique mudança mínima apenas se existir um falsificador de código; evite reruns de compilações pesadas já ativas.
5. Registre `source/ref | owner | HEAD | command/step | expected | observed | state | evidence | gap | rollback | next`.
6. Faça μWRITE somente se existir delta material, como sucessor append-only no ledger do Drive. Este diretório mantém **ponteiros** e não duplica todo o corpus.

**Invariantes:** `SOURCE ≠ ARTIFACT ≠ EXECUTION ≠ EVIDENCE ≠ CLAIM`; `TOKEN_VAZIO ≠ 0`; `IMPLEMENTED_UNTESTED ≠ PASS`; `claim_allowed=false` até gates suficientes.

**P0 direitos/licenças:** conferir autoria, licença, conteúdo e permissão antes de copiar/distribuir TAR, DEB, APK ou conteúdo de terceiros. Fonte compilada não implica direito de redistribuição irrestrito.

**Fronteira:** os estados deste primeiro índice são observações delimitadas de 2026-10-07; **não** substituem leitura atual do GitHub ou do HOTSTATE. Nenhum pipeline, execução no telefone, assinatura nem merge é alegado por este arquivo.
