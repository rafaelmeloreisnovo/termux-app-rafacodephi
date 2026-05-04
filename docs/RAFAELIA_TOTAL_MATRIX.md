# RAFAELIA Total Matrix

| Cadeia | Oficial | Rrr | MVP | Fonte principal | Gap | Próximo passo |
|---|---|---|---|---|---|---|
| BIT | Parcial | Sim | Sim | `rmr/Rrr/rafaelia_core.c` | sem formalização única | consolidar contrato binário |
| BitStack | Não oficial | Sim (N_STACKS=1000,N_EXTRA=8,N_TOTAL=1008) | Não | `rmr/Rrr/rafaelia_core.c` | fora do build | portar com testes |
| ZipRAF | Ausente oficial | não comprovado | não comprovado | docs | sem código | classificar HIPÓTESE |
| BitRAF | Sim | parcial | docs | `app/src/main/cpp/lowlevel/raf_bitraf.*` | cobertura teste | ampliar artifacts |
| BitOmega | Ausente oficial | não comprovado | não comprovado | docs | sem código | classificar HIPÓTESE |
| vCPU | Sim | Sim | parcial | `raf_vcpu.*`, `rafaelia_core.c` | divergência de modelos | alinhar API |
| Hz | Sim | Sim | conceitual | `raf_clock.*`, `rafaelia_core.c` | gaps de medição artifact | benchmark device |
| L1/L2/BUF/RAM | Parcial | Sim | conceitual | `raf_memory_layers.*`, `rafaelia_core.c` | critérios diferentes | normalizar mapping |
| C/H | Parcial | Sim | conceitual | `rafaelia_core.c` | falta integração oficial completa | especificar schema |
| CRC | Sim | Sim | parcial | `rafaelia_core.c`, lowlevel | CRC chain incompleta oficial | testes cruzados |
| Commit Gate | Sim | conceitual | não | `rafaelia_commit_gate_ll.c` | sem artifact CI robusto | publicar artifact |
| MissHit Cache | Doc only | conceitual | não | docs | sem módulo formal | definir contrato/medição |
| TOP42 Benchmark | Parcial | docs | docs | `raf_bench_suite.c`, docs | claims sem artifact | taxonomy + CI artifacts |
| ASM/Inline ASM | Sim parcial | Sim | Sim | `.S/.s`, `rmr.c` | falta índice único | `RAFAELIA_LOWLEVEL_ASM_INDEX.md` |

## Pontos S (matriz global)
S0: tabela acima + arquivos fonte.
S1: misto OFICIAL/SOLTO/MVP/HIPÓTESE.
S2: cadeia operacional RAFAELIA.
S3: prevalece CODE_AHEAD_DOC e CLAIM_WITHOUT_ARTIFACT em benchmarks.
S4: verificar bounds/CRC/overflow/JNI direct capacity/page-size.
S5: selos via CRC/hash/artifact ainda parciais.
S6: scheduler via `raf_clock` + `phase/tick` em Rrr.
S7: memória via `raf_memory_layers` + arena Rrr.
S8: ruído = delta target/actual/jitter/miss.
S9: erro = violação CRC/ABI/buffer/build.
S10: schemas precisam consolidação JSON/CSV benchmark.
S11: source-of-truth = código + testes + artifact.
S12: sem prova => HIPÓTESE/EXPERIMENTAL/PRECISA_MEDIÇÃO.
