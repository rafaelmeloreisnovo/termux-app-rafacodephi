# RAFAELIA Branchless Map

| Local | Classificação | Nota |
|---|---|---|
| `rmr/src/main/cpp/rmr.c` (`rev` inline asm) | ARCH_SELECTED | seleção por arquitetura + instrução específica |
| `app/src/main/cpp/lowlevel/baremetal_asm.S` | BRANCH_REDUCED | hot path otimizado, não prova branchless total |
| `app/src/main/cpp/lowlevel/*.c` | HAS_BRANCH | há if/loops explícitos |
| `rmr/Rrr/*.S` | HAS_BRANCH/BRANCH_REDUCED | mistura execução condicional e desvios |
| docs com termo branchless sem código | DOC_ONLY | claim documental |
| termo ausente em módulos | ABSENT | sem ocorrência literal |

Regra: não classificar como BRANCHLESS_REAL quando houver branch explícito no caminho analisado.
