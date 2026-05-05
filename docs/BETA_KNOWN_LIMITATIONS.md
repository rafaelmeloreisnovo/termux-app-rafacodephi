# BETA_KNOWN_LIMITATIONS

- Build local atual bloqueado por ausência de Android SDK no ambiente.
- Abertura real do app/terminal ainda depende de validação em aparelho Android.
- Auditoria de processo zumbi requer teste dinâmico pós-fechamento de sessão em device real.
- Compatibilidade ABI arm32/arm64 depende de build e instalação em hardware alvo.
- Benchmarks RAFAELIA/top42 não fazem parte do gate da primeira beta.
- Teoria matemática RAFAELIA permanece **EXPERIMENTAL_NOT_BLOCKING** sem benchmark reprodutível.


## Certification and audit claim notice

This repository does not claim ISO certification, formal ISO compliance, or accredited external audit status. Any ISO/IEC references are internal checklist references or methodological alignment notes only. Certification requires an external accredited audit process and is outside the scope of this repository.

Este repositório não declara certificação formal baseada em ISO, conformidade ISO formal nem auditoria externa acreditada. Qualquer referência a ISO/IEC é apenas checklist interno, referência metodológica ou alinhamento preliminar de boas práticas. Certificação exige processo externo acreditado e está fora do escopo deste repositório.

### Audit/benchmark/runtime trail
- `docs/AUDIT_CLAIMS_POLICY.md`
- `reports/vectra_grade_benchmarks.md`
- `reports/device_runtime_smoke.md`
- `reports/rmr_equivalence.md`
- CI validação não equivale a validação em device real.
- Benchmark definido não equivale a benchmark medido.
