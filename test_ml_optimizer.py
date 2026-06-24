#!/usr/bin/env python3
"""Isolated test for ml_optimizer.py (no AdwinGUI / hardware required).

This script emulates the GUI side of the file handshake: it writes ml_config.txt,
launches ml_optimizer.py as a subprocess, then for each suggestion it computes a
known noisy cost and writes the result file -- exactly as MLOptimize.c would after
running a shot and reading the fitting computer's cost file. It checks that the
optimizer's best-found parameters land near the known optimum.

Run:  python test_ml_optimizer.py
"""

import os
import sys
import time
import shutil
import tempfile
import subprocess
import random


# Known test problem: maximize a noisy negative paraboloid whose peak is TARGET.
BOUNDS = [(-5.0, 5.0), (-5.0, 5.0), (-5.0, 5.0)]
TARGET = [1.5, -2.0, 0.5]
NOISE_STD = 0.5
N_CALLS = 60
N_INITIAL = 16
DIRECTION = 0          # maximize
POLL = 0.05
TIMEOUT = 120.0


def true_cost(x):
    base = -sum((xi - ti) ** 2 for xi, ti in zip(x, TARGET))
    return base + random.gauss(0.0, NOISE_STD)


def write_config(workdir):
    with open(os.path.join(workdir, "ml_config.txt"), "w") as f:
        f.write("n_params %d\n" % len(BOUNDS))
        f.write("n_initial_points %d\n" % N_INITIAL)
        f.write("n_calls %d\n" % N_CALLS)
        f.write("direction %d\n" % DIRECTION)
        for lo, hi in BOUNDS:
            f.write("bound %.10g %.10g\n" % (lo, hi))


def read_suggestion(workdir, n):
    """Mirror MLOptimize.c's ml_read_suggestion: poll, then parse OK/DONE."""
    path = os.path.join(workdir, "suggest_%05d.txt" % n)
    start = time.time()
    while not os.path.exists(path):
        if time.time() - start > TIMEOUT:
            raise TimeoutError("No suggestion %d" % n)
        time.sleep(POLL)
    for _ in range(50):
        with open(path) as f:
            lines = f.read().splitlines()
        if not lines:
            time.sleep(POLL)
            continue
        if lines[0].strip() == "DONE":
            return None
        try:
            return [float(v) for v in lines[1:1 + len(BOUNDS)]]
        except ValueError:
            time.sleep(POLL)
    raise ValueError("Malformed suggestion %d" % n)


def write_result(workdir, n, cost):
    tmp = os.path.join(workdir, "result_%05d.txt.tmp" % n)
    final = os.path.join(workdir, "result_%05d.txt" % n)
    with open(tmp, "w") as f:
        f.write("%.10g\n" % cost)
    os.replace(tmp, final)


def main():
    random.seed(0)
    workdir = tempfile.mkdtemp(prefix="ml_test_")
    os.makedirs(os.path.join(workdir, "plots"), exist_ok=True)
    write_config(workdir)

    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ml_optimizer.py")
    proc = subprocess.Popen([sys.executable, script, workdir])

    best_cost = None
    best_x = None
    try:
        n = 0
        while True:
            x = read_suggestion(workdir, n)
            if x is None:
                break
            c = true_cost(x)
            if best_cost is None or c > best_cost:
                best_cost, best_x = c, x
            write_result(workdir, n, c)
            n += 1
            if n > N_CALLS + 5:
                raise RuntimeError("Optimizer never signalled DONE")
    finally:
        proc.wait(timeout=30)

    err = sum((bi - ti) ** 2 for bi, ti in zip(best_x, TARGET)) ** 0.5
    print("\nTARGET        :", TARGET)
    print("best_x        :", ["%.3f" % v for v in best_x])
    print("best_cost     : %.4f" % best_cost)
    print("dist to target: %.3f" % err)

    # With noise present the recovered optimum should still be reasonably close.
    ok = err < 1.5
    print("\nRESULT:", "PASS" if ok else "FAIL")
    shutil.rmtree(workdir, ignore_errors=True)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
