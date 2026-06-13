"""Tune dimod SA: find minimal reads/sweeps that reliably reach energy -8, with timing."""
import sys, time
import numpy as np
import dimod
from dwave.samplers import SimulatedAnnealingSampler

PENALTY=1.1; GROUP=256; NG=8; NVARS=2048

def build_qubo(e):
    n=e.shape[0]; Q=np.zeros((n,n))
    di=np.arange(n)
    Q[di,di]-=1.0
    # same-group off diagonal
    for g in range(NG):
        s=g*GROUP; t=(g+1)*GROUP
        Q[s:t, s:t]+=PENALTY
        Q[np.arange(s,t),np.arange(s,t)]-=PENALTY  # remove diagonal double of same-group
    # edges
    nz=np.nonzero(e)
    for i,j in zip(nz[0],nz[1]):
        Q[i,i]+=PENALTY; Q[i,j]-=PENALTY
    # upper triangular
    Q=np.triu(Q)+np.tril(Q,-1).T  # fold lower into upper
    iu=np.tril_indices(n,-1); Q[iu]=0.0
    return Q

def qdict(Q):
    nz=np.nonzero(Q); return {(int(i),int(j)):float(Q[i,j]) for i,j in zip(nz[0],nz[1])}

path=sys.argv[1]
e=np.loadtxt(path,dtype=np.int8)
Q=build_qubo(e)
bqm=dimod.BinaryQuadraticModel.from_qubo(qdict(Q))
sa=SimulatedAnnealingSampler()

for reads,sweeps in [(1,1000),(5,1000),(10,1000),(1,2000),(5,2000),(10,4000)]:
    energies=[]; ts=[]
    for trial in range(5):
        t=time.time()
        ss=sa.sample(bqm,num_reads=reads,num_sweeps=sweeps)
        ts.append(time.time()-t)
        energies.append(ss.first.energy)
    hit=sum(1 for en in energies if en<-7.99)
    print(f"reads={reads:3d} sweeps={sweeps:5d}  hit-(-8)={hit}/5  meanE={np.mean(energies):.3f}  wall={np.mean(ts)*1000:.1f}ms")
