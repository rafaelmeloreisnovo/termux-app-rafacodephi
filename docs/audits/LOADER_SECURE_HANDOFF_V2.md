# Loader Secure Handoff v2

## Estado

```text
implementation = IMPLEMENTED_SECURITY_GATED
build          = TOKEN_VAZIO até workflow com steps
artifact       = TOKEN_VAZIO até APKs e hashes
runtime        = TOKEN_VAZIO até aparelho
claim_allowed  = false
```

## Cadeia

```text
BootstrapGateActivity
→ pins externos ausentes: bootstrap embutido
→ pins URL+SHA+BLAKE3 presentes:
  assinatura e metadata v2 do loader
  → HTTPS/443, redirects da mesma origem
  → download privado limitado
  → SHA-256
  → URI read-only
  → host SHA-256 + BLAKE3
  → ZIP budgets e nomes seguros
  → inbox privada atômica
  → JNI O_NOFOLLOW sem malloc
  → TermuxInstaller staging, validação e rollback
```

## Fronteiras

- permissão `signature` e certificados iguais;
- loader nunca recebe `target_dir`;
- loader nunca extrai;
- URL e SHA-256 são opcionais e vazios por padrão;
- handoff externo exige BLAKE3 canônico real;
- download máximo: 128 MiB;
- até 65.536 entradas, 256 MiB por entrada e 768 MiB descompactados;
- razão de compressão máxima convencional: 500;
- entradas únicas, relativas, sem NUL, backslash ou `..`;
- `SYMLINKS.txt` obrigatório;
- recibo host com `claim_allowed=false`;
- fallback embutido somente quando a inbox externa não existe.

## Gates

```bash
python3 -m unittest tests/test_loader_secure_handoff_contract.py -v
python3 tools/validate_loader_secure_handoff.py
./gradlew :app:assembleDebug :loader:materializeLoaderApk --no-daemon
python3 tools/verify_loader_artifacts.py \
  --loader dist/loader/loader.apk \
  --host dist/loader/host-debug.apk
```

O workflow existente `.github/workflows/loader-apk-contract.yml` executa essa
cadeia e publica APKs, hashes e recibos.

## Provas abertas

1. workflow com steps e logs;
2. APKs associados ao HEAD final;
3. certificados iguais observados por `apksigner`;
4. testes instrumentados de app não autorizado e URI grant/revogação;
5. pins reais quando a rota externa for ativada;
6. Moto E7 ARM32 e Realme ARM64;
7. `DEVICE_RECEIPT_COMPLETE`.

## Rollback

Sem pins, o caminho embutido permanece. Falha no loader não altera o host.
Falha na custódia remove arquivos `.part`. Falha no instalador remove staging e
preserva o prefixo anterior. Inbox incompatível com nova versão é descartada.
