# RAFAELIA Refutation Review

## Regra de Verdade Aplicada
- Código = verdade operacional.
- Documento sem teste/medição = hipótese.
- Benchmark sem artifact reproduzível = não comprovado.

## Matriz de claims
| Claim | Onde aparece | Código relacionado | Implementação | Teste | Benchmark | Contradição | Refutável? | Prova faltante | Status |
|---|---|---|---|---|---|---|---|---|---|
| BitRAF 42 bits encode/decode | `RAFAELIA_SEED_ALIGNMENT`, `raf_bitraf.h` | `app/src/main/cpp/lowlevel/raf_bitraf.c` | Sim | Não localizado selftest dedicado neste repo | Não | Não | Parcial | suíte de roundtrip + corpus ASM | PROVADO_NO_CÓDIGO |
| Ciclo 42 e estado T^7 em núcleo low-level | `RAFAELIA_SEMENTES`, docs auditoria | `rmr/Rrr/rafaelia_core.c`, `rmr/Rrr/rafaelia_b1.S` | Sim | Parcial (integração local) | Não formal | Não | Parcial | teste automatizado de invariantes T^7/42 | PROVADO_NO_CÓDIGO |
| ZipRAF completo em Termux | docs e seed narrativa | não encontrado módulo ZipRAF C/H em Termux | Não | Não | Não | Sim (doc menciona, código não) | Sim | portar código real (Vectras) + testes | DOCUMENTADO_NÃO_PROVADO |
| BitOmega FSM em Termux | seed/docs narrativos | não encontrado módulo `bitomega.*` em Termux | Não | Não | Não | Sim | Sim | implementação + invariantes FSM | DOCUMENTADO_NÃO_PROVADO |
| D_H ≈ 1.347 | `RAFAELIA_SEMENTES` | `raf_gp_dimension.c` existe, mas claim exato não validado por artifact | Parcial | Não conclusivo | Não artifactado | Potencial | Sim | pipeline Grassberger-Procaccia reprodutível | HIPÓTESE_MATEMÁTICA |
| 21 fontes + 21 sorvedouros (Poincaré-Hopf) | `RAFAELIA_SEMENTES` | sem contagem de índices implementada | Não | Não | Não | Sim | Sim | cálculo de índice por singularidade | HIPÓTESE_MATEMÁTICA |
| Bilhões/trilhões ops/s | docs benchmark taxonomia | `raf_top42_bench.c` + scripts | Parcial | Parcial | sem artifact device fixo | Sim (taxonomia alerta) | Sim | artifacts ARM32/ARM64 com metodologia fixa | PRECISA_MEDIÇÃO |

## Conclusão rápida
- Sustentado por código: BitRAF 42-bit, estrutura ciclo-42/T^7 operacional.
- Não sustentado neste repo: ZipRAF e BitOmega como implementação real.
- Claims matemáticos fortes permanecem hipóteses sem prova mínima.
