"""CP-SAT (constraint-programming / SAT-based) baseline.

Solver : OR-Tools CP-SAT.
Model  : same linear 0-1 model as the ILP baseline --
         maximize sum(y_v)  s.t.  one value per key byte  and  edge implications.
         optimum = 8  <=>  QUBO energy -8  <=>  a valid key candidate.
Env    : dwave-env  (or any env with ortools)
Usage  : python run_cpsat.py [num_instances=100]
Output : cpsat_results.json  (mean/std/min/max wall-clock ms + #optimal per #KS)

Reported in the paper (mean wall-clock to optimum, opt. 100/100 on every #KS):
    #KS=1: 86 ms   #KS=2: 41 ms   #KS=3: 47 ms
"""
import sys, os, time, glob, json
import numpy as np
from ortools.sat.python import cp_model

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
OUT = os.path.join(os.path.dirname(__file__), "cpsat_results.json")

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
            cm = cp_model.CpModel()
            y = [cm.NewBoolVar(f"y{i}") for i in range(NVARS)]
            for g in range(NG):
                cm.Add(sum(y[g*GROUP:(g+1)*GROUP]) <= 1)
            for (i, j) in edges_from_e(e):
                cm.Add(y[i] <= y[j])
            cm.Maximize(sum(y))
            slv = cp_model.CpSolver()
            t = time.perf_counter()
            rc = slv.Solve(cm)
            ms.append((time.perf_counter()-t)*1000)
            if rc == cp_model.OPTIMAL and abs(slv.ObjectiveValue()-8) < 1e-6:
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
