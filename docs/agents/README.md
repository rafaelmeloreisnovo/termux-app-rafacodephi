# Portal de Agentes — Migração Freestanding

Este diretório contém instruções específicas para Claude/Cloud, GitHub Copilot e OpenAI Codex executarem a mesma arquitetura sem criar três versões incompatíveis do projeto.

## Documentos canônicos

Todos os agentes devem ler primeiro:

- [`../architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md`](../architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md)
- [`../audits/QEMU_ANDROIDX_RMR_CORRECTION_AUDIT.md`](../audits/QEMU_ANDROIDX_RMR_CORRECTION_AUDIT.md)
- [`../../AGENTS.md`](../../AGENTS.md)
- [`../STATUS.md`](../STATUS.md)
- [`../RUNTIME_TRUTH_TABLE.md`](../RUNTIME_TRUTH_TABLE.md)
- [`../ENGINEERING_SYSTEM_RUNBOOK.md`](../ENGINEERING_SYSTEM_RUNBOOK.md)

## Correção de escopo

O `qemu_rafaelia` já possui um runtime cíclico integrado ao lifecycle, timer, ABI, rota e observabilidade do QEMU.

Não remover esse runtime de dentro do QEMU e não reclassificá-lo como erro arquitetural.

A separação correta é:

```text
RAFAELIA QEMU RUNTIME
- permanece integrado ao QEMU
- agenda e mede ciclos lógicos
- conhece QEMUTimer, lifecycle, QMP/IPC e rotas

PURE COMPUTE CORE
- contém somente a matemática portátil
- não conhece QEMU, Android, Linux, JNI ou syscall
- pode ser chamado pelo runtime QEMU e por outros adapters

ANDROID APP
- instala/valida o artifact externo
- controla processo, UI, câmera, sensores e persistência
```

Portanto, “core” deve sempre ser qualificado como `QEMU hosted runtime core` ou `pure compute core`.

## Invariante compartilhada

```text
runtime QEMU cíclico preservado
+ pure compute core canônico
+ adapters especializados
+ flags por módulo
+ exports por allowlist
+ vetores ouro
+ auditoria ELF/QEMU/device
```

## Papéis

| Agente | Papel principal | Forma de entrega |
|---|---|---|
| Claude/Cloud | arquitetura, decomposição e revisão sistêmica | plano, contratos e refatoração guiada |
| Copilot | implementação orientada por issue/arquivo | commits pequenos e testes locais |
| Codex | execução transversal no repositório | branch, patches, CI, relatórios e PR |

## Regras comuns

1. Não chamar o APK, AAR ou `.so` JNI de freestanding.
2. Somente o `pure compute core` fica livre de Android, JNI, Linux, QEMU, arquivo, thread, relógio e syscall.
3. Preservar o runtime cíclico RAFAELIA já integrado ao QEMU.
4. Não introduzir novo CRC, estado ou kernel paralelo.
5. Não apagar comentários para reduzir binário; comentários não entram no ELF.
6. Não desabilitar warning globalmente sem justificativa por arquivo.
7. Não misturar `-Os` e `-O3` no mesmo módulo.
8. Não usar `-ffast-math` no pure core determinístico.
9. Não importar o repositório AndroidX inteiro no build do app.
10. Não incorporar a árvore QEMU no APK; consumir artifact pinado e validado.
11. Câmera/HDR permanece Android-first; só criar ponte virtual quando o guest realmente precisar do frame.
12. Toda afirmação deve apontar para código, comando, artifact ou ficar marcada como `TOKEN_VAZIO`.

## Ordem de execução compartilhada

```text
baseline
→ inventário do que já sobrevive no QEMU/AndroidX_RmR
→ pure compute core sem remover o runtime QEMU
→ adapters
→ warnings/flags
→ símbolos/ELF
→ QEMU-user
→ QEMU-system
→ Android device
→ AndroidX/RmR seletivo
→ câmera/HDR bridge opcional
```

## Resultado final esperado

Os três agentes devem produzir mudanças compatíveis com os mesmos contratos:

- `QEMU_CYCLE_RUNTIME_PRESERVED`;
- `CORE_SOURCE_PURE`;
- `CORE_NO_HEAP`;
- `CORE_NO_SYSCALL`;
- `EXPORT_ALLOWLIST_EXACT`;
- `C_ASM_BIT_EQUIVALENCE`;
- ARM32/ARM64;
- QEMU user/system;
- Android device;
- vetores ouro idênticos;
- QEMU artifact externo pinado;
- câmera/HDR fora do guest por padrão.
