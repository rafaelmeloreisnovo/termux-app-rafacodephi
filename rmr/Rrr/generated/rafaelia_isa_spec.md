# RAFAELIA Quantum ISA (BitRAF-42)

## 1) Word format (42 bits)
- `bits[0..6]`   harmonic frequencies field (`freq_q`)
- `bits[7..13]`  adaptive weights field (`w_q`)
- `bits[14..20]` toroidal phase field (`phase_q`)
- `bits[21..27]` partial CRC field (`crc7`)
- `bits[28..34]` vCPU load field (`load7`)
- `bits[35..41]` opcode field (`op7`)

Encoding:

`word42 = (op7<<35)|(load7<<28)|(crc7<<21)|(phase_q<<14)|(w_q<<7)|freq_q`

## 2) Immutable constants
- `SPIRAL_Q16 = 56755`
- `PHI_Q16 = 105965`
- `PI_Q16 = 205887`
- `Q16_2PI = 411774`
- `PERIOD = 42`

## 3) Opcodes
- `0x01 INIT_TORUS`
- `0x02 DIR_UP`
- `0x03 DIR_DOWN`
- `0x04 DIR_FORWARD`
- `0x05 DIR_RECURSE`
- `0x06 DIR_COMPRESS`
- `0x07 DIR_EXPAND`
- `0x08 DIR_PHASE_XOR`
- `0x09 COMMIT`
- `0x0A MEASURE`

`DIR_FORWARD` applies:

`F_{n+1} = F_n*(sqrt(3)/2) - pi*sin(279°)`

with fixed-point constant term `3146` in Q16.16.

## 4) Assembly-like syntax
- `INIT_TORUS f,w,p`
- `DIR_UP f,w,p`
- `DIR_DOWN f,w,p`
- `DIR_FORWARD`
- `DIR_RECURSE f,w,p`
- `DIR_COMPRESS f,w,p`
- `DIR_EXPAND f,w,p`
- `DIR_PHASE_XOR f,w,p`
- `COMMIT crc,load`
- `MEASURE mode`

## 5) Examples
- `INIT_TORUS`:
  - `op7=0x01 load7=0 phase_q=0 w_q=127 freq_q=1 crc7=0x12`
- `DIR_FORWARD`:
  - `op7=0x04 load7=64 phase_q=21 w_q=86 freq_q=3 crc7=0x4B`
  - `word42 = 0x0000008C96A83`
- `COMMIT`:
  - `op7=0x09 load7=100 phase_q=0 w_q=0 freq_q=0 crc7=0x55`

## 6) Execution contract
- Canonical scheduling uses cycle modulo 42.
- Verification happens on cycle multiples of 7.
- Global XOR + CRC32C gate the `COMMIT` transition.
