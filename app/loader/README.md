# RAFCODEΦ loader APK

Este módulo produz o artefato contratual `loader.apk` sem incorporar código de negócio ou payload de bootstrap.

## Build

```bash
./gradlew :loader:materializeLoaderApk --no-daemon
```

Saída esperada:

```text
app/loader/build/dist/loader.apk
```

## Estado

```text
module_present = true
apk_producer = true
has_code = false
bootstrap_payload = absent
installer_behavior = absent
release_allowed = false
state = STUB_NO_BOOTSTRAP_PAYLOAD
```

O APK contém somente metadados de contrato. Ele fecha a ausência estrutural de um produtor chamado `loader.apk`, mas **não** deve ser descrito como instalador funcional.

## Gate para implementação funcional

A promoção exige, no mínimo:

1. definição do fluxo de instalação e consentimento do usuário;
2. origem pinada do bootstrap;
3. SHA-256/BLAKE3 dos payloads;
4. validação de ABI e page size;
5. política de atualização/rollback;
6. assinatura e ownership definidos;
7. testes instrumentados de instalação;
8. integração explícita com `com.termux.rafacodephi`;
9. revisão de segurança para intents, providers e permissões.

Até esses itens existirem, use:

```text
BLOCKED_BY[LOADER_FUNCTIONAL_CONTRACT_REQUIRED]
```
