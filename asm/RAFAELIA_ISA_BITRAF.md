# RAFAELIA-ISA (BitRAF 42 bits)

## Formato de instrução (42 bits)
- `[41:36] OPCODE (6)`
- `[35:33] DIR (3)` — 7 direções toroidais + reservado
- `[32:23] LAYER (10)`
- `[22:11] IMM/W (12)`
- `[10:0] FLAGS/PAR (11)`

## Mnemônicos principais
- `DPU, DPD, DPL, DPR, DPF, DPB, DPC` (7 direções)
- `SINI layer, amp, phase`
- `WSET layer, w`
- `STEP n`
- `CGATE mode`
- `RLBK chkpt`
- `HASH mix`

## Exemplo
```asm
SINI 0, 1024, 279
WSET 0, 56756
STEP 42
CGATE PARITY
RLBK 1
```
