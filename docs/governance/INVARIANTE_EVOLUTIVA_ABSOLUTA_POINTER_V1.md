# Termux RAFCODEΦ — ponte para a Invariante Evolutiva Absoluta V1

**Autoridade canônica:** `rafaelmeloreisnovo/Mapa`  
**Documento:** `governanca/invariantes/INVARIANTE_EVOLUTIVA_ABSOLUTA_V1.md`  
**Estado local:** `RESPONSIBILITY_POINTER`  
**Claim:** `claim_allowed=false`

## Responsabilidade deste repositório

Dentro da IEA, o Termux RAFCODEΦ preserva a passagem entre fonte, bootstrap, APK, instalação e execução local Android.

```text
fonte
→ bootstrap identificado
→ hashes
→ APK
→ prefixo privado
→ execução física
→ receipt
```

Mudanças no package name, prefixo, ABI, bootstrap ou toolchain são evolutivas somente quando a revisão, o payload instalado, o ambiente, os resultados e o rollback permanecem verificáveis.

## Evidência delimitada do espelho auditado

```text
ZIP: termux-app-rafacodephi-master (4).zip
SHA-256: c2549ba985b804dcda3a75261f97a28972aa1ededc873883156cb2e0f3cf05b5
arquivos: 2164
estado: MIRROR_SOURCE_SNAPSHOT_BOUND
self-test C: ok=21 fail=0
contratos Python selecionados: 12 PASS / 1 FAIL
```

A falha observada registra deriva entre o teste e a tarefa Gradle esperada. Ela não foi ocultada nem promovida a falha do shell físico.

## Fronteiras preservadas

```text
shell funcional ≠ distribuição Termux completa
arquivo chamado pkg ≠ backend APT real
bootstrap íntegro ≠ pkg update/install comprovado
build host ≠ ARM32/ARM64 físico
APK criado ≠ APK instalado e validado
```

## Próximo fechamento local

1. manter perfis `bridge` e `real-pkg` distintos;
2. ligar hash aos bytes realmente embutidos;
3. compilar e instalar o APK exato;
4. emitir receipt ARM32/ARM64;
5. promover `pkg/apt/dpkg` somente após update/install real.

O userland pode evoluir; a cadeia entre payload, APK, instalação e prova física não pode desaparecer.
