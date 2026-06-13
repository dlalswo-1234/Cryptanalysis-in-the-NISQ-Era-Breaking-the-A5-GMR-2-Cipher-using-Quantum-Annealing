"""Extract the D-Wave hybrid-solver timing breakdown from the saved run logs.

Source : Results_on_DWave/_Experiment_Result_KS{1,2,3}/run_data/test_*/results.txt
         (written by the attack notebook for each of the 100 runs per #KS).
For each run it records:
    qpu_access_time  -- pure QPU time within the hybrid pipeline (the paper's "Time")
    run_time         -- D-Wave server-side total time for the hybrid solve
    charge_time      -- billed time
    Computation Time -- client-side wall-clock of the sample() call
Output : hybrid_timing.json   (per-#KS averages)

This is what separates "how much is really QPU" from the total hybrid cost:
    qpu_access ~ 102 ms  vs  run_time ~ 5.3 s  =>  QPU is ~1.9% of the hybrid solve.
The qpu_access averages reproduce Table 1 exactly (102.52 / 102.25 / 104.11 ms).
"""
import os, re, glob, json
import numpy as np

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
OUT = os.path.join(os.path.dirname(__file__), "hybrid_timing.json")

PATS = {
    "qpu_access_ms": re.compile(r"qpu_access_time:\s*([\d.]+)\s*ms"),
    "run_time_ms":   re.compile(r"run_time:\s*([\d.]+)\s*ms"),
    "charge_time_ms":re.compile(r"charge_time:\s*([\d.]+)\s*ms"),
    "client_wall_ms":re.compile(r"Computation Time:\s*([\d.]+)\s*s"),  # seconds -> ms below
}

def main():
    results = {}
    for ks, d in KS_DIRS.items():
        files = glob.glob(os.path.join(DWAVE_BASE, d, "run_data", "**", "results.txt"), recursive=True)
        acc = {k: [] for k in PATS}
        for f in files:
            txt = open(f, encoding="utf-8", errors="ignore").read()
            for key, pat in PATS.items():
                m = pat.search(txt)
                if m:
                    val = float(m.group(1))
                    if key == "client_wall_ms":
                        val *= 1000.0
                    acc[key].append(val)
        summ = {k: (float(np.mean(v)) if v else None) for k, v in acc.items()}
        summ["n"] = len(files)
        if summ["qpu_access_ms"] and summ["run_time_ms"]:
            summ["qpu_fraction_of_runtime_pct"] = 100.0 * summ["qpu_access_ms"] / summ["run_time_ms"]
        results[ks] = summ
        print(f"#KS={ks}: {json.dumps(summ)}", flush=True)
    json.dump(results, open(OUT, "w"), indent=2)
    print("WROTE", OUT, flush=True)

if __name__ == "__main__":
    main()
