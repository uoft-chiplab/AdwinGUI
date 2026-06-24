#!/usr/bin/env python3
"""ML optimization helper for AdwinGUI (see Section 4.7.5 of Beregi 2024).

Runs scikit-optimize's Bayesian optimizer (Gaussian process, the package's
gp_minimize estimator) in an ask/tell loop and exchanges files with the
LabWindows CVI sequencer (MLOptimize.c). The GUI owns the experiment loop; this
process only suggests parameter vectors and is told the measured cost.

File protocol (all inside <workdir>):
    ml_config.txt        GUI -> here (once)
        n_params N
        n_initial_points I
        n_calls T
        direction D          # 0 = maximize, 1 = minimize
        bound LO HI          # repeated N times, in parameter order
    suggest_%05d.txt     here -> GUI
        line 1: "OK" or "DONE"
        next N lines: one parameter value each (only when "OK")
    result_%05d.txt      GUI -> here
        line 1: measured (raw) cost for that shot

Usage:  python ml_optimizer.py <workdir>
"""

import os
import sys
import time

try:
    from skopt import Optimizer
except ImportError:
    sys.stderr.write(
        "ERROR: scikit-optimize is required. Install with:\n"
        "    pip install scikit-optimize\n"
    )
    sys.exit(1)


RESULT_POLL_SECONDS = 0.1
RESULT_TIMEOUT_SECONDS = 3600.0   # give up waiting for a single result after this


def read_config(workdir):
    cfg = {"bounds": []}
    path = os.path.join(workdir, "ml_config.txt")
    with open(path) as f:
        for line in f:
            parts = line.split()
            if not parts:
                continue
            key = parts[0]
            if key == "bound":
                cfg["bounds"].append((float(parts[1]), float(parts[2])))
            elif key in ("n_params", "n_initial_points", "n_calls", "direction"):
                cfg[key] = int(parts[1])
    return cfg


def write_suggest(workdir, n, x):
    tmp = os.path.join(workdir, "suggest_%05d.txt.tmp" % n)
    final = os.path.join(workdir, "suggest_%05d.txt" % n)
    with open(tmp, "w") as f:
        f.write("OK\n")
        for v in x:
            f.write("%.10g\n" % v)
    os.replace(tmp, final)   # atomic: GUI never sees a half-written file


def write_done(workdir, n):
    tmp = os.path.join(workdir, "suggest_%05d.txt.tmp" % n)
    final = os.path.join(workdir, "suggest_%05d.txt" % n)
    with open(tmp, "w") as f:
        f.write("DONE\n")
    os.replace(tmp, final)


def wait_for_result(workdir, n):
    path = os.path.join(workdir, "result_%05d.txt" % n)
    start = time.time()
    while not os.path.exists(path):
        if time.time() - start > RESULT_TIMEOUT_SECONDS:
            raise TimeoutError("Timed out waiting for %s" % path)
        time.sleep(RESULT_POLL_SECONDS)
    # The GUI may still be flushing; retry parse briefly.
    for _ in range(50):
        try:
            with open(path) as f:
                return float(f.readline().strip())
        except (ValueError, OSError):
            time.sleep(RESULT_POLL_SECONDS)
    raise ValueError("Could not parse cost from %s" % path)


def save_outputs(workdir, opt):
    """Dump the evaluation history and (if matplotlib is available) the
    scikit-optimize objective plot for diagnostics."""
    try:
        res = opt.get_result()
    except Exception as exc:                       # pragma: no cover
        sys.stderr.write("Could not build result: %s\n" % exc)
        return

    try:
        with open(os.path.join(workdir, "ml_result.csv"), "w") as f:
            ndim = len(res.x_iters[0]) if res.x_iters else 0
            f.write("iter," + ",".join("p%d" % (i + 1) for i in range(ndim)) + ",func_val\n")
            for i, (x, fv) in enumerate(zip(res.x_iters, res.func_vals)):
                f.write("%d,%s,%.10g\n" % (i, ",".join("%.10g" % v for v in x), fv))
        with open(os.path.join(workdir, "ml_best.txt"), "w") as f:
            f.write("best_x %s\n" % " ".join("%.10g" % v for v in res.x))
            f.write("best_func_val %.10g\n" % res.fun)
    except OSError as exc:                          # pragma: no cover
        sys.stderr.write("Could not write result files: %s\n" % exc)

    try:                                            # pragma: no cover - plotting optional
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from skopt.plots import plot_objective
        plot_objective(res)
        plt.savefig(os.path.join(workdir, "plots", "ml_objective.png"))
        plt.close("all")
    except Exception as exc:
        sys.stderr.write("Skipping objective plot: %s\n" % exc)


def main():
    if len(sys.argv) < 2:
        sys.stderr.write("Usage: python ml_optimizer.py <workdir>\n")
        sys.exit(2)
    workdir = sys.argv[1]

    cfg = read_config(workdir)
    bounds = cfg["bounds"]
    n_calls = cfg.get("n_calls", 80)
    n_initial = cfg.get("n_initial_points", 16)
    maximize = (cfg.get("direction", 0) == 0)

    print("ML optimizer: %d params, %d calls, %d init points, %s" % (
        len(bounds), n_calls, n_initial, "maximize" if maximize else "minimize"))

    opt = Optimizer(
        dimensions=bounds,
        base_estimator="GP",
        n_initial_points=n_initial,
        initial_point_generator="sobol",
        acq_func="gp_hedge",
    )

    for n in range(n_calls):
        x = opt.ask()
        write_suggest(workdir, n, x)
        cost = wait_for_result(workdir, n)
        # gp_minimize minimizes, so negate when the experiment should be maximized.
        y = -cost if maximize else cost
        opt.tell(x, y)
        print("iter %d: cost=%.6g  x=%s" % (n, cost, ["%.4g" % v for v in x]))

    write_done(workdir, n_calls)
    save_outputs(workdir, opt)
    print("ML optimizer finished.")


if __name__ == "__main__":
    main()
