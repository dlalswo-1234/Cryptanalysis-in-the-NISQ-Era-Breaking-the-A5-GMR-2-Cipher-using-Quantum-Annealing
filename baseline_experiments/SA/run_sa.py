"""Simulated Annealing baseline (classical analogue of quantum annealing).

Solver : dimod / dwave-samplers SimulatedAnnealingSampler, directly on the QUBO.
Target : reach the QUBO optimum E = -8 (one vertex per key byte, edge-consistent
         => a valid key candidate).
Env    : dwave-env   (conda activate dwave-env)
Usage  : python run_sa.py [num_instances=100] [num_reads=1] [num_sweeps=1000]
Output : sa_results.json   (mean/std/min/max wall-clock ms + optimum-hit count per #KS)

Reported in the paper (num_reads=1, num_sweeps=1000), mean wall-clock to optimum:
    #KS=1: 406 ms (opt. 9/100)   #KS=2: 396 ms (88/100)   #KS=3: 394 ms (97/100)
With num_reads=100 (= the 100 anneals of the QA runs): ~8.6-9.0 s, opt. 90-100%.
"""
import sys, os, time, glob, json
import numpy as np
import dimod
from dwave.samplers import SimulatedAnnealingSampler

PENALTY = 1.1; GROUP = 256; NG = 8
HERE = os.path.dirname(os.path.abspath(__file__))
def _project_root(start):
    d = start
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "Results_on_DWave")):
            return d
        d = os.path.dirname(d)
    raise RuntimeError("Could not locate Results_on_DWave above: " + start)
DWAVE_BASE = os.path.join(_project_root(HERE), "Results_on_DWave")
KS_DIRS = {1: "_Experiment_Result_KS1", 2: "_Experiment_Result_KS2", 3: "_Experiment_Result_KS3"}
OUT = os.path.join(os.path.dirname(__file__), "sa_results.json")

def build_qubo(e):
    """Exact replica of the notebook's _create_standard_qubo (upper-triangular)."""
    n = e.shape[0]; Q = np.zeros((n, n)); di = np.arange(n)
    Q[di, di] -= 1.0                                   # selection reward
    for g in range(NG):                                # one-per-group penalty
        s, t = g*GROUP, (g+1)*GROUP
        Q[s:t, s:t] += PENALTY
        Q[np.arange(s, t), np.arange(s, t)] -= PENALTY
    nz = np.nonzero(e)                                 # edge-implication penalty
    for i, j in zip(nz[0], nz[1]):
        Q[i, i] += PENALTY; Q[i, j] -= PENALTY
    Q = np.triu(Q) + np.tril(Q, -1).T                  # fold to upper triangular
    Q[np.tril_indices(n, -1)] = 0.0
    return Q

def qdict(Q):
    nz = np.nonzero(Q)
    return {(int(i), int(j)): float(Q[i, j]) for i, j in zip(nz[0], nz[1])}

def main():
    limit  = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    reads  = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    sweeps = int(sys.argv[3]) if len(sys.argv) > 3 else 1000
    sa = SimulatedAnnealingSampler(); results = {}
    for ks, d in KS_DIRS.items():
        files = sorted(glob.glob(os.path.join(DWAVE_BASE, d, "**", "e_matrix_*.txt"), recursive=True))[:limit]
        ms = []; ok = 0
        for k, f in enumerate(files):
            e = np.loadtxt(f, dtype=np.int8)
            bqm = dimod.BinaryQuadraticModel.from_qubo(qdict(build_qubo(e)))
            t = time.perf_counter()
            ss = sa.sample(bqm, num_reads=reads, num_sweeps=sweeps)
            ms.append((time.perf_counter()-t)*1000)
            if ss.first.energy < -7.99: ok += 1
            if (k+1) % 10 == 0:
                print(f"  [KS{ks}] {k+1}/{len(files)} mean={np.mean(ms):.1f}ms opt={ok}/{k+1}", flush=True)
        ms = np.array(ms)
        results[ks] = {"reads": reads, "sweeps": sweeps, "mean_ms": float(ms.mean()),
                       "std_ms": float(ms.std()), "min_ms": float(ms.min()),
                       "max_ms": float(ms.max()), "optimum_hit": ok, "n": len(files)}
        print(f"#KS={ks} DONE: {json.dumps(results[ks])}", flush=True)
    json.dump(results, open(OUT, "w"), indent=2)
    print("WROTE", OUT, flush=True)

if __name__ == "__main__":
    main()
