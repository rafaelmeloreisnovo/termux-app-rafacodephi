# CI Private Billing Boundary V1 — Runtime Android/Termux

**Evento:** `RAFAELIA-CI-PRIVATE-BILLING-BOUNDARY-V1-20260730T054800Z`  
**Predecessor:** termux-app-rafacodephi `5125123999023913d95245cd3b5b6a4aedad9faf` (PR #312)  
**Tempo:** 2026-07-30 05:48 UTC / 02:48 BRT  
**Política:** `APPEND_ONLY · NON_DESTRUCTIVE · CLAIM_ALLOWED=false · NO_AUTO_MERGE`

## Regra de runtime

Para este repositório privado, GitHub Actions indisponível por cobertura de pagamento ausente deve ser classificado como:

```text
CI_UNAVAILABLE_PRIVATE_BILLING
```

Não é falha de APK, ABI, assinatura, instalação, Android ou Termux. Esses estados continuam não observados até um receipt físico do dispositivo.

## Receipt mínimo de Termux

```text
commit + dispositivo/ABI + Android API
+ ambiente + comando + input hashes
+ stdout/stderr hashes + exit code
+ output/receipt SHA-256
```

O runner local pode produzir evidência de execução sem depender de Actions. Essa evidência não substitui testes de segurança, compatibilidade ampla, desempenho ou publicação.

## F_next

Executar no aparelho alvo com o bundle-fonte exato, preservar o receipt separado e indexá-lo no Mapa. Até então: `TOKEN_VAZIO_RUNTIME_NOT_EXECUTED`.
