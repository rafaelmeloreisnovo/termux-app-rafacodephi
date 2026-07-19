# BOOTSTRAP_SOURCE_CONTRACT.md
<!-- Atualizado: 2026-07-19 | Complementar a RAFCODEPHI_BOOTSTRAP_CONTRACT.md -->

Este documento define a fonte canônica dos ZIPs de bootstrap, separando três estados que não podem ser confundidos:

1. payload local de ponte;
2. payload real derivado de pacotes Termux;
3. artefato de release comprovado por hash.

## Correção canônica da auditoria

No grafo Gradle atual **não existe** a task `:app:downloadBootstraps`.
A task efetivamente ligada a `preBuild` é:

```bash
./gradlew :app:generateRafcodephiBootstraps --no-daemon
```

Ela executa:

```bash
bash scripts/build_rafaelia_bootstraps.sh
```

Portanto, referências anteriores a download remoto automático eram drift documental e não descreviam o código corrente.

## Arquivos produzidos

```text
app/src/main/cpp/bootstrap-aarch64.zip
app/src/main/cpp/bootstrap-arm.zip
app/src/main/cpp/bootstrap-i686.zip
app/src/main/cpp/bootstrap-x86_64.zip
app/src/main/cpp/rewritten-bootstrap-aarch64.zip
app/src/main/cpp/rewritten-bootstrap-arm.zip
app/src/main/cpp/rewritten-bootstrap-i686.zip
app/src/main/cpp/rewritten-bootstrap-x86_64.zip
```

Esses ZIPs são artefatos de build e não devem ser tratados como fontes versionadas.

## Fonte 1 — gerador local de ponte

Estado padrão:

```bash
RAFCODEPHI_REAL_PKG_BOOTSTRAP=false \
./gradlew :app:generateRafcodephiBootstraps --no-daemon
```

O script compila `scripts/bootstrap_zip_builder.c` e gera um payload mínimo com wrappers para `sh`, `pkg`, `apt`, `apt-get`, `busybox`, `proot`, `apkmanager` e `shellbash`.

Classificação obrigatória:

```text
BOOTSTRAP_BRIDGE_ONLY
NOT_RELEASE_RUNTIME_PROOF
```

Esse modo prova estrutura e empacotamento, mas não prova backend real de `apt`, `busybox`, `proot` ou shell completo.

## Fonte 2 — pacotes Termux reais

Ativação explícita:

```bash
RAFCODEPHI_REAL_PKG_BOOTSTRAP=true \
RAFCODEPHI_REAL_PKG_ARCH=all \
RAFCODEPHI_REAL_PKG_REPO=https://packages.termux.dev/apt/termux-main \
./gradlew :app:generateRafcodephiBootstraps --no-daemon
```

Variáveis do contrato:

| variável | padrão | função |
|---|---|---|
| `RAFCODEPHI_REAL_PKG_BOOTSTRAP` | `false` | habilita payload real |
| `RAFCODEPHI_REAL_PKG_ARCH` | `all` | `all`, `aarch64` ou `arm` |
| `RAFCODEPHI_REAL_PKG_VALIDATE` | `true` | valida os ZIPs reais gerados |
| `RAFCODEPHI_REAL_PKG_REPO` | `https://packages.termux.dev/apt/termux-main` | repositório APT de origem |
| `TERMUX_BOOTSTRAP_PACKAGE_NAME` | `com.termux.rafacodephi` | package/prefix canônico |
| `TERMUX_BOOTSTRAP_PAGE_SIZE` | `16384` | contrato de page size |

A URL acima é fonte de pacotes, não um endpoint de ZIP pronto. A geração continua sendo local e deve registrar versões e hashes dos pacotes consumidos.

## Fonte 3 — artefato de release

Um ZIP só pode receber classificação `RELEASE_BOOTSTRAP_VERIFIED` quando houver:

- commit do gerador;
- URL/revisão do repositório de pacotes;
- lista pinada de pacotes e versões;
- ABI;
- SHA-256;
- BLAKE3, quando exigido pelo runtime;
- relatório de validação;
- workflow/run que materializou o artefato.

Sem esse conjunto, use `BLOCKED_BY[CANONICAL_BUILD_EVIDENCE_REQUIRED]`.

## Verificação

```bash
sha256sum app/src/main/cpp/bootstrap-*.zip
b3sum app/src/main/cpp/bootstrap-*.zip
python3 scripts/validate_real_arm_bootstrap_core.py \
  app/src/main/cpp/rewritten-bootstrap-aarch64.zip \
  app/src/main/cpp/rewritten-bootstrap-arm.zip
```

## Registro de hashes

| ABI | SHA-256 | BLAKE3 | origem | estado |
|---|---|---|---|---|
| aarch64 | `NOASSERTION` | `NOASSERTION` | aguardando build canônico | `BLOCKED_BY[CI_ARTIFACT]` |
| arm | `NOASSERTION` | `NOASSERTION` | aguardando build canônico | `BLOCKED_BY[CI_ARTIFACT]` |
| i686 | `NOASSERTION` | `NOASSERTION` | gerador de ponte somente | `DEV_ONLY` |
| x86_64 | `NOASSERTION` | `NOASSERTION` | gerador de ponte somente | `DEV_ONLY` |

`NOASSERTION` aqui significa ausência declarada de artefato canônico; não é um hash substituto.

## Build reproduzível

A promoção exige:

1. pin do commit deste repositório;
2. pin do snapshot/revisão de `termux-packages`;
3. lista ordenada de pacotes e versões;
4. ambiente/toolchain registrado;
5. geração dos quatro ZIPs;
6. validação do conteúdo e prefixos;
7. SHA-256/BLAKE3 materializados;
8. upload imutável ou release asset;
9. ledger relacionando artefato, commit e workflow.

## Estado atual

```text
source_contract_documented = true
legacy_download_task_claim = removed
default_payload = BOOTSTRAP_BRIDGE_ONLY
real_arm_payload_path = implemented_but_unproven_on_current_head
release_hashes = BLOCKED_BY[CANONICAL_BUILD_EVIDENCE_REQUIRED]
```
