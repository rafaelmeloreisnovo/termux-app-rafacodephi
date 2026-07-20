# RAFCODEΦ Loader — secure acquisition handoff v2

O APK `com.termux.rafacodephi.loader` é um adquirente separado, não um
instalador do `$PREFIX`.

```text
host assinado
→ URL HTTPS + SHA-256 + ABI pinados
→ loader baixa até 128 MiB
→ SHA-256 + fsync
→ URI read-only concedida somente ao host
→ host repete SHA-256 + BLAKE3
→ política estrutural do ZIP
→ inbox privada
→ TermuxInstaller staging/rollback
```

## Autoridade

- loader: aquisição e primeira verificação;
- host: assinatura, BLAKE3, ZIP, inbox e instalação;
- `TermuxInstaller`: único publicador do prefixo.

O loader nunca recebe diretório de destino e nunca extrai arquivos.

## Build

```bash
./gradlew :app:assembleDebug :loader:materializeLoaderApk --no-daemon
```

Host e loader precisam usar o mesmo certificado. Release consome as mesmas
variáveis explícitas de keystore do host.

## Ativação externa

Todos os pares URL/SHA-256 defaultam para vazio em
`gradle/bootstrap-external.gradle`. Sem pins externos, o bootstrap embutido
permanece canônico. Quando a rota externa for ativada, o BLAKE3 canônico da ABI
também é obrigatório.

## Limites

```text
permission       = signature
cleartext        = false
redirects        = máximo 3, mesma origem
max download     = 128 MiB
provider         = private/read-only
host hashes      = SHA-256 + BLAKE3
ZIP uncompressed = até 768 MiB
native malloc    = 0
claim_allowed    = false
```

Build, artefato, testes instrumentados e `DEVICE_RECEIPT_COMPLETE` continuam
provas independentes.
