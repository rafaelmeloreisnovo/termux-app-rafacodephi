# Engenharia de Build, Release e CI — Runbook Canônico

## Objetivo

Este documento consolida a trilha oficial de build/release/CI do repositório em um único ponto de verdade, com foco em:

- Android + Gradle + CMake + NDK + JNI;
- execução local reproduzível;
- execução em GitHub Actions coerente com build local;
- geração e publicação de artefatos sem desviar da trilha de release oficial.

## Fonte de Verdade

1. **Versões Android/NDK/Build Tools:** `gradle.properties`.
2. **Instalação do toolchain no CI:** `scripts/setup_android_toolchain.sh`.
3. **Orquestração principal de CI/CD:** `.github/workflows/rafaelia_pipeline.yml`.
4. **Builds auxiliares/manuais:** workflows em `.github/workflows/`.

## Cadeia Oficial de Execução

1. **Quality gate**: valida arquivos críticos + wrapper + estrutura nativa.
2. **Bootstrap toolchain Android**: leitura de `gradle.properties` e instalação via `sdkmanager`.
3. **Build**: debug/release por variante de pacote e por ABI.
4. **Validação**: presença de APKs mandatórios, checksums e testes.
5. **Publicação de artefatos**: upload no Actions/release.

## Contratos Estruturais

- Não duplicar lógica de setup Android em múltiplos workflows.
- Toda alteração de SDK/NDK deve ocorrer em `gradle.properties`.
- Toda alteração de bootstrap de toolchain deve ocorrer em `scripts/setup_android_toolchain.sh`.
- Workflows devem chamar o script de bootstrap, e não reimplementar parsing de propriedades.

## Execução Local (baseline)

```bash
bash -n scripts/setup_android_toolchain.sh
python3 scripts/validate_native_structure.py
./gradlew -q :app:printVersionName
```

> Observação: para build local completo, o ambiente precisa de Android SDK/NDK instalados e licenças aceitas.

## Resultado Esperado

- CI e build local passam a compartilhar o mesmo contrato de toolchain.
- Menos divergência entre workflows.
- Menor risco de erro mascarado por configuração ad-hoc.
