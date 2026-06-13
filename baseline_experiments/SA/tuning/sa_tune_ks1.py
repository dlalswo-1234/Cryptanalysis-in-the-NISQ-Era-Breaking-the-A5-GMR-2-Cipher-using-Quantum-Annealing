import sys, time, glob, os
import numpy as np
import dimod
from dwave.samplers import SimulatedAnnealingSampler
PENALTY=1.1; GROUP=256; NG=8
def build_qubo(e):
    n=e.shape[0]; Q=np.zeros((n,n)); di=np.arange(n); Q[di,di]-=1.0
    for g in range(NG):
        s,t=g*GROUP,(g+1)*GROUP; Q[s:t,s:t]+=PENALTY; Q[np.arange(s,t),np.arange(s,t)]-=PENALTY
    nz=np.nonzero(e)
    for i,j in zip(nz[0],nz[1]): Q[i,i]+=PENALTY; Q[i,j]-=PENALTY
    Q=np.triu(Q)+np.tril(Q,-1).T; Q[np.tril_indices(n,-1)]=0.0; return Q
def qd(Q):
    nz=np.nonzero(Q); return {(int(i),int(j)):float(Q[i,j]) for i,j in zip(nz[0],nz[1])}
HERE = os.path.dirname(os.path.abspath(__file__))
def _project_root(start):
    d = start
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "Results_on_DWave")):
            return d
        d = os.path.dirname(d)
    raise RuntimeError("Could not locate Results_on_DWave above: " + start)
DWAVE_BASE = os.path.join(_project_root(HERE), "Results_on_DWave")
base=os.path.join(DWAVE_BASE,"_Experiment_Result_KS1")
f=sorted(glob.glob(os.path.join(base,"**","e_matrix_*.txt"),recursive=True))[0]
e=np.loadtxt(f,dtype=np.int8); bqm=dimod.BinaryQuadraticModel.from_qubo(qd(build_qubo(e)))
sa=SimulatedAnnealingSampler()
print("instance:",os.path.basename(f),"edges",int(e.sum()))
for reads,sweeps in [(1,1000),(1,5000),(1,10000),(1,20000),(10,1000),(20,1000),(50,1000)]:
    hit=0; ts=[]
    for _ in range(10):
        t=time.perf_counter(); ss=sa.sample(bqm,num_reads=reads,num_sweeps=sweeps); ts.append(time.perf_counter()-t)
        if ss.first.energy<-7.99: hit+=1
    print(f"reads={reads:3d} sweeps={sweeps:6d}  hit={hit}/10  wall={np.mean(ts)*1000:7.1f}ms")
