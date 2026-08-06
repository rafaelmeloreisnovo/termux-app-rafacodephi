# R3.2B — Reconciliação de autoridade do `termux-packages`

## Fato temporal

O `runtime-lock.json` do PR #330 foi escrito quando o fork:

```text
rafaelmeloreisnovo/termux-packages
```

não possuía branches observáveis. Depois desse registro, o fork foi povoado e recebeu as fases 1–8.

A correção não apaga o estado anterior. Ela registra a transição:

```text
fork_status=empty
→ fork_main=cde7ed15e5c529847f9e27904a4d454f166bcb8f
→ experimental PR #4=e111c2f81b13ee9b30cb5cc0d675c37b9feec659
```

## Duas autoridades, duas funções

| Corpo | Papel | Estado |
|---|---|---|
| `termux/termux-packages@eb124b51...` | referência produtiva pinada | preservada |
| `rafaelmeloreisnovo/termux-packages` | construtor experimental RAFCODE-Φ | ativo, não promovido |

O fork não substitui o upstream até produzir:

1. receipt NDK `26.3.11579264`;
2. build ARMv7;
3. build AArch64;
4. varredura de prefixo legado;
5. decisão explícita sobre TAR versus `.deb/apt/dpkg`;
6. hashes assinados;
7. instalação, execução e rollback em dispositivo.

## Estado epistemológico

```text
claim_allowed=false
release_allowed=false
fork_promotion=TOKEN_VAZIO_ARMV7_AARCH64_PREFIX_SAFE_DEVICE_RECEIPTS
```

## Manifesto experimental

O PR #4 do fork:

- liga o loader ao executor;
- corrige o deslocamento do string pool;
- reserva offset zero para ausência;
- rejeita manifesto truncado ou fora do contrato;
- impede truncamento silencioso acima de 16 dependências;
- preserva falha real de `configure`;
- exige fonte materializada e payload não vazio.

Ainda permanecem:

```text
TOKEN_VAZIO_SOURCE_FETCH
TOKEN_VAZIO_PATCH_EXECUTION
TOKEN_VAZIO_MANIFEST_V2_REQUIRED
TOKEN_VAZIO_DEPENDENCY_CLOSURE
TOKEN_VAZIO_TOOLCHAIN_RECEIPT
TOKEN_VAZIO_DEB_APT_DPKG_CONTRACT
TOKEN_VAZIO_DEVICE_INSTALL
```

## R3

- **F_ok:** origem produtiva e construtor experimental separados no lock.
- **F_gap:** toolchain, duas ABIs, contrato de distribuição e receipts físicos.
- **F_next:** revisar PR #4, instalar NDK, compilar ARMv7/AArch64 e fechar o ciclo no dispositivo.
