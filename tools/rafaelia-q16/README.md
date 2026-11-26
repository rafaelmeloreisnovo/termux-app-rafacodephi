# RAFAELIA Q16 — scripts de build e comparação (2026-10-25)

**Fonte:** [build_rafaelia_q16.sh](build_rafaelia_q16.sh) gera fontes e executável Q16 com foco ARM32 descrito no código. [build_dual_artifacts.sh](build_dual_artifacts.sh) compara execução do original versus cópia sem símbolos, produz hashes, tamanhos e manifest.

**Distinção crítica:** 'dual artifacts' significa original/stripped, NÃO prova duas ABIs nem dois APKs. O gerador usa shell e ferramentas de host, diferentes do núcleo C freestanding.

**Achado P1:** a fonte gerada define TOKEN_VAZIO como 0. Isso conflita com o contrato epistemológico TOKEN_VAZIO != 0. Revisar a semântica tipada e compatibilidade binária antes de propor correção; não substituir cegamente.

**Provas pendentes:** ABI/compilador exatos, link/imports, estado de saída original/stripped, recibo por SHA e dispositivo. Nada foi executado ou promovido aqui; zero-delete e HOLD.
