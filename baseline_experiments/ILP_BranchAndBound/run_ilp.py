"""Integer Linear Programming / branch-and-bound baseline.

Solver : OR-Tools CBC (MILP solved by branch-and-bound).
Model  : the linear 0-1 equivalent of the penalty QUBO --
         maximize sum(x_v)  s.t.  sum_{v in byte g} x_v <= 1  (one value per key byte)
                                   x_i <= x_j  for every edge (v_i -> v_j)  (implication)
         optimum = 8 (one per byte, edge-consistent)  <=>  QUBO energy -8  <=>  key candidate.
Env    : dwave-env  (or any env with ortools)
Usage  : python run_ilp.py [num_instances=100]
Output : ilp_results.json  (mean/std/min/max wall-clock ms + #optimal per #KS)

Reported in the paper (mean wall-clock to optimum, opt. 100/100 on every #KS):
    #KS=1: 181 ms   #KS=2: 563 ms   #KS=3: 544 ms
"""
import sys, os, time, glob, json
import numpy as np
from ortools.linear_solver import pywraplp

GROUP = 256; NG = 8; NVARS = 2048
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
OUT = os.path.join(os.path.dirname(__file__), "ilp_results.json")

def edges_from_e(e):
    nz = np.nonzero(e)
    return list(zip(nz[0].tolist(), nz[1].tolist()))

def main():
    limit = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    results = {}
    for ks, d in KS_DIRS.items():
        files = sorted(glob.glob(os.path.join(DWAVE_BASE, d, "**", "e_matrix_*.txt"), recursive=True))[:limit]
        ms = []; ok = 0
        for k, f in enumerate(files):
            e = np.loadtxt(f, dtype=np.int8)
            solver = pywraplp.Solver.CreateSolver("CBC")
            x = [solver.BoolVar(f"x{i}") for i in range(NVARS)]
            for g in range(NG):
                solver.Add(sum(x[g*GROUP:(g+1)*GROUP]) <= 1)
            for (i, j) in edges_from_e(e):
                solver.Add(x[i] <= x[j])
            solver.Maximize(sum(x))
            t = time.perf_counter()
            st = solver.Solve()
            ms.append((time.perf_counter()-t)*1000)
            if st == pywraplp.Solver.OPTIMAL and abs(solver.Objective().Value()-8) < 1e-6:
                ok += 1
            if (k+1) % 10 == 0:
                print(f"  [KS{ks}] {k+1}/{len(files)} mean={np.mean(ms):.1f}ms opt={ok}/{k+1}", flush=True)
        ms = np.array(ms)
        results[ks] = {"mean_ms": float(ms.mean()), "std_ms": float(ms.std()),
                       "min_ms": float(ms.min()), "max_ms": float(ms.max()),
                       "optimal": ok, "n": len(files)}
        print(f"#KS={ks} DONE: {json.dumps(results[ks])}", flush=True)
    json.dump(results, open(OUT, "w"), indent=2)
    print("WROTE", OUT, flush=True)

if __name__ == "__main__":
    main()
