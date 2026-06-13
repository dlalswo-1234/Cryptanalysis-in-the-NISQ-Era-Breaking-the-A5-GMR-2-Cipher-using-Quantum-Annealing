# Classical baseline experiments (paper Table 6)

Wall-clock comparison of classical solvers against the D-Wave hybrid solver, on the
**identical residual instances**: the 100 saved graphs per `#KS` in
`Results_on_DWave/_Experiment_Result_KS{1,2,3}/.../e_matrix_*.txt` that were
actually submitted to the annealer. Each method must reach the optimum
`E = -8` (one vertex per key byte, edge-consistent) = a **valid key candidate**.

Every script is **self-contained** (no shared module) and points at the absolute
`Results_on_DWave` path. Run with no argument for the full 100 instances/`#KS`,
or pass a smaller count as the first argument for a quick check.

## Folders (one method each)

| Folder | Method | Solver | Env | Script | Result |
|--------|--------|--------|-----|--------|--------|
| `SA/`                  | Simulated annealing (on the QUBO) | dimod `SimulatedAnnealingSampler` | `dwave-env` | `run_sa.py` | `sa_results.json`, `sa_100reads_results.json` |
| `ILP_BranchAndBound/`  | Integer LP solved by branch-and-bound | OR-Tools CBC | `dwave-env` | `run_ilp.py` | `ilp_results.json` |
| `CP-SAT/`              | Constraint programming / SAT-based | OR-Tools CP-SAT | `dwave-env` | `run_cpsat.py` | `cpsat_results.json` |
| `SAT_PySAT/`           | CDCL SAT (Boolean encoding) | PySAT / Glucose3 | `dcsat` | `run_sat.py` | `sat_results.json` |
| `Hybrid_DWave/`        | D-Wave hybrid timing breakdown | (parses run logs) | any | `extract_hybrid_timing.py` | `hybrid_timing.json` |

`SA/tuning/` holds the sweep/read tuning scripts used to pick `num_reads`/`num_sweeps`.
`_combined_source/` holds the original all-in-one runner and the raw combined JSON
results from which the per-method files were split (kept for provenance).

## How to run

```powershell
$dw = "C:\Users\insung\anaconda3\envs\dwave-env\python.exe"
$dc = "C:\Users\insung\anaconda3\envs\dcsat\python.exe"
& $dw SA\run_sa.py                 # SA, 1 read x 1000 sweeps, 100 inst/#KS
& $dw SA\run_sa.py 100 100 1000    # SA, 100 reads (matches the 100 QA anneals)
& $dw ILP_BranchAndBound\run_ilp.py
& $dw CP-SAT\run_cpsat.py
& $dc SAT_PySAT\run_sat.py
& $dw Hybrid_DWave\extract_hybrid_timing.py
```

## Equivalence note

SA runs on the penalty QUBO directly. ILP/CP-SAT/SAT use the natural linear/Boolean
equivalent: maximize the number of selected vertices subject to one-per-key-byte and
edge-implication constraints. Optimum 8 selected  <=>  QUBO ground state `E = -8`
<=>  a valid key candidate, so all methods solve the same problem.

## Headline numbers (mean wall-clock to optimum, 100 instances/#KS)

| Method | #KS=1 | #KS=2 | #KS=3 | optimum reached |
|--------|------:|------:|------:|-----------------|
| Hybrid `qpu_access_time` (QPU only) | 102.52 ms | 102.25 ms | 104.11 ms | 100/100 |
| Hybrid `run_time` (server total)    | 5329 ms   | 5328 ms   | 5329 ms   | 100/100 |
| SAT (PySAT/Glucose3)                | 0.96 ms   | 0.21 ms   | 0.21 ms   | 100/100 |
| CP-SAT (OR-Tools)                   | 86 ms     | 41 ms     | 47 ms     | 100/100 |
| ILP / branch-and-bound (CBC)        | 181 ms    | 563 ms    | 544 ms    | 100/100 |
| SA (1 read x 1000 sweeps)           | 406 ms    | 396 ms    | 394 ms    | 9 / 88 / 97 (/100) |
| SA (100 reads x 1000 sweeps)        | 9021 ms   | 8733 ms   | 8611 ms   | 9 / 10 / 10 (/10)  |

- The three exact solvers reach the optimum on every one of the 300 instances.
- The hybrid `qpu_access_time` averages reproduce paper Table 1 exactly.
- Of the hybrid `run_time` (~5.3 s), the QPU is only ~1.9%; the remaining ~98% is
  classical computation inside the hybrid pipeline (see `hybrid_timing.json`).
- Conclusion: no quantum speedup; the residual problem is easy for classical
  solvers, several of which beat the annealer.
