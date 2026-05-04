# RAFAELIA ISA + BitRAF(42)

Formato de 42 bits por instrução:
- bits 0..6: `freq_idx[6:0]`
- bits 7..13: `weight_q[6:0]`
- bits 14..20: `phase_q[6:0]`
- bits 21..27: `crc7`
- bits 28..34: `vcpu_load[6:0]`
- bits 35..41: `opcode[6:0]`

Opcodes (7 direções + controle):
- `0x01 INIT_TORUS`
- `0x02 DIR_UP f,w`
- `0x03 DIR_DOWN f,w`
- `0x04 DIR_FORWARD` (recorrência `F_{n+1}=F_n*sqrt(3)/2-π*sin(279°)`)
- `0x05 DIR_RECURSE`
- `0x06 DIR_COMPRESS`
- `0x07 DIR_EXPAND`
- `0x08 DIR_PHASE_XOR`
- `0x09 COMMIT`
- `0x0A MEASURE`

Constantes imutáveis:
`SPIRAL_Q16=56755`, `PHI_Q16=105965`, `PI_Q16=205887`, `PERIOD=42`.

Exemplo codificação (`DIR_FORWARD`):
- `freq_idx=3`, `weight_q=86`, `phase_q=21`, `crc7=0x4B`, `vcpu_load=64`, `opcode=0x04`
- palavra: `(opcode<<35)|(vcpu_load<<28)|(crc7<<21)|(phase_q<<14)|(weight_q<<7)|freq_idx`
