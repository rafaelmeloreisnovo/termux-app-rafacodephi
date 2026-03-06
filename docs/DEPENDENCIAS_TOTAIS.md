# Dependências Totais e Ligações com Outros Repositórios

Este documento consolida **todas as dependências** declaradas nos arquivos de build do projeto e as **ligações explícitas com outros repositórios** referenciados pela aplicação, mantendo rastreabilidade total da arquitetura e do ecossistema.

---

## 1. Repositórios de Dependências (Gradle)

**Repositórios configurados no nível raiz**:

```gradle
allprojects {
    repositories {
        google()
        mavenCentral()
        maven { url "https://jitpack.io" }
    }
}
```

**Buildscript (plugin Gradle Android)**:

```gradle
buildscript {
    repositories {
        mavenCentral()
        google()
    }
    dependencies {
        classpath "com.android.tools.build:gradle:8.13.2"
    }
}
```

---

## 2. Módulos Incluídos no Projeto

**Definição de módulos (settings.gradle)**:

```gradle
include ':app', ':termux-shared', ':terminal-emulator', ':terminal-view', ':rafaelia', ':rmr'
```

---

## 3. Dependências do Módulo `app`

**Dependências de implementação**:

```gradle
implementation "androidx.annotation:annotation:1.9.0"
implementation "androidx.core:core:1.13.1"
implementation "androidx.drawerlayout:drawerlayout:1.2.0"
implementation "androidx.preference:preference:1.2.1"
implementation "androidx.viewpager:viewpager:1.0.0"
implementation "com.google.android.material:material:1.12.0"
implementation "com.google.guava:guava:24.1-jre"
implementation "io.noties.markwon:core:$markwonVersion"
implementation "io.noties.markwon:ext-strikethrough:$markwonVersion"
implementation "io.noties.markwon:linkify:$markwonVersion"
implementation "io.noties.markwon:recycler:$markwonVersion"
implementation 'com.google.guava:listenablefuture:9999.0-empty-to-avoid-conflict-with-guava'

implementation project(":terminal-view")
implementation project(":termux-shared")
implementation project(":rafaelia")
implementation project(":rmr")
```

**Dependências de teste e desugaring**:

```gradle
testImplementation "junit:junit:4.13.2"
testImplementation "org.robolectric:robolectric:4.10"
coreLibraryDesugaring "com.android.tools:desugar_jdk_libs:2.1.2"
```

**Dependência externa de bootstrap (Termux packages)**:

O build exige bootstraps locais (em `app/src/main/cpp/bootstrap-<arch>.zip`) gerados para
`com.termux.rafacodephi` e com `BOOTSTRAP_INFO` (incluindo `TERMUX_PACKAGE_NAME` e `TERMUX_PAGE_SIZE=16384`).
O download automático não é executado durante o build para permitir builds offline e reprodutíveis.

Quando necessário, o download pode ser feito manualmente apontando `TERMUX_BOOTSTRAP_BASE_URL` para
um repositório que hospede os bootstraps corretos.

---

## 4. Dependências do Módulo `termux-shared`

**Dependências de implementação**:

```gradle
implementation "androidx.appcompat:appcompat:1.6.1"
implementation "androidx.annotation:annotation:1.9.0"
implementation "androidx.core:core:1.13.1"
implementation "com.google.android.material:material:1.12.0"
implementation "com.google.guava:guava:24.1-jre"
implementation "io.noties.markwon:core:$markwonVersion"
implementation "io.noties.markwon:ext-strikethrough:$markwonVersion"
implementation "io.noties.markwon:linkify:$markwonVersion"
implementation "io.noties.markwon:recycler:$markwonVersion"
implementation "org.lsposed.hiddenapibypass:hiddenapibypass:6.1"
implementation "androidx.window:window:1.1.0"
implementation "commons-io:commons-io:2.5"
implementation project(":terminal-view")
implementation "com.termux:termux-am-library:v2.0.0"
```

**Dependências de teste e desugaring**:

```gradle
testImplementation "junit:junit:4.13.2"
androidTestImplementation "androidx.test.ext:junit:1.1.5"
coreLibraryDesugaring "com.android.tools:desugar_jdk_libs:2.1.2"
```

---

## 5. Dependências do Módulo `terminal-view`

**Dependências de implementação**:

```gradle
implementation "androidx.annotation:annotation:1.9.0"
api project(":terminal-emulator")
```

**Dependências de teste**:

```gradle
testImplementation "junit:junit:4.13.2"
```

---

## 6. Dependências do Módulo `terminal-emulator`

**Dependências de implementação**:

```gradle
implementation "androidx.annotation:annotation:1.9.0"
```

**Dependências de teste**:

```gradle
testImplementation "junit:junit:4.13.2"
```

---

## 7. Dependências do Módulo `rafaelia`

**Dependências de teste**:

```gradle
testImplementation "junit:junit:4.13.2"
```

---

## 8. Dependências do Módulo `rmr`

**Dependências de teste**:

```gradle
testImplementation "junit:junit:4.13.2"
```

---

## 9. Ligações com Outros Repositórios (Ecossistema Termux)

Os repositórios externos **explicitamente referenciados** pela documentação principal do projeto incluem:

- **Termux App (upstream)**: https://github.com/termux/termux-app
- **Termux Packages (bootstrap/pacotes)**: `qemu_rafaelia/termux-packages-README.md`
- **Termux:API**: `androidx_rmr/termux-api-README.md`
- **Termux:Boot**: `androidx_rmr/termux-boot-README.md`
- **Termux:Float**: https://github.com/termux/termux-float
- **Termux:Styling**: https://github.com/termux/termux-styling
- **Termux:Tasker**: https://github.com/termux/termux-tasker
- **Termux:Widget**: https://github.com/termux/termux-widget

---

## 10. Arquivos com Referências aos Repositórios

Arquivos no repositório que **contêm referências explícitas** aos links acima:

- `ASSINATURA_AUTORIA.md`
- `CONTRIBUTORS.md`
- `DIFERENCAS_FORK.md`
- `DOCUMENTACAO.md`
- `IMPLEMENTACAO_BAREMETAL.md`
- `IMPLEMENTATION_COMPLETE.md`
- `LICENSE.md`
- `LEIA-ME-DOCUMENTACAO.md`
- `README.md`
- `SECURITY.md`
- `docs/en/index.md`
- `docs/LOWLEVEL_MIGRATION.md`
- `docs/LOWLEVEL_SUMMARY.md`

---

## 11. Diretriz de Integração (Low-Level)

Integrações adicionais devem respeitar o perfil **low-level** do projeto:

- Preferência por código nativo C/ASM e chamadas diretas, evitando camadas de abstração desnecessárias.
- Estruturas e variáveis tratadas como **matrizes/arrays**, mantendo organização determinística e previsível.
- Dependências externas devem ser justificadas e registradas neste inventário.

---

## 12. Conclusão

Esta consolidação garante **rastreabilidade total** das dependências internas (módulos Gradle), externas (artefatos Maven/Jitpack) e das ligações explícitas com outros repositórios essenciais ao ecossistema Termux.

---

## 13. Verificação Operacional (2026-03)

Validação executada localmente para garantir que o inventário de dependências está versionado no repositório e consistente com os `build.gradle`:

```bash
rg -n "^(\s*)(implementation|api|testImplementation|androidTestImplementation|coreLibraryDesugaring|classpath)\s" --glob "**/build.gradle"
```

Resultado: **OK** (dependências encontradas em `build.gradle`, `app/build.gradle`, `termux-shared/build.gradle`, `terminal-view/build.gradle`, `terminal-emulator/build.gradle`, `rafaelia/build.gradle` e `rmr/build.gradle`).

### Observação de ambiente

A execução de tasks Gradle que exigem Android SDK falha sem `local.properties`/`ANDROID_HOME` configurado, portanto a verificação desta rodada foi feita por inspeção estática dos arquivos versionados.

