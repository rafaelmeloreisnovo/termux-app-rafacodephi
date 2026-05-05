# RAFAELIA Total Matrix

| Elo | Oficial | Rrr | MVP | Fonte | Gap | Próximo passo |
|---|---|---|---|---|---|---|
| BIT | PARCIAL | SIM | SIM | raf_bitraf.c, rafaelia_core.c | sem unificação formal | consolidar contrato |
| BitStack | AUSENTE oficial | SIM | conceitual | rmr/Rrr/rafaelia_core.c | sem integração build oficial | portar com testes |
| ZipRAF | AUSENTE | hipótese | hipótese | docs/Rrr | sem código | manter HIPÓTESE |
| BitRAF | IMPLEMENTADO | SIM | parcial | app/src/main/cpp/lowlevel/raf_bitraf.c | benchmark parcial | artifact device |
| BitOmega | AUSENTE | hipótese | hipótese | docs dispersas | sem código | manter AUSENTE/HIPÓTESE |
| vCPU | IMPLEMENTADO | SIM | parcial | raf_vcpu.c | sem benchmark robusto em device | ampliar suite |
| Hz | IMPLEMENTADO | SIM | conceitual | raf_clock.c / Rrr cores | diferença target vs actual | medir jitter em device |
| L1/L2/BUF/RAM | PARCIAL | SIM | conceitual | raf_memory_layers.c / rmr Rrr | thresholds só em Rrr pleno | alinhar oficial |
| C/H | PARCIAL | SIM | conceitual | commit/state structs | sem schema único | definir schema |
| CRC | IMPLEMENTADO | SIM | parcial | commit_gate_ll.c / core Rrr | artifacts incompletos | CI+device artifacts |
| Commit Gate | IMPLEMENTADO | SIM | parcial | rafaelia_commit_gate_ll.c | sem política unificada | doc contrato |
| MissHit Cache | DOC_ONLY | parcial conceitual | ausente | docs | sem módulo oficial | manter conceito |
| TOP42 | SCRIPT/CI placeholder | n/a | n/a | scripts/run_top42_bench.sh | precisa device artifact | executar em hardware |
| ASM/Inline/Low-level | IMPLEMENTADO parcial | extenso | extenso | *.S, rmr.c | divergência oficial vs Rrr | mapear priorização |

Todos os itens devem ler S12 como: sem prova -> HIPÓTESE/EXPERIMENTAL/PRECISA_MEDIÇÃO.
