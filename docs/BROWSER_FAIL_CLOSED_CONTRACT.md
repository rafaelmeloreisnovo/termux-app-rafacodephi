# RAFAELIA Browser — contrato HTTPS fail-closed

## Escopo

`Browser.sh` contém um gerador histórico de navegador textual freestanding. Ele possui primitivas reais de syscall, DNS, TCP, HTTP/1.1 e renderização textual, mas o bloco chamado TLS implementa apenas parte do `ClientHello`, leitura de cabeçalho de record e estados demonstrativos.

Ele **não** implementa TLS 1.2 ou TLS 1.3 funcional.

## Problema corrigido no caminho canônico

O protótipo histórico alterava uma solicitação HTTPS para porta 80 após o handshake incompleto:

```text
HTTPS:443
→ handshake incompleto
→ use_tls = 0
→ HTTP:80
```

Esse comportamento viola a invariante de confidencialidade:

```text
HTTPS_REQUESTED
→ PLAINTEXT_FORBIDDEN
```

## Entrada canônica

Materialize o build seguro com:

```bash
python3 scripts/materialize_browser_fail_closed.py \
  --output /tmp/raf-browser-build-safe.sh
```

Para executar o script de build materializado explicitamente:

```bash
bash /tmp/raf-browser-build-safe.sh
```

Também existe a opção consciente:

```bash
python3 scripts/materialize_browser_fail_closed.py \
  --output /tmp/raf-browser-build-safe.sh \
  --execute
```

## Semântica

Quando uma URL HTTPS chega ao protótipo materializado:

1. o `ClientHello` demonstrativo pode ser emitido;
2. nenhum segredo de aplicação é considerado estabelecido;
3. o socket é fechado;
4. o estado TLS torna-se `TLS_ERROR`;
5. o processo retorna erro;
6. porta, esquema e intenção HTTPS não são reescritos para HTTP.

```text
HTTPS + TLS_INCOMPLETE
→ FAIL_CLOSED
→ NO_PLAINTEXT_DOWNGRADE
```

## Prova

```bash
python3 scripts/validate_browser_fail_closed.py
```

O validador materializa o script em diretório temporário e verifica:

- ausência de `ctx->port=80u;ctx->use_tls=0;`;
- ausência da mensagem de fallback HTTP;
- presença de `TLS_ERROR`;
- presença de erro HTTPS explícito;
- retorno terminal `-2`;
- registro correspondente na matriz operacional.

Esse gate é chamado por:

```bash
bash scripts/test_raf_native_compile_contract.sh
```

## Limites

O gate prova somente a ausência do downgrade conhecido no caminho canônico. Ele não prova:

- TLS 1.2;
- TLS 1.3;
- X25519;
- HKDF;
- AES-GCM;
- ChaCha20-Poly1305;
- validação X.509;
- hostname verification;
- trust store;
- interoperabilidade;
- certificação externa.

```text
FAIL_CLOSED = VERIFIED_HOST
TLS_FUNCTIONAL = TOKEN_VAZIO
TLS_CERTIFICATION = TOKEN_VAZIO
claim_allowed_tls = false
```

`Browser.sh` bruto permanece classificado como fonte histórica de protótipo. O caminho de build aprovado é o materializador fail-closed até que o código TLS completo substitua essa fronteira.
