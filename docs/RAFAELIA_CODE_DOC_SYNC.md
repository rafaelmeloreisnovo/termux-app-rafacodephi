# RAFAELIA CODE↔DOC SYNC

## Regra de sincronização
- Toda alteração de código exige alteração documental correspondente.
- Toda alteração documental com claim exige origem rastreável (código, teste, benchmark, comando ou artifact).

## Estados
- CODE_AHEAD_DOC
- DOC_AHEAD_CODE
- CODE_DOC_SYNCED
- CLAIM_NEEDS_BENCH
- CLAIM_INVALID
- EXPERIMENTAL_ONLY

## Matriz
| Código | Documento | Teste/Validação | Artifact | Status |
|---|---|---|---|---|
| app/src/main/cpp/lowlevel/raf_vcpu.c | docs/RAFAELIA_VCPU_CLOCK.md, docs/RAFAELIA_MEMORY_MODEL.md | scripts/validate_code_doc_sync.py | top42.json | CODE_DOC_SYNCED |
| app/src/main/cpp/lowlevel/raf_clock.c | docs/RAFAELIA_VCPU_CLOCK.md, docs/RAFAELIA_HZ_AS_MEMORY.md | scripts/validate_code_doc_sync.py | top42.json | CODE_DOC_SYNCED |
| app/src/main/cpp/lowlevel/raf_memory_layers.c | docs/RAFAELIA_MEMORY_MODEL.md | scripts/validate_code_doc_sync.py | top42_env.txt | CODE_DOC_SYNCED |
| app/src/main/cpp/lowlevel/raf_bitraf.c | docs/RAFAELIA_ISA_BITRAF.md | scripts/validate_code_doc_sync.py | top42.json | CLAIM_NEEDS_BENCH |
| app/src/main/cpp/lowlevel/raf_gp_dimension.c | docs/RAFAELIA_SEED_ALIGNMENT.md | scripts/validate_code_doc_sync.py | top42.json | CLAIM_NEEDS_BENCH |
| BENCHMARKS_COMPARISON.md | docs/RAFAELIA_TOP42_BENCHMARKS.md | scripts/run_top42_bench.sh | top42.csv/top42.json | CLAIM_NEEDS_BENCH |
