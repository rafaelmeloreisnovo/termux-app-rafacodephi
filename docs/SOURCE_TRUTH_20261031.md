# RAFCODEΦ Android — estado verificável e roteiro documental (31/10/2026)

> **STATUS: DRAFT/HOLD · documentação, não release.** Corte de leitura: `rafaelmeloreisnovo/termux-app-rafacodephi@f2d3900a8aee8dcef71f8e2df3f470ea4bc34632` (`master` no início deste trabalho). Os SHAs abaixo são objetos de **fonte**, não comprovantes de execução. `claim_allowed=false`; `release_allowed=false`.

## Autoridade e trilha curta

Este repositório produz a **aplicação Android**; a fábrica de receitas, pacotes e bootstrap real é [`rafaelmeloreisnovo/termux-packagesMy`](https://github.com/rafaelmeloreisnovo/termux-packagesMy). `termux-packages-rafcode` é um experimento freestanding separado, não substitui `pkg/apt`. Contratos locais: [`AGENTS.md`](../AGENTS.md), [`app/build.gradle`](../app/build.gradle), [`gradle.properties`](../gradle.properties), [`AndroidManifest.xml`](../app/src/main/AndroidManifest.xml), [`RAFCODEPHI_BOOTSTRAP_CONTRACT.md`](../RAFCODEPHI_BOOTSTRAP_CONTRACT.md) e [`scripts/verify_bootstrap_contract.sh`](../scripts/verify_bootstrap_contract.sh).

## Configuração observada no código

| Propriedade | Valor configurado | Fonte |
| --- | --- | --- |
| `applicationId` padrão | `com.termux.rafacodephi` (override por variável de ambiente disponível) | `app/build.gradle` |
| `compileSdkVersion` / `minSdkVersion` / `targetSdkVersion` | `35 / 21 / 28` | `gradle.properties` |
| `ndkVersion` configurado | `26.3.11579264` | `gradle.properties` |
| ABIs obrigatórias / APK universal | `armeabi-v7a,arm64-v8a` / `true` | `gradle.properties` e `app/build.gradle` |
| `versionCode` / `versionName` padrão | `118` / `0.118.0-rafacodephi` | `app/build.gradle`; **não** prova APK instalado nem release |
| Gradle Android plugin declarado | `8.13.2` | `build.gradle` |
| Ponte nativa / worker periódico | `rafaelia.nativeBridge.enabled=false` / `rafaelia.workRuntime.enabled=false` por padrão | `gradle.properties` |

O `AndroidManifest.xml` registra `TermuxActivity` e `BetaOrchestratorActivity`; registro estático não comprova que o APK correspondente foi compilado, instalado e aberto no dispositivo. Relatórios de crash ou testes de agosto permanecem históricos até nova observação pareada.

## Bootstrap: três fronteiras diferentes

1. **Arquivos fonte:** `app/src/main/cpp/bootstrap-{arm,aarch64,i686,x86_64}.zip`.
2. **Arquivos embutidos no build:** `app/src/main/cpp/rewritten-bootstrap-{arm,aarch64,i686,x86_64}.zip`. Os dois conjuntos são distintos em `scripts/verify_bootstrap_contract.sh`; `app/build.gradle` referencia o conjunto *rewritten*.
3. **Payload real de pacotes:** o produtor `termux-packagesMy/scripts/build-rafcodephi-real-bootstrap.sh` declara somente `arm` e `aarch64` para seu primeiro gate source-built. Existir ZIP de compatibilidade i686/x86_64 no contrato do app **não** demonstra que a fábrica real produziu essas arquiteturas.

`RAF_BOOTSTRAP_SOURCE=local` / `--prepare` pode preparar material para **build**, sem certificar `pkg/apt`. `source-built-real` exige ZIPs reais e manifesto rastreável. O próprio gerador declara `package_repo_runtime_state=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED`: não orientar `pkg update` ou instalação como funcional enquanto faltarem repositório assinado publicado e provas no dispositivo.

## Reexecução controlada (não executada nesta atualização)

```sh
# Somente checagem estrutural com os artefatos já presentes:
bash scripts/verify_bootstrap_contract.sh --check

# Caminho de build com toolchain Android compatível e bootstrap preparado:
./gradlew :app:assembleDebug --no-daemon
```

Estes comandos são **procedimentos**, não receipts de sucesso. Para promover o estado: registrar commit exato, bootstrap ZIP + manifesto + SHA-256, APK + SHA-256, saída do CI, instalação/primeira abertura, ABI real, shell e execução de `pkg/apt/dpkg/proot` no Android correspondente. Separar teste ARM32 de ARM64; exigir teste negativo de arquivo ausente/hash divergente. Para assinaturas de release, exigir receipt próprio, não deduzir de um build debug.

## Divergências documentais e decisão

- [`docs/STATUS.md`](STATUS.md), [`docs/RUNTIME_TRUTH_TABLE.md`](RUNTIME_TRUTH_TABLE.md) e [`INDICE_DOCUMENTACAO.md`](../INDICE_DOCUMENTACAO.md) incluem baselines antigos: **predecessores**, nunca prova automática do HEAD.
- [`RAFCODEPHI_BOOTSTRAP_CONTRACT.md`](../RAFCODEPHI_BOOTSTRAP_CONTRACT.md) lista ZIPs fonte; consulte o script e o Gradle para distinguir os ZIPs **embutidos**.
- Links upstream e badges upstream no README pertencem ao projeto Termux original: não representam CI da fork. Preservar notices/licenças upstream.

**F_ok:** identidade do produtor, configuração e caminhos de código conferidos por leitura de fonte. **F_gap:** CI exata, integridade do artefato gerado, `pkg/apt` real e instalação/execução física ARM32/ARM64 = `TOKEN_VAZIO_NOT_RUN`. **F_next:** verificar um APK debug com bootstrap source-built pinado e executar smoke test e falsificadores em um dispositivo autorizado. **Rollback:** fechar o PR documental sem merge. Nenhum arquivo de código, package recipe ou licença foi alterado nesta proposta.
