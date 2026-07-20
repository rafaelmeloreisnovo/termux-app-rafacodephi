# FINAL BUILD CLOSURE — Termux RAFCODEΦ

> Revisão: 2026-07-20  
> Autoridade executável: `tools/validate_system_finalization.py`  
> Contrato: `configs/system-finalization-contract.json`

## O que significa “terminado”

Este repositório possui três produtos sobrepostos. Eles não podem compartilhar a mesma palavra **finalizado** sem indicar o escopo.

```text
SAFE_CORE_IMPLEMENTATION_CLOSED
!= FUNCTIONAL_DISTRIBUTION_RELEASE_CLOSED
!= FULL_RESEARCH_PLATFORM_CLOSED
```

## Perfil 1 — safe-core

O perfil `safe-core` fecha o núcleo implementado e suas fronteiras de segurança:

- metadados Android/NDK/ABI canônicos;
- referências de GitHub Actions dentro da política;
- loader aceito somente como stub inerte em quarentena ou implementação integralmente protegida;
- RAFAELIA ZERO ligada ao APK com instrumentos de probe, receipt, bundle e matriz;
- fontes de verdade presentes;
- validação host executável e fail-closed.

Ele **não** afirma:

- APK de produção instalado em aparelho;
- `pkg/apt/dpkg/proot` reais;
- assinatura de produção disponível;
- CI remoto observável;
- loader funcional;
- TLS próprio;
- compiladores completos;
- VM completa.

Estado esperado deste perfil:

```text
state=SAFE_CORE_IMPLEMENTATION_CLOSED
claim_allowed_scope=true
release_allowed=false
```

Comando:

```bash
python3 tools/validate_system_finalization.py \
  --profile safe-core \
  --strict \
  --write-report
```

## Perfil 2 — functional-distribution

Este é o fechamento necessário para chamar o projeto de uma distribuição Termux funcional e promovível.

Além do `safe-core`, exige simultaneamente:

1. CI final com etapas e logs observáveis no SHA exato;
2. assinatura de produção registrada como evidência, sem expor segredo;
3. bundle físico ARM32 e bundle físico ARM64 válidos;
4. matriz `DUAL_ARM_DEVICE_PROOF`;
5. stack real `pkg/apt/apt-get/dpkg/libapt/proot` recompilada para o prefixo RAFCODEΦ;
6. certificados, DNS e repositório validados no aparelho;
7. `runtime-lock.json` fixado no commit exato de release, com hash externo preenchido.

Estado atual:

```text
state=BLOCKED
release_allowed=false
```

Bloqueadores dominantes:

```text
LEGACY_PREFIX_BINARY_RISK
DEVICE_RECEIPT_COMPLETE_ARM32=TOKEN_VAZIO
DEVICE_RECEIPT_COMPLETE_ARM64=TOKEN_VAZIO
PRODUCTION_RELEASE_SIGNING=TOKEN_VAZIO
OBSERVABLE_REMOTE_CI=TOKEN_VAZIO
RUNTIME_LOCK=STALE_OR_INCOMPLETE
```

Comando de prova negativa:

```bash
python3 tools/validate_system_finalization.py \
  --profile functional-distribution \
  --strict
```

Enquanto os bloqueadores existirem, o retorno não zero é comportamento correto.

## Perfil 3 — full-platform

Este perfil inclui os objetivos de pesquisa que não pertencem ao fechamento mínimo da distribuição:

- TLS 1.2 e TLS 1.3 próprios e interoperáveis;
- validação X.509 e certificação externa;
- compiladores APKC completos, com IR, backends DEX/ELF, empacotamento e runtime matrix;
- VCPU promovida de kernel de estado para VM completa.

Fixtures fixas de ELF/DEX, ClientHello parcial, fail-closed HTTPS e máquina de estados não satisfazem este perfil.

Estado atual:

```text
state=BLOCKED
research_open=true
```

## Matriz canônica

| Camada | Estado atual | Pode ser chamada de finalizada? |
|---|---|---|
| Núcleo estático, contratos e quarentenas | `SAFE_CORE_IMPLEMENTATION_CLOSED` após o gate | Sim, somente nesse escopo |
| APK/NDK produzido em CI histórico | `PROVADO`, sujeito ao SHA/artefato correspondente | Não prova release atual |
| Loader | `STUB_SAFE_BLOCKED` | Sim como stub seguro; não como instalador |
| RAFAELIA ZERO | instrumentação `PROVADO ESTRUTURAL` | Não prova aparelho |
| Prova ARM32 + ARM64 | `TOKEN_VAZIO` | Não |
| Stack real de pacotes | `BLOCKED` por prefixo legado | Não |
| Release de produção | `BLOCKED` | Não |
| TLS/compiladores/VM completos | `TOKEN_VAZIO` | Não |

## Gate incorporado

O perfil `safe-core` é chamado pelo validador canônico `scripts/verify_rafaelia_native_safety.py`. A integração não cria uma nova autoridade de CI: ela amplia a trilha já existente.

```text
native compile contract
→ RAFAELIA ZERO contract
→ system finalization safe-core
```

## Regra de promoção

Um perfil só pode subir de `BLOCKED` para seu estado final quando **todos** os checks obrigatórios daquele perfil forem verdadeiros no mesmo corte.

```text
arquivo presente != integrado
integrado != compilado
compilado != executado
executado no host != executado no Android
um aparelho != matriz ARM32 + ARM64
safe-core fechado != release liberado
```

## Próximo fechamento físico

O próximo passo que depende do aparelho, e não de mais abstração, é o issue `#295`:

```text
APK exato
→ instalação no Android alvo
→ probe protegido
→ captura do base.apk instalado
→ receipt e transcript ligados por hash
→ bundle ARM32
→ bundle ARM64
→ matriz dual
```

Até essa prova existir, `release_allowed=false` é invariante, não deficiência documental.
