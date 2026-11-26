# Cadeia de branches e custódia low-level

Cada fase material nasce em uma branch nova. Nenhuma branch histórica deve ser apagada.

## Fluxo

1. 'main': somente integração proveniente de PR.
2. 'phase/*': uma mudança delimitada, com predecessor explícito.
3. PR Draft/Hold: revisão automática e humana.
4. Gates: contrato, proveniência, fonte, Java/DEX, ELF/ABI, ARMv7, AArch64 e receipts.
5. Auditoria: dois revisores, um de implementação e um de custódia/integração.
6. 'beta/*': somente após todos os gates do escopo PASS.
7. 'release/*': somente após aprovação explícita e recibo de promoção.

A publicação em beta/release nunca é automática apenas porque a compilação terminou.

## Receipts

Cada receipt registra: event, parent, repository, branch, commit, source_refs, artifact_hashes, gate, result, falsifier, F_ok, F_gap, F_next, rollback e claim_allowed.

'TOKEN_VAZIO' deve nomear o campo faltante. Não é zero, falso nem PASS.

## Java autoral

'AuthorialBitOps' é uma biblioteca mínima de primitivas: sem imports, sem chamadas a java.util, java.lang, java.math, java.nio ou java.io, sem alocação explícita. O Java aqui é a fronteira hospedada; a versão crítica de produção deve ser implementada também no núcleo PURE_CORE e ligada por contrato de ABI.

A troca de um bit é O(1). A contagem/paridade usam operações fixas sobre palavra de 32 bits. Isso não prova que sejam mais rápidas que intrínsecas ou assembly: o benchmark por ABI é um gate separado.
