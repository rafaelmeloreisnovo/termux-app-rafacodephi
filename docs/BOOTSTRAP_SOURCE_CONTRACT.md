# BOOTSTRAP_SOURCE_CONTRACT.md
<!-- Criado: 2026-07-19 | Complementar a RAFCODEPHI_BOOTSTRAP_CONTRACT.md -->

Este documento especifica a **fonte canônica** dos ZIPs de bootstrap e os procedimentos
de verificação de integridade. Complementa `RAFCODEPHI_BOOTSTRAP_CONTRACT.md`, que
descreve o *conteúdo esperado* do bootstrap.

---

## O Problema

`./gradlew :app:downloadBootstraps` baixa ZIPs de bootstrap de um servidor remoto,
mas a **URL de origem e os hashes esperados** não estavam documentados explicitamente,
criando o estado `BOOTSTRAP_UNVERIFIABLE` — ninguém pode confirmar de onde vieram os ZIPs
sem inspecionar o código Gradle.

Este documento resolve isso.

---

## Arquivos Bootstrap Esperados

```text
app/src/main/cpp/bootstrap-aarch64.zip
app/src/main/cpp/bootstrap-arm.zip
app/src/main/cpp/bootstrap-i686.zip
app/src/main/cpp/bootstrap-x86_64.zip
```

Esses arquivos **não são versionados no Git** por design (binários grandes + rebuild frequente).
São gerados ou baixados durante o build.

---

## Fontes Canônicas e Prioridade

### Fonte 1 — Variáveis de ambiente com BLAKE3 (fonte primária, release-grade)

Configure as variáveis antes de executar o build:

```bash
export TERMUX_BOOTSTRAP_BLAKE3_AARCH64="<blake3-hash-do-zip-aarch64>"
export TERMUX_BOOTSTRAP_BLAKE3_ARM="<blake3-hash-do-zip-arm>"
export TERMUX_BOOTSTRAP_BLAKE3_I686="<blake3-hash-do-zip-i686>"
export TERMUX_BOOTSTRAP_BLAKE3_X86_64="<blake3-hash-do-zip-x86_64>"
```

O build verifica que os ZIPs baixados coincidem com os hashes declarados via
`BootstrapBaremetalGuard`. Release builds **falham** se essas variáveis estiverem vazias.

### Fonte 2 — Download via Gradle task (desenvolvimento)

```bash
./gradlew :app:downloadBootstraps --no-daemon
```

A URL de download é definida na task `downloadBootstraps` em `app/build.gradle.kts`.
**Ação necessária:** Documentar a URL explícita aqui após confirmação de qual
servidor é usado (Termux packages server, GitHub Releases, ou build próprio).

### Fonte 3 — Bootstrap local de desenvolvimento (sem download remoto)

```bash
bash scripts/verify_bootstrap_contract.sh --prepare-dev
```

Gera `bootstrap-*.zip` mínimos localmente via `bootstrap_zip_builder.c` com marcador
`BUILD_ONLY` — não apto para release ou teste de runtime.

---

## Verificação de Integridade

### SHA-256 (mínimo obrigatório)

```bash
sha256sum app/src/main/cpp/bootstrap-*.zip
```

Os valores SHA-256 devem ser registrados após cada build canônico que produza
ZIPs verificados. Estado atual: `NOASSERTION` — preencher após primeira execução
de CI bem-sucedida com fonte 1 ou 2.

| ABI       | SHA-256                 | Status       |
|-----------|-------------------------|--------------|
| aarch64   | NOASSERTION             | PENDING      |
| arm       | NOASSERTION             | PENDING      |
| i686      | NOASSERTION             | PENDING      |
| x86_64    | NOASSERTION             | PENDING      |

### BLAKE3 (obrigatório para release)

BLAKE3 é o mecanismo de verificação usado pela `BootstrapBaremetalGuard` em
runtime. Requer `b3sum` no ambiente de build:

```bash
b3sum app/src/main/cpp/bootstrap-*.zip
```

---

## Registro de Dependências do Bootstrap

Os ZIPs devem conter, no mínimo:

| Arquivo              | Propósito                    | Verificado por              |
|----------------------|------------------------------|-----------------------------|
| `SYMLINKS.txt`       | Links simbólicos do bootstrap| `BootstrapBaremetalGuard`   |
| `bin/sh`             | Shell mínimo                 | runtime contract check      |
| `bin/pkg`            | Gerenciador de pacotes       | runtime contract check      |
| `bin/busybox`        | Utilitários Unix             | runtime contract check      |
| `bin/proot`          | Emulação de chroot           | runtime contract check      |
| `BOOTSTRAP_INFO`     | Metadados de build           | contract validator          |

---

## Build Reproduzível

Para garantir reprodutibilidade:

1. Fixar a versão do toolchain termux-packages usado para compilar os ZIPs
2. Usar commits pinados de `termux-packages`
3. Registrar o SHA-256 + BLAKE3 de cada ZIP como artefato de CI
4. Armazenar os ZIPs como GitHub Release assets ou em armazenamento de artefatos
   com retenção de no mínimo 90 dias

---

## Gaps Pendentes (BLOCKED_BY_AUDIT)

- [ ] Documentar URL exata usada por `downloadBootstraps` em `app/build.gradle.kts`
- [ ] Preencher SHA-256 e BLAKE3 da tabela após primeiro CI build bem-sucedido
- [ ] Confirmar versão do termux-packages usada para gerar os ZIPs
- [ ] Decidir entre: servidor próprio, GitHub Releases ou artefato de CI como fonte primária

Até esses itens serem preenchidos, o estado do bootstrap é:
`BOOTSTRAP_DEV_LOCAL_ONLY` (conforme `docs/BETA_BOOTSTRAP_STATUS.md`)
