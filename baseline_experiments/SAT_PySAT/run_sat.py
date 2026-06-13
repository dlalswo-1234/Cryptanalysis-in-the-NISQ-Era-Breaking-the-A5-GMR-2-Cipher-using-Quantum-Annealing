"""SAT (CDCL) baseline.

Solver : PySAT / Glucose3 (CDCL).
Model  : Boolean feasibility of the key-candidate constraints --
         exactly-one selected vertex per key byte (at-least-one + at-most-one via
         a sequential-counter cardinality encoding) and edge implications
         (~x_i v x_j). A satisfying assignment <=> QUBO energy -8 <=> key candidate.
Env    : dcsat  (conda activate dcsat ; has python-sat)
Usage  : python run_sat.py [num_instances=100]
Output : sat_results.json  (mean/std/min/max solve-time ms + #SAT per #KS)

Reported in the paper (mean solve wall-clock, all SAT/100 on every #KS):
    #KS=1: 0.96 ms   #KS=2: 0.21 ms   #KS=3: 0.21 ms
"""
import sys, os, time, glob, json
import numpy as np
from pysat.formula import CNF, IDPool
from pysat.card import CardEnc, EncType
from pysat.solvers import Glucose3

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
OUT = os.path.join(os.path.dirname(__file__), "sat_results.json")

def build_cnf(e):
    cnf = CNF(); vpool = IDPool(start_from=NVARS + 1)
    for g in range(NG):                                    # exactly-one per key byte
        lits = list(range(g*GROUP + 1, (g+1)*GROUP + 1))
        cnf.append(lits)                                   #  at-least-one
        am = CardEnc.atmost(lits=lits, bound=1, vpool=vpool, encoding=EncType.seqcounter)
        cnf.extend(am.clauses)                             #  at-most-one
    nz = np.nonzero(e)                                     # edge implications
    for i, j in zip(nz[0], nz[1]):
        cnf.append([-(int(i)+1), int(j)+1])                #  x_i -> x_j
    return cnf

def main():
    limit = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    results = {}
    for ks, d in KS_DIRS.items():
        files = sorted(glob.glob(os.path.join(DWAVE_BASE, d, "**", "e_matrix_*.txt"), recursive=True))[:limit]
        ms = []; ok = 0
        for k, f in enumerate(files):
            e = np.loadtxt(f, dtype=np.int8)
            cnf = build_cnf(e)
            s = Glucose3(bootstrap_with=cnf.clauses)
            t = time.perf_counter()
            sat = s.solve()
            ms.append((time.perf_counter()-t)*1000)
            if sat:
                model = s.get_model()
                sel = [v for v in range(NVARS) if model[v] > 0]
                groups = sorted(set(v // GROUP for v in sel))
                ok += int(len(sel) == NG and groups == list(range(NG)))
            s.delete()
            if (k+1) % 10 == 0:
                print(f"  [KS{ks}] {k+1}/{len(files)} mean={np.mean(ms):.3f}ms SAT={ok}/{k+1}", flush=True)
        ms = np.array(ms)
        results[ks] = {"mean_ms": float(ms.mean()), "std_ms": float(ms.std()),
                       "min_ms": float(ms.min()), "max_ms": float(ms.max()),
                       "sat": ok, "n": len(files)}
        print(f"#KS={ks} DONE: {json.dumps(results[ks])}", flush=True)
    json.dump(results, open(OUT, "w"), indent=2)
    print("WROTE", OUT, flush=True)

if __name__ == "__main__":
    main()
