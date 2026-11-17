# Java puro: seleção, custódia e limite

Predecessor: phase-2/authorial-bit-library-gates. Esta fase mantém a API existente de AuthorialBitOps e acrescenta o backend JAVA_PURE como seleção explícita v2. O manifesto v1 continua como histórico.

A biblioteca usa operadores primitivos de Java para read/write/count/parity. Não importa bibliotecas nem carrega JNI ou ELF. Seu bytecode continua executado por JVM/ART: "Java puro" descreve a dependência do módulo, não uma execução sem runtime.

Execute: bash scripts/verify_pure_java_bit_core.sh. O gate compila com javac, executa vetores de borda e 4096 inteiros, verifica invariantes de escrita e altera um bit em uma cópia para confirmar que o teste acusa regressão. O teste da cópia não altera a fonte versionada.

O workflow anexa receipt com commit, run ID, hashes e limite JVM_HOSTED_ONLY. O receipt de build não prova que o módulo entrou no APK/DEX, nem desempenho ou execução no Android. Comparação de latência requer entradas, aquecimento, rodadas e amostras no mesmo aparelho; sem essa medição, o argumento "trocar um bit fica mais lento" permanece hipótese.

A PR desta fase mira a branch predecessora. Após a revisão, a cadeia pode ser integrada a master por PR com aprovação humana. Branches históricas permanecem. Beta e release exigem seus gates e aprovação próprios.
