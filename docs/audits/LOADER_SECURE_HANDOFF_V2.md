# Loader Secure Handoff v2 — arquitetura, segurança e evidência

## Estado epistemológico

```text
implementation_state = IMPLEMENTED_SECURITY_GATED
build_state          = TOKEN_VAZIO até execução observável
artifact_state       = TOKEN_VAZIO até APKs e hashes
runtime_state        = TOKEN_VAZIO até aparelho
claim_allowed        = false
```

Este documento descreve a fronteira implementada; não declara que o workflow,
o APK release ou o aparelho já foram comprovados.

## Invariante de autoridade

```text
loader APK → aquisição e SHA-256
host APK   → identidade, BLAKE3, ZIP policy, inbox e instalação
```

O loader nunca recebe um diretório de destino e nunca toca no `$PREFIX`.

## Fluxo

```text
BootstrapGateActivity
├─ prefix pronto → TermuxActivity
├─ pins externos vazios → TermuxActivity → payload embutido
└─ URL+SHA+BLAKE3 pinados
   → assinatura do loader + metadata contract v2
   → LoaderActivity protegida por permission signature
   → BootstrapInstallService
   → HTTPS/443, sem user-info ou fragmento
   → até 3 redirects da mesma origem
   → até 128 MiB
   → SHA-256 + fsync + rename privado
   → VerifiedBootstrapProvider read-only
   → broadcast explícito ao host
   → BootstrapHandoffReceiver
   → SHA-256 novamente
   → BLAKE3 canônico da ABI
   → política estrutural do ZIP
   → receipt + ZIP em inbox privada
   → TermuxActivity
   → JNI lê inbox com O_NOFOLLOW e sem malloc
   → TermuxInstaller staging/validação/rollback
```

## Decisões de segurança

### Permissão

```text
com.termux.rafacodephi.permission.BOOTSTRAP_HANDOFF
protectionLevel = signature
```

O host verifica ainda `PackageManager.checkSignatures()` e metadata:

```text
CONTRACT_VERSION >= 2
CONTRACT_STATE = BOOTSTRAP_ACQUIRE_HANDOFF_CAPABLE
```

### Origem

A URL é fornecida pelo build do host, não pelo loader e não por default.

Bloqueios:

- HTTP;
- porta diferente de 443;
- user-info;
- fragmento;
- redirects cross-origin;
- mais de três redirects;
- URL sem SHA-256 correspondente.

### Download

```text
MAX_DOWNLOAD_BYTES = 128 MiB
Content-Length      = verificado quando disponível
stream total        = limitado
SHA-256             = calculado durante a gravação
fsync                = obrigatório
```

### Provider

- `exported=false`;
- URI temporária concedida somente ao pacote host;
- nome limitado a ABI + SHA-256;
- canonical path sob `files/verified`;
- `openFile` aceita somente modo `r`;
- insert/update/delete são proibidos.

### Host

O host repete SHA-256 e exige BLAKE3 canônico real. O bypass debug do payload
embutido é rejeitado para qualquer handoff externo.

A política ZIP v1 limita:

```text
entradas              <= 65.536
entrada individual    <= 256 MiB
soma descompactada    <= 768 MiB
razão de compressão   <= 500
nomes                 relativos, sem backslash/NUL/..
duplicatas            proibidas
SYMLINKS.txt           obrigatório
```

Esses limites são convenções defensivas e podem ser reduzidos depois de medir
os quatro bootstraps canônicos. Eles não são uma alegação sobre o tamanho
necessário de qualquer distribuição externa.

### Inbox e JNI

Arquivos:

```text
files/bootstrap-inbox/bootstrap-external.zip
files/bootstrap-inbox/bootstrap-external.receipt.json
```

O JNI:

- deriva o package name de `/proc/self/cmdline`;
- usa `O_NOFOLLOW | O_CLOEXEC`;
- exige arquivo regular;
- limita a 128 MiB;
- copia em buffer de 8 KiB;
- não usa `malloc`, `calloc` ou `realloc`;
- usa o payload embutido somente quando a inbox não existe (`ENOENT`);
- qualquer outro erro retorna bytes vazios e força falha BLAKE3.

## Build pins

Todos defaultam para string vazia:

```text
TERMUX_EXTERNAL_BOOTSTRAP_URL_*
TERMUX_EXTERNAL_BOOTSTRAP_SHA256_*
```

Assim, fonte desconhecida não vira uma URL inventada. Para ativação externa,
o BLAKE3 canônico `TERMUX_BOOTSTRAP_BLAKE3_*` também precisa existir.

## Testes e gates

```sh
python3 -m unittest tests/test_loader_secure_handoff_contract.py -v
python3 tools/validate_loader_secure_handoff.py \
  --write-report dist/loader/loader-secure-handoff-validation.json

./gradlew :app:assembleDebug :loader:materializeLoaderApk --no-daemon

bash scripts/verify_loader_apk.sh \
  dist/loader/loader.apk \
  dist/loader/loader-contract.txt \
  dist/loader/host-debug.apk
```

O workflow existente `.github/workflows/loader-apk-contract.yml` executa a
cadeia e compara certificados do host e loader.

## Provas ainda abertas

1. workflow com steps e logs;
2. APKs produzidos e hashes associados ao commit final;
3. certificados iguais observados por `apksigner`;
4. teste instrumentado de app não autorizado;
5. teste instrumentado de URI grant e revogação;
6. teste de rede com redirects e download truncado;
7. quatro arquivos canônicos medidos contra os ZIP budgets;
8. URL/SHA/BLAKE3 reais, quando a rota externa for ativada;
9. instalação e rollback no Moto E7 ARM32 e Realme ARM64;
10. `DEVICE_RECEIPT_COMPLETE`.

## Rollback

- sem pins externos: remover/ignorar loader mantém payload embutido;
- falha no download: nenhum arquivo host é alterado;
- falha no host: `.part` é removido e `$PREFIX` não é tocado;
- falha no TermuxInstaller: staging é removido e o prefixo anterior permanece;
- inbox incompatível com nova versão: receipt/ZIP são descartados antes do uso.

## R3

```text
F_ok   = fronteira assinatura→HTTPS→SHA→BLAKE3→ZIP→staging implementada
F_gap  = build remoto, testes instrumentados, pins reais e aparelho
F_next = executar workflow e fechar evidência antes de tirar PR #290 de draft
```
