# ARM32 Bootstrap — guia de 1 minuto

Para o usuário que só quer avançar o aparelho ARM32 v7 sem entender a árvore inteira de CI.

## No GitHub

1. Abra **Actions**.
2. Escolha **🧭 RAFAELIA — Executar / Diagnosticar**.
3. Toque em **Run workflow**.
4. Em `O que você quer fazer?`, escolha **bootstrap-arm32**.
5. Em `ARM32: toolchain`, deixe **canonical**.
6. Em `Governança estrita`, deixe **false** enquanto a migração dos workflows antigos não terminar.
7. Execute.

## O que você verá

A execução é apresentada em seis passos:

1. 🧭 inventário dos workflows;
2. 🧪 testes, quando solicitados;
3. 📱 ARM32 v7;
4. 🧱 contrato do bootstrap;
5. 🧾 contrato de evidência APK;
6. Ω resultado simples.

Abra logs detalhados somente se uma etapa ficar vermelha.

## Resultado esperado para avançar ao aparelho

Para `bootstrap-arm32`, o GitHub precisa mostrar como `success`:

- inventário;
- a lane ARM32 escolhida;
- bootstrap contract;
- APK evidence contract.

Depois baixe o artefato `arm32-v7-canonical-<commit>` e use o APK `armeabi-v7a` no aparelho.

## O que o GitHub não pode provar sozinho

Mesmo tudo verde, o estado físico continua:

`device_runtime_proof=TOKEN_VAZIO`

até o APK ser realmente instalado no Android e um novo receipt ser produzido pelo Bootstrap Wizard/Beta Orchestrator.

## Regra simples

**Verde no GitHub = pode avançar para o teste físico.**

**Receipt físico PASS = pode fechar o gate do aparelho.**

Uma coisa não substitui a outra.
