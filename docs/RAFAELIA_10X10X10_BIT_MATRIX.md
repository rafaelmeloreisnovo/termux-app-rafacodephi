# RAFAELIA 10x10x10 Bit Matrix

## Formalização mínima
- 10×10×10 = 1000 células (bit_volume base).
- Se existir `N_EXTRA=8`, total lógico passa a 1008 (metadados/controle), não volume geométrico puro.

## Campos propostos
- `bit_state`: valor binário por célula.
- `bit_projection`: projeção 1D/2D de subespaço.
- `bit_plane`: fatia z=k.
- `bit_volume`: cubo completo 1000.
- `bit_vortex`: direção/índice de rotação local.
- `bit_sine_phase`: fase Q16 associada.
- `bit_crc`: checksum de bloco.
- `bit_route`: rota/trajectory id.
- `bitomega_state`: estado FSM quando existir módulo.

## Estado atual neste repositório
- Matriz 10x10x10 está documentada conceitualmente; não há módulo único consolidado com API explícita para indexação 1000/1008.
- Relação com BitRAF: possível via packing de campos e layer/opcode.
- Relação com ZipRAF/BitOmega: depende de import de implementação externa (Vectras).

## Regra anti-exagero
Não inferir prova física/7D apenas por estrutura 10x10x10 sem teste quantitativo.
