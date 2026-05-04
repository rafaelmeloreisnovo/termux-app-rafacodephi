#!/usr/bin/env python3
"""Calibração automática por algoritmo genético leve (42 ciclos)."""
import numpy as np

PERIOD=42
FREQ0=np.array([9804,19608,29412,39216,49020,58824,68628],dtype=float)

def fitness(pump):
    c=0.5; h=0.5
    for t in range(PERIOD):
        cin=np.tanh(np.dot(pump,np.sin((t+1)*FREQ0/65536.0))) *0.5+0.5
        hin=1-cin
        c=0.75*c+0.25*cin; h=0.75*h+0.25*hin
    return (1-h)*c

def evolve(pop=32,gens=40):
    rng=np.random.default_rng(42)
    P=rng.uniform(0.1,2.0,size=(pop,7))
    for _ in range(gens):
        fit=np.array([fitness(p) for p in P])
        elite=P[np.argsort(fit)[-8:]]
        children=[]
        for _ in range(pop-8):
            a,b=elite[rng.integers(0,8)],elite[rng.integers(0,8)]
            m=rng.random(7)<0.5
            c=np.where(m,a,b)+rng.normal(0,0.05,7)
            children.append(np.clip(c,0.05,3.0))
        P=np.vstack([elite,np.array(children)])
    fit=np.array([fitness(p) for p in P]); i=np.argmax(fit)
    return P[i],fit[i]

if __name__=='__main__':
    best,score=evolve(); print('best_pump=',best); print('phi42=',score)
