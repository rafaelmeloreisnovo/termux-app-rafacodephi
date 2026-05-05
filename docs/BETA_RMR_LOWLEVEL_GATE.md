# BETA_RMR_LOWLEVEL_GATE

| Módulo | Entra na beta? | Motivo | Teste existe? | Risco | Status |
|---|---|---|---|---|---|
| BitRAF (`raf_bitraf.*`) | Sim (condicional) | simples, isolado, não crítico ao terminal | mínimo unitário pendente | baixo | SUPPORT_BETA |
| `raf_clock` | Sim | utilitário isolado | parcial | baixo | SUPPORT_BETA |
| `raf_vcpu` | Sim (condicional) | não interfere diretamente no lifecycle terminal | parcial | médio | EXPERIMENTAL_NOT_BLOCKING |
| `raf_memory_layers` | Sim (condicional) | leitura de capacidades memória | não formal | baixo | EXPERIMENTAL_NOT_BLOCKING |
| RAFAELIA matemática (RMR) | Não obrigatório | claims sem benchmark formal beta | incompleto | médio | EXPERIMENTAL_NOT_BLOCKING |
| ASM (`*.S`, `*.s`) | Desligado por padrão | risco ABI/toolchain | parcial | médio | EXPERIMENTAL_NOT_BLOCKING |
