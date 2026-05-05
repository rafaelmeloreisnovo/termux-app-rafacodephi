# RAFAELIA Hz as Memory

## 1. Hz físico
- Frequência de CPU/cpufreq é hardware.
- Não equivale automaticamente ao Hz lógico do motor RAFAELIA.

## 2. Hz lógico
- Campos observáveis: `target_hz`, `period_ns`, `actual_hz_q16`, `jitter_ns`, `missed_ticks`, `total_ticks` (clock/profile).
- Fonte: `raf_clock.c/.h`, wrappers em `RafaeliaCore.java`.

## 3. Hz como memória
- Cada tick carrega estado anterior (phase/step acumulam histórico).
- CRC sela consistência de estado em caminhos commit.
- `jitter_ns` = ruído mensurável.
- `missed_ticks` = ruído ou erro conforme contrato de operação.

## 4. Hz -> Layer
- Regra encontrada em trilha Rrr: `hz>50000=L1`, `>38000=L2`, `>25000=BUF`, senão RAM.
- Origem: Rrr (`rafaelia_core.c`/família b7).
- Núcleo oficial: memória por camadas existe, mas limiares precisam validação de sincronia.
- Status: PRECISA_TESTE no oficial.

## 5. TTL
- TTL literal não aparece como símbolo formal no core oficial.
- Mapeamento conceitual: TTL ~ ciclo de vida por tick/cycle/phase/period.
- Classificação: DOC_ONLY/HIPÓTESE até haver código explícito.
