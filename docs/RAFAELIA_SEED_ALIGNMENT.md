# RAFAELIA Seed Alignment

| Semente | Classificação | Evidência | Ação |
|---|---|---|---|
| RAF_STATE_DIM=7 / RAF_PERIOD=42 | IMPLEMENTADO | JNI core + vCPU | manter teste de ciclo |
| vCPU[8] e clock lógico | PARCIAL | raf_vcpu/raf_clock | ampliar instrumentação |
| DirectByteBuffer zero-copy | IMPLEMENTADO | RafaeliaCore + JNI | manter bounds |
| Arena sem malloc hot path | IMPLEMENTADO | JNI arena/baremetal_nomalloc | auditar benchmark |
| BitRAF ISA 42 bits | PARCIAL | raf_bitraf MVP encode/decode | validar com asm corpus |
| D_H Grassberger-Procaccia | SOLTO/EXPERIMENTAL | raf_gp_dimension MVP | medir com dataset maior |
| Hipóteses matemáticas (F*, n_crítico) | HIPÓTESE MATEMÁTICA | RAFAELIA_SEMENTES.txt | não promover sem medição |
| NEON EMA/hardware probe avançado | SOLTO/EXPERIMENTAL | rmr/Rrr/rafaelia_core.c | promover por etapas |
| Claims de performance | PRECISA MEDIÇÃO | sem benchmark oficial | criar baseline CI |
