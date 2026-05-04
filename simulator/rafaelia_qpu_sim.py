import numpy as np

N=1000; D=7; STEPS=42; ALPHA=0.25
rng=np.random.default_rng(42)
state=rng.random((N,D))
coh=0.5; ent=0.5


def mandelbrot_mod(zx, zy, it=20):
    c = zx + 1j*zy
    z = 0j
    for _ in range(it):
        z = z*z + c
        if abs(z) > 2: break
    return min(abs(z),2)/2

for t in range(STEPS):
    coupling=np.empty(N)
    for i in range(N):
        coupling[i]=mandelbrot_mod(state[i,0]*2-1, state[i,1]*2-1)
    roll=np.roll(state,7,axis=0)
    state=(state*(1-ALPHA)+ALPHA*roll)*((0.5+0.5*coupling)[:,None])
    coh=(1-ALPHA)*coh+ALPHA*np.mean(np.cos(2*np.pi*state[:,2]))
    ent=(1-ALPHA)*ent+ALPHA*np.mean(-state*np.log(np.clip(state,1e-9,1))).item()

commit_ok = coh > 0.1 and ent < 2.5
print({"coherence":float(coh),"entropy":float(ent),"commit":commit_ok})
