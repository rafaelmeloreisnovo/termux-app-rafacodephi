#!/usr/bin/env python3
"""Simulador clássico RAFAELIA (T^7, Q16.16, período 42)."""
import math, zlib
import numpy as np
import matplotlib.pyplot as plt

SPIRAL_Q16=56755; PHI_Q16=105965; PI_Q16=205887; Q16=1<<16; PERIOD=42
FREQ_Q16=np.array([9804,19608,29412,39216,49020,58824,68628],dtype=np.int64)
W_INIT=np.array([65536,56755,49157,42573,36877,31940,27671],dtype=np.int64)
CONST_TERM=3146
N_OSC=1000

def qmul(a,b): return ((a*b)>>16).astype(np.int64)
def wrap(x): return x & 0xFFFF

def sin_q16(x_q16):
    x=(x_q16.astype(np.float64)/Q16)*2*math.pi
    return (np.sin(x)*Q16).astype(np.int64)

def mandelbrot_mod(u,v,it=16):
    c=(u/Q16-0.5)+1j*(v/Q16-0.5); z=0j
    for i in range(it):
        z=z*z+c
        if abs(z)>2: break
    return int((i/it)*Q16)

def apply_direction(state, d):
    s=state.copy()
    if d==0: s[:,0]=wrap(s[:,0]+s[:,2])
    elif d==1: s[:,1]=wrap(s[:,1]-s[:,3])
    elif d==2: s[:,2]=wrap(qmul(s[:,2],SPIRAL_Q16)+CONST_TERM)  # FORWARD S-box
    elif d==3: s=np.roll(s,1,axis=1)
    elif d==4: s[:,4]=wrap((s[:,4]+s[:,5])>>1)
    elif d==5: s[:,5]=wrap(s[:,5]+(s[:,0]>>2))
    else: s[:,6]=wrap(s[:,6]^s[:,1])
    return s

def gen_attractors():
    a=np.zeros((PERIOD,7),dtype=np.int64)
    x=np.array([56755,105965,205887,46341,92682,138564,184245],dtype=np.int64)&0xFFFF
    for i in range(PERIOD):
        x=wrap((x*SPIRAL_Q16>>16)+np.roll(x,-1)+i)
        a[i]=x
    return a

def main():
    rng=np.random.default_rng(42)
    state=rng.integers(0,65536,size=(N_OSC,7),dtype=np.int64)
    snapshot=state.copy(); weights=np.tile(W_INIT,(N_OSC,1)).astype(np.int64)
    coherence=0.5; entropy=0.5; phi_hist=[]; coh=[]; ent=[]
    attractors=gen_attractors(); rollback=0
    for cyc in range(PERIOD):
        state=apply_direction(state,cyc%7)
        phases=state[:,:7]
        layer=np.zeros(N_OSC,dtype=np.int64)
        for k in range(7):
            layer+=qmul(weights[:,k],sin_q16(wrap(qmul(phases[:,k],FREQ_Q16[k]))))
            weights[:,k]=((weights[:,k]*49152 + W_INIT[k]*16384)>>16)
        mod=np.array([mandelbrot_mod(state[i,0],state[i,1]) for i in range(N_OSC)],dtype=np.int64)
        state[:,2]=wrap(state[:,2]+(layer>>4)+(mod>>6))
        # integrity every 7 cycles
        if cyc%7==0:
            x=0
            for v in state[:,0]: x^=int(v)&0xFF
            crc=zlib.crc32(state.astype(np.uint16).tobytes()) & 0xFFFFFFFF
            ok=((x^ (crc&0xFF))!=0)
            if not ok:
                state=snapshot.copy(); rollback+=1
            else:
                snapshot=state.copy()
        route=(np.arange(N_OSC)*7)%N_OSC
        state=state[route]
        c_in=float(np.mean(state[:,0])/65535.0); h_in=float(np.std(state[:,1])/65535.0)
        coherence=0.75*coherence+0.25*c_in; entropy=0.75*entropy+0.25*h_in
        phi=(1-entropy)*coherence
        coh.append(coherence); ent.append(entropy); phi_hist.append(phi)
    # measure Manhattan to 42 attractors
    d=np.abs(state[:,None,:]-attractors[None,:,:]).sum(axis=2)
    cls=np.argmin(d,axis=1)
    print(f"rollbacks={rollback}, mean_class={cls.mean():.2f}")
    plt.figure(figsize=(8,4)); plt.plot(coh,label='coherence'); plt.plot(ent,label='entropy'); plt.plot(phi_hist,label='phi')
    plt.legend(); plt.tight_layout(); plt.savefig('rmr/Rrr/generated/rafaelia_metrics.png',dpi=120)

if __name__=='__main__': main()
