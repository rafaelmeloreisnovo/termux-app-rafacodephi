# Build, Bootstrap e Modelo de Hardware

## Cadeia de build e instalação

- O APK é compilado antes de ser instalado.
- Código Java/Kotlin é convertido para bytecode DEX durante o build.
- Código nativo C/ASM é compilado em bibliotecas `.so` durante o build (NDK).
- A instalação do APK pode acionar otimizações ART/dexopt no dispositivo, mas **não** compila código-fonte do app.

## Bootstrap embutido por ABI

- O bootstrap ZIP é embutido no binário nativo por ABI usando `.incbin`.
- O payload bootstrap é carregado via JNI no runtime.
- A integridade do bootstrap é verificada por BLAKE3 em runtime, de acordo com a ABI atual.

## Modelo de detecção de hardware em user-space

- A detecção de capacidades usa interfaces de user-space, incluindo `/proc/self/auxv` (HWCAP/HWCAP2) e CPUID/xgetbv em x86.
- O código usa fallback quando capacidades de runtime não estão disponíveis.
- O perfil de hardware inclui dados como page size, CPU online e flags de acesso.

## Limites de acesso em app Android comum

- Aplicativos Android em user-space comum não têm acesso direto a registradores físicos.
- Aplicativos Android em user-space comum não têm acesso direto a GPIO.
- Aplicativos Android em user-space comum não têm acesso direto a MMIO de kernel.
