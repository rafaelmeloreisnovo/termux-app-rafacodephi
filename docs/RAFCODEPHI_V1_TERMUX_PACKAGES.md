# RAFCODEPHI V1 — integração canônica com `termux-packages`

Estado: `V1_BUILD_PIPELINE_IMPLEMENTED / DEVICE_RUNTIME=TOKEN_VAZIO / claim_allowed=false`

## Objetivo

A V1 liga o aplicativo `com.termux.rafacodephi` ao fork `rafaelmeloreisnovo/termux-packages` por uma cadeia única e verificável:

```text
termux-packages@SHA
  -> properties.sh com package/prefix RAFCODEPHI
  -> source-build real ARM + AArch64
  -> manifesto + SHA-256
  -> importador fail-closed do app
  -> rewritten-bootstrap-arm.zip + rewritten-bootstrap-aarch64.zip
  -> hashes BLAKE3/SHA-256 do payload realmente embutido
  -> assembleDebug
  -> APKs + receipt cruzando os dois commits
```

## Autoridade V1 congelada

- app: `rafaelmeloreisnovo/termux-app-rafacodephi`
- packages: `rafaelmeloreisnovo/termux-packages`
- pin atual: `7b59383c25f7557ba8a29a24f715c5fb5b26cc53`
- origem do pin: merge do `termux-packages` PR #75
- evidência do pin: grafo completo ARM32 `PASS`, ARM64 `PASS`, agregador `PASS`, Phase 9.15 `PASS`
- package: `com.termux.rafacodephi`
- prefix: `/data/data/com.termux.rafacodephi/files/usr`
- arquiteturas: `arm` e `aarch64`
- workflow: `.github/workflows/rafcodephi-v1-termux-packages.yml`

Em pull requests, o pin é obrigatório. Em execução manual, outro ref pode ser informado explicitamente; o SHA efetivamente consumido entra no receipt.

## O que o bootstrap V1 inclui

O artefato consumido é produzido por `scripts/build-rafcodephi-real-bootstrap.sh` no repositório de packages e exige, nas duas arquiteturas, payload real de `bash`, `pkg`, `apt`, `apt-get`, `dpkg`, `busybox`, `proot` e cliente `termux-api`, além do perfil RAFCODEPHI. Bridge e prefixo legado são rejeitados.

O app não aceita apenas o nome do artefato: valida manifesto, SHA-256, arquitetura ELF, package, prefixo, entradas obrigatórias, rota da API e ausência do prefixo `/data/data/com.termux/files/usr` no caminho crítico.

## Evidência de portabilidade do pin atual

O gate `RAFCODEPHI Full ARM Cross Graph` foi corrigido para instalar apenas o toolchain correspondente a cada lane. Antes, cada lane instalava simultaneamente os toolchains ARM32 e AArch64; a ARM32 atingia timeout durante a instalação. Após a correção:

- ARM32: toolchain específico `PASS`; grafo completo `PASS`; ELF/receipt `PASS`;
- ARM64: toolchain específico `PASS`; grafo completo `PASS`; ELF/receipt `PASS`;
- transit composto: `PASS`;
- `PHYSICAL_ANDROID` continua `TOKEN_VAZIO`.

Essa evidência é de cross-build Linux e não substitui execução Android/Bionic física.

## Limite epistêmico da V1

Esta integração **não** converte evidência Linux/QEMU D5-D9 em prova Android/Bionic. Também não publica um repositório APT externo.

Enquanto não existir um repositório binário RAFCODEPHI compatível com o prefixo customizado, o bootstrap preserva:

```text
package_repo_runtime_state=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED
apt_update_guard=RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED
device_runtime_proof=TOKEN_VAZIO
claim_allowed_device_runtime=false
```

Portanto:

- shell e package-manager **estão materialmente embutidos no APK candidato** quando o workflow fecha;
- `apt update`/`pkg install` pela rede continuam fail-closed por projeto;
- “funciona no aparelho físico” só pode virar `PASS` após instalação e smoke real no Android, com receipt D10 ou equivalente.

## Gates

1. `TERMUX_PACKAGES_PIN` resolve para o commit esperado.
2. overlay de `scripts/properties.sh` é aplicado e validado.
3. source-build ARM/AArch64 fecha sem bridge.
4. manifesto e ZIPs existem e possuem hashes.
5. app importa o par completo antes do Gradle.
6. testes focados do importador passam.
7. `assembleDebug` produz APKs ARM32 e ARM64.
8. receipt `rafcodephi.v1-termux-packages-to-apk/v1` ancora commits e hashes.
9. estado físico permanece `TOKEN_VAZIO` até medição real.

## Promoção posterior

A próxima versão funcional de rede deve construir/publicar pacotes Android/Bionic no prefixo RAFCODEPHI, assinar metadados APT com raiz de confiança persistente e só então remover o `apt_update_guard`. Pacotes Linux/glibc usados nos gates QEMU não podem ser promovidos diretamente para o Android.

---

**Invariante:** `fonte != build != APK != instalação != runtime != claim`.
