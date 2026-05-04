# Execução dos prompts — status

Este ciclo entrega os artefatos-base para execução prática:

1. Whitepaper técnico em LaTeX: `docs/rafaelia/RAFAELIA_WHITEPAPER.tex`
2. Especificação ISA/BitRAF: `asm/RAFAELIA_ISA_BITRAF.md`
3. Constantes Q16.16: `docs/rafaelia/CONSTANTS.md`
4. Simulador clássico inicial: `simulator/rafaelia_qpu_sim.py`
5. Verificação formal computacional (exaustiva): `scripts/verification/verify_period42_q16.py`

Itens restantes (Qiskit Pulse, C++/CUDA, FPGA, SDK Python, reorganização ampla de repo) exigem etapas incrementais para evitar regressão estrutural no monorepo atual.
