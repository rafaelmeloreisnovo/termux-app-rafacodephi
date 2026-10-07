# RAFCODEΦ Capability Contract — receipt

- timestamp: 2026-09-17T19:43:00-03:00
- source_repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
- source_branch: `feat/rafcodephi-capability-contract-v3`
- parent_master: `834d79608536fc5628f54855b0bfccbcdae14d44`
- implementation_head_before_receipt: `dd4c683ef71ab191e0f8cca71068f743f2259512`
- pull_request: `#450`
- kind: `capability_contract_ui`
- state: `SOURCE_IMPLEMENTED`
- claim_allowed_build_pass: `false`
- physical_android: `TOKEN_VAZIO`

## Delta

Introduz uma camada de apresentação/prova dentro de Settings com um contrato único e seis profundidades de leitura: Leigo, Básico, Médio, Avançado, Expert e Só para Nerds.

Arquivos funcionais do delta:

- `app/src/main/res/xml/rafcodephi_capability_contract.xml`
- `app/src/main/java/com/termux/app/fragments/settings/CapabilityContractPreferencesFragment.java`
- `app/src/main/res/xml/root_preferences.xml`
- `app/src/main/java/com/termux/app/activities/SettingsActivity.java`

## Invariantes

`SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM`

`IMPLEMENTED_UNTESTED != PASS`

`TOKEN_VAZIO` é estado válido e não é promovido implicitamente a `0`, `false` ou `PASS`.

A presença de `classes*.dex` prova artefato empacotado, não cobertura de execução. A presença de `lib/*/*.so` prova artefato nativo empacotado, não `dlopen`, JNI ou símbolo chamado.

## Evidence

- PR #450 é mergeável e contém somente o delta funcional acima antes deste receipt.
- A UI faz inspeção read-only do APK em execução: package/version, SHA-256, tamanho, DEX count, native library count, ABIs empacotadas e ABIs suportadas pelo dispositivo.
- O workflow `RafCodePhi Beta Build`, run `35283946194`, chegou à preparação Android e aos gates auxiliares, mas não forneceu um `BUILD_PASS` para este head.

## Build gate / gap

O run `35283946194` foi bloqueado antes de uma compilação Gradle válida do APK por infraestrutura de bootstrap já existente:

- `scripts/run-rafcodephi-bootstrap-docker.sh`: ausente no checkout do `termux-packages` pinado;
- par `real-pkg` primário/LKG: não resolvido;
- `RAF_REAL_BOOTSTRAP_ZIP_ARM` / `RAF_REAL_BOOTSTRAP_ZIP_AARCH64`: vazios;
- perfil embutido observado pelo gate: `bridge`, enquanto o beta exige `real-pkg`.

Portanto este receipt **não** atribui a falha ao novo contrato e **não** promove o contrato a `BUILD_PASS`.

## Next gate

1. Compilar uma variante Android com a infraestrutura de bootstrap satisfeita.
2. Abrir `Settings -> RAFCODEΦ · Conheça e Comprove`.
3. Alternar os seis níveis e confirmar que a linguagem muda sem alterar a evidência-base.
4. Confirmar no APK instalado os dados de package/version, SHA-256, DEX, `.so` e ABI.
5. Emitir receipt de dispositivo antes de qualquer claim de runtime físico.

## R3

- F_ok: contrato e navegação implementados em fonte; diff revisado; PR mergeável.
- F_gap: `BUILD_PASS` e runtime físico não demonstrados neste receipt.
- F_next: build com bootstrap válido + instalação física + runtime receipt.
