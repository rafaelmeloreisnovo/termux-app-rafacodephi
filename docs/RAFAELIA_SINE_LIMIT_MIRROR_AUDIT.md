# RAFAELIA Sine/Limit/Mirror Audit

## Campos operacionais
- `cycle_length`
- `mirror_index`
- `phase_index`
- `kin_index` (se existir)
- `min`
- `median`
- `max`
- `vortex_direction`
- `sine_phase_q16`

## Mapeamento
- Seno/cosseno/fase: há base Q16 e fases no core/ASM.
- Limite/inversão/espelho: possível como transformação de janela `phiWindow` e comparação min-mediana-max.
- Duas retas `\ | /`: classificar como geometria de seção/projeção, não prova dinâmica isolada.
- Tzolkin/Kin: **analogia simbólica/cíclica** por padrão; não claim matemático sem implementação formal.
