# Portal de Agentes — Migração Freestanding

Este diretório contém instruções específicas para Claude/Cloud, GitHub Copilot e OpenAI Codex executarem a mesma arquitetura sem criar três versões incompatíveis do projeto.

## Documento canônico

Todos os agentes devem ler primeiro:

- [`../architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md`](../architecture/GRADLE_ANDROIDX_QEMU_FREESTANDING_ARCHITECTURE.md)
- [`../../AGENTS.md`](../../AGENTS.md)
- [`../STATUS.md`](../STATUS.md)
- [`../RUNTIME_TRUTH_TABLE.md`](../RUNTIME_TRUTH_TABLE.md)
- [`../ENGINEERING_SYSTEM_RUNBOOK.md`](../ENGINEERING_SYSTEM_RUNBOOK.md)

## Invariante compartilhada

```text
um core canônico
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
2. O core não conhece Android, JNI, Linux, QEMU, arquivo, thread, relógio ou syscall.
3. Não introduzir novo CRC, estado ou kernel paralelo.
4. Não apagar comentários para reduzir binário; comentários não entram no ELF.
5. Não desabilitar warning globalmente sem justificativa por arquivo.
6. Não misturar `-Os` e `-O3` no mesmo módulo.
7. Não usar `-ffast-math` no core determinístico.
8. Não importar o repositório AndroidX inteiro no build do app.
9. Não incorporar a árvore QEMU no APK.
10. Toda afirmação deve apontar para código, comando, artifact ou ficar marcada como `TOKEN_VAZIO`.

## Ordem de execução compartilhada

```text
baseline
→ core único
→ adapters
→ warnings/flags
→ símbolos/ELF
→ QEMU-user
→ QEMU-system
→ Android device
→ AndroidX/RmR seletivo
```

## Resultado final esperado

Os três agentes devem produzir mudanças compatíveis com os mesmos contratos:

- `CORE_SOURCE_PURE`;
- `CORE_NO_HEAP`;
- `CORE_NO_SYSCALL`;
- `EXPORT_ALLOWLIST_EXACT`;
- `C_ASM_BIT_EQUIVALENCE`;
- ARM32/ARM64;
- QEMU user/system;
- Android device;
- vetores ouro idênticos.
