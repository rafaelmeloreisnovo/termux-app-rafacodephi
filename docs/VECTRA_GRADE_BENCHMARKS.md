# Vectra-grade Industrial Benchmarks

Este documento define a configuração mínima para benchmarks industriais do `termux-app-rafacodephi`, alinhados ao padrão técnico usado no Vectra: medir desempenho real, regressão, estabilidade e custo de execução em vez de apenas declarar ganho.

## Objetivo

Criar uma trilha reproduzível que gere artefatos comparáveis entre commits, ABIs e dispositivos Android.

A suíte deve responder, com dados:

- O APK compila de forma limpa?
- O bootstrap foi validado por hash?
- Os `.so` nativos respeitam alinhamento de 16KB?
- O caminho C puro, C branchless e ASM/NEON estão distinguíveis?
- O JNI adiciona quanto overhead?
- O app piorou ou melhorou em latência, jitter, memória e tamanho?
- Há sinais de crash, ANR, signal 9, signal 11 ou phantom process kill?

## Métricas obrigatórias

### 1. Build metrics

- Tempo de build clean.
- Tempo de build incremental.
- Resultado de `:app:testDebugUnitTest`.
- Quantidade de APKs gerados.
- Tamanho por APK.
- Tamanho por biblioteca nativa `.so`.

### 2. Binary metrics

- SHA256 dos APKs e bibliotecas nativas.
- ABI detectada por artefato.
- Presença de universal APK.
- Presença de `armeabi-v7a` e `arm64-v8a`.
- Validação ELF por `readelf -l` quando disponível.
- Checagem de alinhamento `0x4000` para Android 15/16.

### 3. Runtime metrics

- Cold start.
- Warm start.
- Shell spawn.
- Custo médio de chamada JNI.
- Custo médio de operação nativa curta.
- Latência de primeira chamada nativa.

### 4. CPU/native metrics

- Caminho C escalar.
- Caminho C branchless.
- Caminho fallback C.
- Caminho ASM/NEON quando disponível.
- Throughput de operações vetoriais.
- Tempo de cópia de memória.
- Tempo de fill/zero.
- Produto escalar.
- Operações matriciais pequenas e médias.

### 5. Memory metrics

- RSS.
- Java heap.
- Native heap.
- Uso de arena estática.
- Contagem de alocações no caminho crítico.
- Pico de memória durante benchmark.

### 6. I/O metrics

- Leitura sequencial.
- Escrita sequencial.
- Leitura aleatória 4K.
- Escrita aleatória 4K.
- Latência de `fsync`.
- Throughput MB/s.

### 7. Stability metrics

- Crash count.
- ANR count.
- `signal 9`.
- `signal 11`.
- Phantom process kill.
- Falhas de bootstrap.
- Falhas de carregamento de `.so`.

### 8. Jitter/tail latency

- p50.
- p90.
- p95.
- p99.
- Média.
- Desvio padrão.
- Pior caso.

## Artefatos obrigatórios

A suíte deve gerar os seguintes arquivos em `dist/vectra-benchmarks/`:

```text
dist/vectra-benchmarks/
├── vectra_benchmark_report.md
├── vectra_benchmark_results.json
├── vectra_benchmark_results.csv
├── APK_SIZE_REPORT.tsv
├── SHA256SUMS.txt
├── native_libs.tsv
├── elf_alignment_report.tsv
└── run_environment.txt
```

## Critério de aceite

Um benchmark industrial só é considerado válido quando:

1. O commit SHA está registrado.
2. O ambiente está registrado.
3. Os APKs foram encontrados ou a falha foi registrada claramente.
4. Os hashes foram gerados.
5. O relatório JSON foi emitido.
6. O CSV foi emitido.
7. O Markdown final foi emitido.
8. As falhas não são escondidas: devem aparecer no relatório.

## Regra de honestidade técnica

Números estimados podem existir em documentação conceitual, mas benchmark industrial precisa separar:

- `measured`: medido de fato nesta execução.
- `derived`: calculado a partir de medidas.
- `declared`: declarado por configuração/código.
- `missing`: não medido por ausência de ambiente, APK, ADB, readelf ou dispositivo.

Nenhum ganho deve ser tratado como comprovado se não tiver artefato reproduzível.

## Execução local mínima

```bash
./scripts/run_vectra_grade_benchmarks.sh
```

## Integração com CI

O workflow deve:

1. Rodar preflight.
2. Rodar build/test quando possível.
3. Rodar `scripts/run_vectra_grade_benchmarks.sh`.
4. Fazer upload de `dist/vectra-benchmarks/` como artifact.

## F de resolvido

A configuração de benchmark industrial está formalizada como contrato técnico do projeto.

## F de gap

Ainda precisa conectar medições runtime reais via dispositivo Android/ADB e hooks JNI específicos para medir chamadas nativas internas.

## F de next

1. Criar script local que gere os artefatos mínimos.
2. Criar workflow GitHub Actions para publicar os relatórios.
3. Criar binário/hook nativo específico para medir C escalar, C branchless, fallback e ASM/NEON.
