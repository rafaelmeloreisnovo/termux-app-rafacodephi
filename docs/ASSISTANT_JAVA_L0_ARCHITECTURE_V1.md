# RAFAELIA Assistente — Java puro + backends opcionais — contrato V1

Data: 2026-11-18. Estado: REVIEW_ONLY / claim_allowed=false.
Produtor: rafaelmeloreisnovo/termux-app-rafacodephi.
Branch: feature/assistant-java-arena-v1-20261118.
Base imutável desta proposta: phase-2/authorial-bit-library-gates@61de5f3d3b176dc9bfb9d315cbec60c4dfb84220.
Pasta privada de coordenação: https://drive.google.com/drive/folders/1pMf7n2bJ6CHtm3Zg4vhbH9Ya9-l2LXQB
Roteador canônico: START HERE Ω V2.2 (Drive), não copiar corpus aqui.

## 1. Autoridade e reaproveitamento

- Runtime Java/DEX/Termux: este repositório produtor.
- Vectras: perfis de hardware, ABI e backends ISA continuam no produtor Vectras; não copiar kernels sem licença e equivalência aferidas.
- Mapa: roteamento federado, não implementação do motor.
- Drive: ponteiro privado, estado, receipts; GitHub: código/testes/contratos.
- Reutilizar AuthorialBitOps.java da linha phase-2; preservar PR #544 DRAFT/HOLD e histórico.
- O código da arena não substitui o inferenciador LLaMA ou a memória longitudinal; fornece um primitivo limitado para buffers e execução.

## 2. Separação de fronteiras

HOSTED_JAVA: Java puro, sem imports, JNI, bibliotecas de terceiros ou alocação no caminho de escrita; precisa de JVM/ART, e arrays são objetos no heap pré-alocados.
NATIVE_PURE_CORE: C11/ASM (ou Rust no_std se experimentalmente justificado) com buffers fornecidos pelo chamador, ABI declarada, sem libc/heap/syscall no núcleo.
PLATFORM_GATE: ELF/ABI/syscalls dependentes do SO isolados; nunca chamar registro privilegiado em user-space.
HOSTED_NATIVE_ADAPTER: JNI/Android, getauxval(AT_HWCAP) ou API equivalente com autorização de hardware; CPU feature discovery não é uma promessa de aceleração.
GPU_GATE: Vulkan/GLES compute somente após constatar extensão, driver, tamanho de lote, memória e desempenho real; transferência e sincronização entram no custo total.
NUMA_GATE: declarar UNSUPPORTED_OR_UNKNOWN sem observação de múltiplos nós de memória e política de afinidade; afinidade big.LITTLE não equivale automaticamente a NUMA.
Falhas em capacidade ou evidência: escolher Java escalar, nunca assumir SIMD/GPU.

## 3. Unidade implementada

app/src/main/java/com/termux/rafacodephi/assistant/RafAssistantArenaV1.java
app/src/main/java/com/termux/rafacodephi/assistant/RafAssistantArenaV1Adversarial.java

Memória pré-alocada por instância: arena de 65536 bytes, log undo de 65536 bytes, mapa de páginas, 256 entradas de páginas alteradas.
Granularidade: página de 256 bytes; primeira escrita copia página antiga ao undo; escritas seguintes na mesma página não copiam novamente.
BEGIN -> WRITE -> COMMIT ou ROLLBACK. Estado protegido contra escrita sem transação; fail-safe bloqueia novas transações.
Watchdog: COOPERATIVO, exige heartbeat do host; atraso/regressão de relógio provoca rollback da transação e fail-safe. Não oferece recuperação de pane total, watchdog de processo independente, failover automático, snapshot durável ou prova bare metal.
Single-writer obrigatório; sem sincronização/locks. Uso simultâneo precisa de envelope serializado externo.

## 4. Verificações e falsificadores

EXECUTADO_LOCAL_HOST: javac --release 8, JVM OpenJDK 21, suíte adversarial passou.
EXECUTADO_LOCAL_NEGATIVE: alteração sintética de retorno de erro fez a suíte falhar no caso 1; fonte original preservada.
NÃO_EXECUTADO: CI exata do HEAD remoto, DEX, APK, ARMv7, ARM64, JNI, ELF, Rust, GPU e dispositivo.
Aceite mínimo: bytes e limites corretos; rollback de páginas 0,255,256,65535; erro de leitura, erro de estado, watchdog expirado e relógio regressivo; comparação binária do conteúdo após rollback.
Gates extras antes de promoção: compilação Android do HEAD; teste de processo/ANR; sem escrita concorrente; fuzzing de offsets/tempo; dump/inspeção DEX; ABI nativa e prova de nenhum syscall oculto no PURE_CORE; ARMv7+ARM64 on-device com receipts; validação de permissões.

## 5. Gate de desempenho / paralelismo

Comparar SERIAL_JAVA contra C_SCALAR/NEON/ASM, Rust_no_std e GPU apenas para o mesmo algoritmo e input. Ordem inicial por ROI: buffers/índices -> redução de cópias -> layout/dados -> batch JNI -> SIMD -> paralelismo -> GPU.
Tempo total = preparo + travessia de fronteira + cópias + computação + sincronização + efeitos de temperatura/energia.
Nunca declarar backend mais rápido com microbenchmark isolado. Medir lote pequeno e grande; aquecimento; no mínimo múltiplas rodadas; p50/p95/p99, vazão, RSS/heap, energia e temperatura do dispositivo; comparação cold/warm; corretude byte a byte. Penalidade de paralelismo = T_parallel - T_serial sob a mesma carga. Seleção nativa exige melhoria sustentada sem piorar cauda, RAM ou falhas; se não houver, permanece Java.
Arenas fixas reduzem alocação explícita no hot path, não removem heap/GC da VM; erro por excesso de tamanho retorna erro tipado, não expansão silenciosa.

## 6. Estado e rollback

EVIDENCE: arquivos, blobs e compilação local; PROVEDOR: readback do commit.
GAP: build Android e medições independentes TOKEN_VAZIO.
ROLLBACK: não mesclar esta branch; descartar como candidata ou criar sucessor revisado. Não apagar branch nem histórico.
F_ok: núcleo Java, 45 casos de assert e falsificador negativo locais, registros de código com readback.
F_gap: integração completa, persistência, crash recovery, watchdog externo, failover nativo e performance em dispositivo.
F_next: CI exata do HEAD de revisão + prova DEX/ARM32 antes de propor ponte ao chatbot.
