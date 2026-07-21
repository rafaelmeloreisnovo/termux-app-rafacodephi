# RAFCODEΦ loader APK — secure acquisition handoff v2

O módulo `loader` produz um APK funcional e separado, mas **não instala nem
extrai diretamente o `$PREFIX`**.

Sua única responsabilidade é:

```text
pedido do host assinado
→ validação ABI + URL HTTPS + SHA-256
→ download privado e limitado
→ SHA-256
→ URI read-only concedida somente ao host
→ recibo explícito protegido por permission signature
```

O host `com.termux.rafacodephi` continua sendo o único responsável por:

```text
BLAKE3 canônico
→ inspeção estrutural do ZIP
→ inbox privada
→ staging
→ binários obrigatórios
→ publicação atômica do prefixo
→ rollback
```

## Build

```bash
./gradlew :app:assembleDebug :loader:materializeLoaderApk --no-daemon
```

Saídas esperadas:

```text
app/loader/build/dist/loader.apk
app/build/outputs/apk/**/termux-rafcodephi-debug-*.apk
```

O host e o loader debug usam a mesma chave debug. Em release, ambos consomem
as mesmas variáveis explícitas:

```text
TERMUX_ENABLE_RELEASE_SIGNING
TERMUX_RELEASE_KEYSTORE_FILE
TERMUX_RELEASE_KEYSTORE_PASSWORD
TERMUX_RELEASE_KEY_ALIAS
TERMUX_RELEASE_KEY_PASSWORD
```

Um certificado diferente é rejeitado pelo host e pelo gate de APK.

## Pins externos

A rota externa vem **desativada por padrão**. Para uma ABI, URL e SHA-256
precisam existir juntas:

```text
TERMUX_EXTERNAL_BOOTSTRAP_URL_AARCH64
TERMUX_EXTERNAL_BOOTSTRAP_SHA256_AARCH64
TERMUX_EXTERNAL_BOOTSTRAP_URL_ARM
TERMUX_EXTERNAL_BOOTSTRAP_SHA256_ARM
TERMUX_EXTERNAL_BOOTSTRAP_URL_I686
TERMUX_EXTERNAL_BOOTSTRAP_SHA256_I686
TERMUX_EXTERNAL_BOOTSTRAP_URL_X86_64
TERMUX_EXTERNAL_BOOTSTRAP_SHA256_X86_64
```

Além disso, o BLAKE3 canônico correspondente continua obrigatório:

```text
TERMUX_BOOTSTRAP_BLAKE3_AARCH64
TERMUX_BOOTSTRAP_BLAKE3_ARM
TERMUX_BOOTSTRAP_BLAKE3_I686
TERMUX_BOOTSTRAP_BLAKE3_X86_64
```

Sem esses pins, o launcher utiliza o bootstrap embutido. Par incompleto,
HTTP, porta diferente de 443, user-info, fragmento ou BLAKE3 ausente falham
fechado.

## Segurança

```text
permission        = signature
cleartext         = false
redirects         = máximo 3, mesma origem
max download      = 128 MiB
provider          = private + read-only URI grant
loader extraction = proibida
host hashes       = SHA-256 + BLAKE3
ZIP entries       = limitadas e únicas
ZIP expansion     = limitada
native malloc     = 0
fallback embedded = somente quando inbox externa não existe
```

## Estado

```text
module_present       = true
apk_producer         = true
has_code             = true
bootstrap_payload    = acquired_only_when_pinned
installer_behavior   = host_only
security_contract    = IMPLEMENTED_SECURITY_GATED
build_evidence       = TOKEN_VAZIO até workflow executar
instrumented_device  = TOKEN_VAZIO
release_allowed      = false
claim_allowed        = false
```

A promoção exige build observável, certificados iguais, testes instrumentados
de chamador não autorizado, pins reais e `DEVICE_RECEIPT_COMPLETE` no aparelho
alvo.
