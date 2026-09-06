# BOOTSTRAP_SOURCE_CONTRACT.md

> Repository: `rafaelmeloreisnovo/termux-app-rafacodephi`
> Documentation audit baseline: `b207970fc7a8630a534956cb544350cfd61ba33a`
> Complementar a `RAFCODEPHI_BOOTSTRAP_CONTRACT.md`.

Este documento define a fonte canônica dos ZIPs de bootstrap e a cadeia de custódia que separa:

1. payload local de ponte;
2. payload real source-built;
3. artefato de build observado;
4. artefato de release comprovado;
5. execução física em device.

Esses estados não são intercambiáveis.

## 1. Regra de autoridade

```text
source/test/workflow > machine-readable contract > receipt > documentation
```

`TOKEN_VAZIO` significa que a evidência ainda não foi observada. Não significa zero, falso nem PASS.

## 2. Correção canônica da auditoria Gradle

No grafo Gradle documentado não existe a task `:app:downloadBootstraps` como autoridade atual. A rota estrutural é:

```bash
./gradlew :app:generateRafcodephiBootstraps --no-daemon
```

que chama a geração local correspondente. Referências históricas a download remoto automático não descrevem o grafo corrente.

## 3. Arquivos produzidos no app

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

Esses ZIPs são artefatos de build e não devem ser tratados como fonte autoritativa por simples presença no workspace.

## 4. Fonte 1 — gerador local de ponte

Exemplo de geração estrutural:

```bash
RAFCODEPHI_REAL_PKG_BOOTSTRAP=false \
./gradlew :app:generateRafcodephiBootstraps --no-daemon
```

Classificação máxima sem backend real comprovado:

```text
BOOTSTRAP_BRIDGE_ONLY
NOT_RELEASE_RUNTIME_PROOF
```

Bridge prova wiring/empacotamento; não prova package stack real nem execução física.

## 5. Fonte 2 — candidato upstream pré-compilado

A rota upstream pré-compilada permanece conceitualmente distinta do source-build RAFCODEPHI. Binários Termux upstream compilados para:

```text
/data/data/com.termux/files/usr
```

não podem ser promovidos como equivalentes ao prefixo RAFCODEPHI:

```text
/data/data/com.termux.rafacodephi/files/usr
```

sem evidência explícita de compatibilidade/relocation. A política é fail-closed para `LEGACY_PREFIX_BINARY_RISK`.

## 6. Fonte 3 — source-build RAFCODEPHI ARM/ARM64

A fonte de payload real recompila os pacotes no fork `rafaelmeloreisnovo/termux-packages` com identidade RAFCODEPHI.

### 6.1 Contrato de identidade

```text
repository = https://github.com/rafaelmeloreisnovo/termux-packages.git
package    = com.termux.rafacodephi
prefix     = /data/data/com.termux.rafacodephi/files/usr
ABIs       = armeabi-v7a, arm64-v8a
```

### 6.2 Canais pinados

A autoridade semântica de canais vive em:

```text
data/contracts/termux-packages-rafcodephi-pin.v1.json
```

No baseline auditado:

```text
canonical = 837afec42ecf5f9ac1bd8b00e65d143bc23a380b
candidate = 0ffb24a5a6be58316236383a6d249544c39eb3e3
```

O candidato supersede o commit histórico `1fc540b0c296581c5793c109e3834589f85a0114`; PR #89 é histórico e registrado como merged. Um PR já mesclado não pode continuar descrito como candidato ativo por simples inércia documental.

Resolução reproduzível:

```bash
python3 scripts/resolve_termux_packages_pin.py canonical --json
python3 scripts/resolve_termux_packages_pin.py candidate --json
```

O resolver valida identidade, prefixo, ABI, `claim_allowed=false` e `physical_android=TOKEN_VAZIO`.

### 6.3 Rotas de workflow não devem ser colapsadas

`.github/workflows/beta-build-libllvm18-unblock.yml` usa o canal `candidate`.

`.github/workflows/beta-build.yml` mantém uma rota própria com SHA explícito/default e permite ref manual no `workflow_dispatch`.

Portanto qualquer documentação/receipt deve registrar:

```text
workflow_identity
selector_or_exact_ref
resolved_commit
```

Não usar a expressão ambígua "pin atual" sem nomear a rota.

## 7. Build source-built e produtos obrigatórios

No checkout correto de `termux-packages`:

```bash
./scripts/build-rafcodephi-real-bootstrap.sh --architectures arm,aarch64
```

O conjunto esperado é:

```text
artifacts/rafcodephi-bootstrap/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt
artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-arm.zip
artifacts/rafcodephi-bootstrap/rafcodephi-bootstrap-aarch64.zip
```

Esses três objetos formam um conjunto de evidência. Não criar manualmente o manifest para "destravar" consumidor downstream.

## 8. Preflight de capacidade da fonte

Antes do build caro, a rota `beta-build-libllvm18-unblock.yml` valida a capacidade do checkout de `termux-packages`.

Arquivos mínimos atualmente verificados:

```text
scripts/properties.sh
scripts/apply-rafcodephi-build-properties.py
scripts/validate-rafcodephi-build-properties.sh
scripts/run-docker.sh
scripts/build-rafcodephi-real-bootstrap.sh
scripts/generate-bootstraps.sh
packages/libxml2/build.sh
```

Também são verificados:

- SHA realmente checado = SHA resolvido;
- presença da closure host `libllvm18`;
- token do manifest `RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt`;
- schema `rafcodephi.real-bootstrap-sourcebuild/v1`;
- padrão `rafcodephi-bootstrap-${arch}.zip`;
- opção `--architectures`.

Falha no preflight deve encerrar cedo; ela não deve aparecer dezenas de minutos depois como "manifest missing".

## 9. Importação para o app

```bash
RAF_BOOTSTRAP_SOURCE=source-built-real \
RAF_REAL_BOOTSTRAP_ZIP_ARM=/caminho/rafcodephi-bootstrap-arm.zip \
RAF_REAL_BOOTSTRAP_ZIP_AARCH64=/caminho/rafcodephi-bootstrap-aarch64.zip \
RAF_REAL_BOOTSTRAP_MANIFEST=/caminho/RAFCODEPHI_REAL_BOOTSTRAP_MANIFEST.txt \
./scripts/prepare_bootstrap_env.sh --print-env
```

O importador/validador deve manter identidade, hashes, ABI, prefixo, ausência de bridge indevida e fronteira de claim.

## 10. Semântica do receipt

### 10.1 Caminho de sucesso

O receipt estrito `rafcodephi.usable-beta-build/v2` deve ser gerado apenas quando o pipeline anterior teve sucesso e todos os inputs existem.

### 10.2 Caminho de falha upstream

No workflow `beta-build-libllvm18-unblock.yml`, a falha anterior produz um receipt diagnóstico:

```text
state=UPSTREAM_FAILURE_EVIDENCE_INCOMPLETE
claim_allowed=false
release_allowed=false
physical_android=TOKEN_VAZIO
```

com listas de `present_evidence` e `missing_evidence`.

Regra causal obrigatória:

```text
se produtor falhou antes de produzir X,
a ausência de X é consequência;
não substituir a primeira falha por "X missing" como causa-raiz.
```

## 11. Termux API

A rota estrutural documentada usa:

```text
com.termux.rafacodephi.api/com.termux.api.TermuxApiReceiver
```

O teste `tests/test_termux_api_access_contract.py` exige permissão `signature` e ausência de `android:sharedUserId` no manifest principal. Documentação que diga que o app atual depende de `android:sharedUserId="com.termux"` é stale para o baseline auditado.

## 12. Limite atual do package repository

Preservar enquanto não houver evidência contrária atual:

```text
package_repo_runtime_state=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED
apt_update_guard=RAFCODEPHI_PACKAGE_REPOSITORY_NOT_PUBLISHED
device_runtime_proof=TOKEN_VAZIO
claim_allowed_device_runtime=false
```

Isso impede promoção documental prematura de `pkg update`/`pkg install`.

## 13. Fonte 4 — artefato de release

Um ZIP só pode receber classificação `RELEASE_BOOTSTRAP_VERIFIED` quando houver, no mínimo:

- commit do gerador;
- revisão pinada do repositório de pacotes;
- lista pinada de pacotes/versões;
- ABI;
- SHA-256;
- BLAKE3 quando exigido;
- relatório de validação;
- workflow/run que materializou o artefato;
- cadeia de custódia até o APK que o consumiu.

Sem esse conjunto:

```text
BLOCKED_BY[CANONICAL_BUILD_EVIDENCE_REQUIRED]
```

## 14. Registro de hashes

Hashes não observados permanecem `NOASSERTION`/`TOKEN_VAZIO` conforme o schema consumidor. Nunca inventar hash substituto.

## 15. Build reproduzível

A promoção exige:

1. pin do commit do app;
2. identidade da rota de workflow;
3. pin resolvido de `termux-packages`;
4. lista ordenada de pacotes/versões;
5. ambiente/toolchain registrado;
6. preflight de capacidade da fonte;
7. geração do par ARM/ARM64 + manifest;
8. validação semântica/prefix-safe;
9. hashes materializados;
10. APK/artefatos anexados;
11. receipt relacionando fonte → bootstrap → APK;
12. device proof separado para qualquer claim físico.

## 16. Estado documental atual

```text
source_contract_documented=true
pin_route_distinction_documented=true
candidate_preflight_documented=true
upstream_failure_causality_documented=true
shared_user_id_current_claim=STALE_IF_ASSERTED
custom_binary_repository=BLOCKED_CUSTOM_REPOSITORY_NOT_PUBLISHED
physical_android=TOKEN_VAZIO
claim_allowed=false
```

Auditoria relacionada:

- `docs/audits/DOCUMENTATION_CODE_ALIGNMENT_AUDIT_20260906.md`
