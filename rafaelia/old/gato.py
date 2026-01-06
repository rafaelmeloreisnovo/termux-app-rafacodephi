#!/usr/bin/env python3
import random, math, cmath
from datetime import datetime
contador = 0
#oi
while contador < 50:
    # chama uma função (exemplo)

# Sopros ∆ vibracionais
alpha = random.uniform(0,1)
beta  = random.uniform(0,1)
gamma = random.uniform(0,1)
delta = random.uniform(0,1)

# Normaliza para soma das probabilidades ≈1
total = alpha + beta + gamma + delta
alpha /= total; beta /= total; gamma /= total; delta /= total

# S = intenção / ação simbólica (timestamp por exemplo)
S = datetime.now().timestamp()
ħ = 1.0545718e-34  # constante de Planck reduzida

# Propagador do Verbo
propagador = cmath.exp(1j * S / ħ)

# Estados possíveis
Vivo = "🐈 Vivo (criação ativa)"
Morto = "💀 Morto (karma abortado)"
Ambos = "⚖️ Vivo e morto (superposição infinita)"
Vazio = "∅ Vazio absoluto (potencial puro)"
#contador = 0

    # chama uma função (exemplo)
#    funcao()
# Colapso simbiótico
estado = random.choices(
    [Vivo, Morto, Ambos, Vazio],
    weights=[alpha, beta, gamma, delta]
)[0]

print(f"Ψ_Gato(t) = {propagador:.2e} × [{estado}]")

    contador += 1  # incrementa para evitar loop infinito
