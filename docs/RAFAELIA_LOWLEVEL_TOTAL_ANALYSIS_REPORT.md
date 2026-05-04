# RAFAELIA Low-Level Total Analysis Report

## F_ok
- Build oficial low-level definido em `app/src/main/cpp/Android.mk`.
- ASM oficial identificado (`termux-bootstrap-zip.S`, `baremetal_asm.S`) e inline asm em `rmr.c`.
- Núcleo BitRAF/vCPU/clock/memory layers existente no código oficial.

## F_gap
- BitStack está em Rrr, fora do build oficial.
- ZipRAF/BitOmega sem módulo oficial verificável.
- MissHit cache ainda taxonomia documental, sem implementação formal integrada.
- TOP42 sem artifact completo CI/device por métrica.

## F_noise
- divergência entre claims de docs e artifacts benchmark reproduzíveis.
- divergência entre modelo Hz/layers em Rrr e núcleo oficial.

## F_error
- Não identificado erro de compilação nesta etapa documental.
- Erro contratual potencial: promover claim de performance sem artifact.

## F_next
1. Consolidar artifacts CI para scheduler/memory/logical benchmarks.
2. Criar testes de equivalência entre Rrr e oficial para Hz→Layer e CRC chain.
3. Formalizar schema único (JSON/CSV) para TOP42.
4. Só depois decidir portar BitStack/ZipRAF/BitOmega.

## Tabela final
| arquivo | status | build | low-level | asm | inline asm | cache | TTL | branchless | benchmark | Pontos S |
|---|---|---|---|---|---|---|---|---|---|---|
| app/src/main/cpp/termux-bootstrap-zip.S | OFICIAL | sim | sim | sim | não | parcial | ausente | HAS_BRANCH | não | S completos no ASM_INDEX |
| app/src/main/cpp/lowlevel/baremetal_asm.S | OFICIAL/PARCIAL | sim condicional | sim | sim | não | sim | ausente | BRANCH_REDUCED | indireto | S completos no ASM_INDEX |
| rmr/src/main/cpp/rmr.c | OFICIAL | sim | parcial | não | sim | não explícito | ausente | ARCH_SELECTED | não | S completos no ASM_INDEX |
| rmr/Rrr/rafaelia_b1.S | SOLTO/Rrr | não | sim | sim | não | conceitual | conceitual | EXPERIMENTAL | não | S completos no ASM_INDEX |
| mvp/rafaelia_mvp_puro.s | MVP | não | sim | sim | não | conceitual | conceitual | EXPERIMENTAL | não | S completos no ASM_INDEX |
