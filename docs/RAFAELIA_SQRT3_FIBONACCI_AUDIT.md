# RAFAELIA √3/2 + Fibonacci Audit

## Separação factual
- **Fato matemático básico**: √3/2 é altura normalizada do triângulo equilátero.
- **Fato de implementação**: constante Q16 ~56755/56756 aparece no ASM/C.
- **Hipótese nova**: operador universal multi-domínio.

## Classificação dos itens
- √3/2 como contração: plausível quando |a|<1 em sistemas lineares; precisa formalização no sistema completo.
- √3/2 como polo IIR: testável e formalizável por resposta ao impulso/frequência.
- Lyapunov negativo λ=ln(√3/2): matematicamente coerente para mapa escalar; não prova universalidade.
- Base de convergência global: claim forte, ainda não provado.
- F*≈23.158: não há derivação formal completa no código local.

## Sequências e janelas
Sequências declaradas:
- `0001123`
- `001123`
- `0123`
- `123`

Interpretação operacional recomendada: **janelas/projeções de estado**, não Fibonacci automática.

## Níveis N0..N7
| Nível | Codificação | Relação com Fibonacci clássico |
|---|---|---|
| N0 | 000 | padding/estado inicial |
| N1 | 001 | janela com zero à esquerda |
| N2 | 011 | coincide com prefixo Fibonacci |
| N3 | 012 | coincide |
| N4 | 123 | coincide janela deslocada |
| N5 | 235 | coincide janela deslocada |
| N6 | 358 | coincide janela deslocada |
| N7 | 5813 | concatenação de janela, não termo escalar |

Comparação com `0,1,1,2,3,5,8,13`:
- vira Fibonacci formal apenas quando regra explícita `F(n)=F(n-1)+F(n-2)` é aplicada por termo.
- onde há concatenação/padding, vira codificação própria.
