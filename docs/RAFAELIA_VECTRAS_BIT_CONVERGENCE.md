# RAFAELIA Vectras Bit Convergence (Termux x Vectras)

## Escopo real desta auditoria
- Este repositório contém o lado Termux.
- Repositório Vectras não está presente localmente nesta execução; comparação Vectras aqui é por referência declarada e deve ser revalidada no repo alvo.

## BitRAF
- Termux: `raf_bitraf.h/.c` com encode/decode/validate 42 bits.
- Ação: manter como baseline de ABI e criar teste de equivalência contra corpus Vectras.

## ZipRAF
- Termux: documental/conceitual.
- Vectras (referência): `RmR_Zipraf_Execute` com `route_tag`, `bitraf_hash`, `crc32c`, `det_signature`, `status_flags`.
- Ação: importar núcleo C puro + wrapper JNI + selftest de determinismo.

## BitOmega
- Termux: sem módulo oficial equivalente localizado.
- Vectras (referência): FSM Q16 com coherence/entropy/noise/load e estados NEG..META.
- Ação: portar FSM como lib isolada e mapear no `raf_vcpu` sem quebrar ABI.

## Decisão de convergência
- Importar para Termux: ZipRAF core + BitOmega FSM + selftests.
- Ficar em Vectras: integração específica de VM/hardware detect.
- Wrapper necessário: JNI/Gradle para expor APIs.
- Necessita benchmark: determinismo, throughput lógico, custo de CRC.
