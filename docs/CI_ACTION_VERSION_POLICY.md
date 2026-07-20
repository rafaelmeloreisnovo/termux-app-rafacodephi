# Política canônica de versões — GitHub Actions

> Revisão factual: 2026-07-20  
> Escopo: `.github/workflows/*.{yml,yaml}`  
> Regra: **existência da versão, compatibilidade do runner e execução bem-sucedida são provas diferentes**.

## Correção do registro anterior

A afirmação de que `actions/checkout@v6`, `actions/upload-artifact@v7` e `gradle/actions/*@v6` eram versões inexistentes estava incorreta.

Na data desta revisão, os repositórios oficiais publicam e documentam:

| Ação | Major atual verificado | Majors compatíveis mantidos nesta base | Observação |
|---|---:|---|---|
| `actions/checkout` | `v6` | `v4`, `v6` | `v6` exige runner recente; `v4` continua sendo uma escolha compatível. |
| `actions/setup-java` | `v5` | `v4`, `v5` | Validar JDK e runner antes de atualizar. |
| `actions/upload-artifact` | `v7` | `v4`, `v5`, `v6`, `v7` | `v7` é publicado para GitHub.com; ambientes GHES possuem restrições próprias. |
| `gradle/actions/wrapper-validation` | `v6` | `v3`–`v6` | `v3` é antigo, porém publicado; `v6` é documentado oficialmente. |
| `gradle/actions/dependency-submission` | `v6` | `v3`–`v6` | A existência do tag não prova que a submissão funcionou neste repositório. |
| `gradle/actions/setup-gradle` | `v6` | `v3`–`v6` | A partir de versões modernas, `setup-gradle` também executa validação do wrapper. |

Fontes oficiais:

- https://github.com/actions/checkout/releases
- https://github.com/actions/upload-artifact/releases
- https://github.com/actions/setup-java/releases
- https://github.com/gradle/actions
- https://github.com/gradle/actions/releases

## Estados epistêmicos obrigatórios

- **VERSÃO_PUBLICADA** — o major/tag existe no repositório oficial.
- **COMPATÍVEL_DECLARADO** — a política local aceita o major, mas isso não prova execução.
- **EXECUTADO** — houve workflow run associado ao commit e o job terminou com sucesso.
- **NÃO_EXECUTADO** — não existe run associado ao commit analisado.
- **BLOQUEADO** — falha reproduzível atribuída à referência da action ou ao runner.
- **TOKEN_VAZIO** — evidência ainda ausente; não converter em “resolvido”.

## Política operacional deste repositório

1. Não rebaixar nem elevar majors apenas por memória, aparência do número ou suposição de inexistência.
2. Antes de alterar uma action, verificar o repositório oficial e os requisitos mínimos do runner.
3. Alterações em massa devem ocorrer em PR separado, com inventário antes/depois e execução CI associada ao HEAD.
4. Um workflow sem run não pode ser marcado como `PROVADO` ou `RESOLVIDO`.
5. `@v4`/`@v3` atualmente presentes podem ser **compatíveis**, mas não são automaticamente “as últimas versões”.
6. Preferir SHA imutável em trilhas de release quando a governança do projeto definir processo de atualização automatizada.

## Aplicação ao PR #289

O PR foi refeito sobre o `master` atual. As substituições em massa que repetiam alterações já presentes na base foram retiradas. O objetivo corrigido passa a ser:

- restaurar a verdade documental;
- auditar referências de actions de modo determinístico;
- separar compatibilidade declarada de execução comprovada;
- reconstruir o índice canônico sem declarar sucesso de CI inexistente.

`claim_allowed = false` enquanto não houver workflow run conclusivo associado ao HEAD do PR.
