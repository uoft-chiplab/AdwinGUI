# Machine-Learning Optimization

Automated optimization of experimental parameters for AdwinGUI, modelled on
**Section 4.7.5 of Beregi (2024)** (`docs/Beregi_2024_Probing_universality_of.pdf`).
The sequencer varies a set of user-chosen parameters within bounds, runs the
experiment, reads a cost/fitness value produced by a separate atom-cloud fitting
computer, and uses **Bayesian optimization with a Gaussian process**
(scikit-optimize's `gp_minimize` estimator) to converge on the parameters that
optimize that cost.

This optimizer was chosen because, as the thesis found, it **tolerates the noise**
present in a real experiment and converges substantially faster than differential
evolution or a noise-naïve Bayesian optimizer.

---

## 1. Architecture

The GUI owns the experiment loop; an external Python process runs the optimizer.
They communicate only through files in a per-run working directory.

```
   GUI (GUIDesign.exe, C / CVI)                  python ml_optimizer.py (scikit-optimize)
   ────────────────────────────                  ─────────────────────────────────────────
   Start: write ml_config.txt   ───────────────► read config (bounds, n_init, n_calls, dir)
          launch python (cmd /c, output→log)
   poll suggest_0000N.txt        ◄────────────── x = opt.ask();  write suggest_0000N.txt
   apply x to device tables, RunOnce()
   (repeat K shots per suggestion, settle+read
    cost file each shot, take the median)
   write result_0000N.txt        ───────────────► read cost;  opt.tell(x, ±cost)
   (loop)                                          after n_calls: write suggest w/ DONE token
   finish: restore originals, write ml_log.csv     save ml_result.csv, ml_best.txt, plot
```

- **Loop ownership:** the C GUI applies parameters, runs shots, waits, reads the cost,
  and reports `(params, cost)` to Python. Python only does the optimizer `ask`/`tell`.
- **Parameter selection** reuses the existing MultiScan machinery: right-click a cell →
  *"Scan cell value"* populates the position table; the ML panel reads from it.

Relevant source: `MLOptimize.c` / `MLOptimize.h`, the `MLOpt` struct in `vars.h`,
hooks in `GUIDesign.c` (`TIMER_CALLBACK`, `CMDSTOP_CALLBACK`) and `main.c`
(`BuildMLOptPanel`), the shared `applyValueToScannedCell` in `multiscan.c`, and the
Python optimizer `ml_optimizer.py` (test harness: `test_ml_optimizer.py`).

---

## 2. The mathematics of the optimization

### 2.1 The problem

We want to optimize an unknown, expensive, noisy function of $n$ parameters

$$
\mathbf{x} = (x_1, \dots, x_n) \in \prod_{i=1}^{n} [\,l_i, u_i\,] \subset \mathbb{R}^n ,
$$

where $[l_i,u_i]$ are the **lower/upper bounds** you enter per parameter. Each
"evaluation" is a sequence shot whose result $y$ (atom number, integrated OD, …) is
measured by the fitting computer. The measurement is **noisy**:

$$
y = f(\mathbf{x}) + \varepsilon, \qquad \varepsilon \sim \mathcal{N}(0,\sigma_n^2),
$$

and the underlying $f$ has no analytic form or gradient (a *black box*). By convention
scikit-optimize **minimizes**, so to **maximize** a signal $f$ we optimize $g=-f$
(handled automatically — see §2.6).

Because each evaluation costs an experimental cycle, we want to find the optimum in as
**few evaluations** as possible. Bayesian optimization is built for exactly this:
sample-efficient global optimization of expensive black boxes.

### 2.2 Gaussian-process surrogate model

Bayesian optimization builds a cheap probabilistic *surrogate* of $f$ and optimizes
that instead, re-fitting as data arrives. The surrogate is a **Gaussian process (GP)**:
a distribution over functions in which any finite set of function values is jointly
Gaussian. It is specified by a mean function (taken as $0$ after the data are
normalized) and a covariance/kernel function $k(\mathbf{x},\mathbf{x}')$:

$$
f(\mathbf{x}) \sim \mathcal{GP}\!\big(0,\; k(\mathbf{x},\mathbf{x}')\big).
$$

scikit-optimize's default GP (used here) employs a **Matérn kernel** with smoothness
$\nu = 5/2$, scaled by a constant, plus a white-noise term:

$$
k(\mathbf{x},\mathbf{x}')
= \sigma_f^2\left(1 + \frac{\sqrt{5}\,r}{\ell} + \frac{5\,r^2}{3\,\ell^2}\right)
  \exp\!\left(-\frac{\sqrt{5}\,r}{\ell}\right)
  \;+\; \sigma_n^2\,\delta_{\mathbf{x},\mathbf{x}'},
\qquad r = \lVert \mathbf{x}-\mathbf{x}' \rVert .
$$

- $\ell$ — **length scale(s)** (one per dimension; "how far you must move before the
  output changes appreciably"),
- $\sigma_f^2$ — **signal variance** (output scale),
- $\sigma_n^2$ — **observation-noise variance** (the white-noise / `WhiteKernel` term).

The Matérn-5/2 kernel produces twice-differentiable sample paths — smooth enough to be
useful, rough enough to model real apparatus responses without over-smoothing.

### 2.3 Posterior (the prediction with uncertainty)

Given $N$ observations $\mathcal{D} = \{(\mathbf{x}_i, y_i)\}_{i=1}^{N}$, define the
kernel (Gram) matrix $K$ with $K_{ij} = k(\mathbf{x}_i,\mathbf{x}_j)$ and, for a query
point $\mathbf{x}_\*$, the vector $\mathbf{k}_\* = [\,k(\mathbf{x}_\*,\mathbf{x}_i)\,]_i$.
The GP posterior at $\mathbf{x}_\*$ is Gaussian with

$$
\mu(\mathbf{x}_\*) = \mathbf{k}_\*^{\mathsf T}\,(K + \sigma_n^2 I)^{-1}\,\mathbf{y},
$$
$$
\sigma^2(\mathbf{x}_\*) = k(\mathbf{x}_\*,\mathbf{x}_\*)
   - \mathbf{k}_\*^{\mathsf T}\,(K + \sigma_n^2 I)^{-1}\,\mathbf{k}_\* .
$$

$\mu$ is the surrogate's best guess of the cost surface; $\sigma$ is its uncertainty —
small near observed points, large in unexplored regions. **This uncertainty is what
makes the search intelligent**: it tells the optimizer where it might be worth looking.

The kernel hyperparameters $\theta = (\ell, \sigma_f, \sigma_n)$ are fit by maximizing
the **log marginal likelihood**

$$
\log p(\mathbf{y}\mid X,\theta)
= -\tfrac12\,\mathbf{y}^{\mathsf T}(K+\sigma_n^2 I)^{-1}\mathbf{y}
  -\tfrac12\,\log\!\lvert K+\sigma_n^2 I\rvert
  -\tfrac{N}{2}\log 2\pi .
$$

The first term rewards fitting the data; the second penalizes model complexity — an
automatic Occam's razor. Crucially, the fitted $\sigma_n^2 > 0$ means the GP **does not
interpolate every point exactly**: it treats each measurement as noisy, which is the
root of its robustness to experimental fluctuations (see §2.7).

### 2.4 Acquisition function (where to sample next)

Optimizing $f$ directly is replaced by optimizing a cheap **acquisition function**
$a(\mathbf{x})$ built from the GP posterior, which balances **exploitation** (sample
where $\mu$ is good) against **exploration** (sample where $\sigma$ is large). With
$f_{\text{best}}$ the best value seen so far and (for minimization)
$z = \dfrac{f_{\text{best}} - \mu(\mathbf{x}) - \xi}{\sigma(\mathbf{x})}$:

- **Expected Improvement (EI):**

$$
a_{\text{EI}}(\mathbf{x}) = \big(f_{\text{best}} - \mu(\mathbf{x}) - \xi\big)\,\Phi(z)
   + \sigma(\mathbf{x})\,\phi(z),
$$

  where $\Phi,\phi$ are the standard normal CDF and PDF and $\xi\ge 0$ tunes
  exploration. EI is the expected amount by which a new sample beats the incumbent.

- **Lower Confidence Bound (LCB):** $a_{\text{LCB}}(\mathbf{x}) = \mu(\mathbf{x}) - \kappa\,\sigma(\mathbf{x})$
  — optimistic in the face of uncertainty; $\kappa$ sets the explore/exploit trade-off.

- **Probability of Improvement (PI):** $a_{\text{PI}}(\mathbf{x}) = \Phi(z)$.

The optimizer used here defaults to **`gp_hedge`**: it runs EI, LCB and PI each
iteration and selects among their candidate points using a probabilistic
(Hedge-style) weighting that adapts to which strategy has been paying off — so you
don't have to commit to one acquisition function.

### 2.5 Initial design (Sobol sampling)

A GP needs data before its acquisition function is meaningful. The first
$n_{\text{init}}$ evaluations are therefore a space-filling **Sobol sequence** — a
low-discrepancy quasi-random design that covers $[0,1]^n$ far more uniformly than
independent random draws (fewer clusters and gaps). The thesis heuristic, exposed as
**Init points (Sobol)** in the panel, is

$$
n_{\text{init}} \approx 2^{\big\lceil \log_2 (10 n) \big\rceil}
\quad\text{(about 10 samples per parameter, rounded to a power of two).}
$$

Powers of two are used because the balance properties of the Sobol sequence are
strongest at $2^k$ points. After this exploration phase the GP takes over.

### 2.6 The ask/tell loop and the maximize/minimize sign

The run is an **ask–tell** loop (`skopt.Optimizer`):

1. `x = opt.ask()` — for the first $n_{\text{init}}$ calls this is the next Sobol point;
   thereafter it is $\arg\min a(\mathbf{x})$ from the current GP.
2. The GUI applies $\mathbf{x}$, runs the experiment, obtains a cost $c$.
3. `opt.tell(x, y)` updates the GP, where

$$
y = \begin{cases} -c & \text{Objective = Maximize} \\ \;\;c & \text{Objective = Minimize}\end{cases}
$$

   because the optimizer minimizes internally. **You always write the raw cost with its
   natural sign; the Objective setting handles negation.**

After $n_{\text{calls}}$ iterations Python emits a `DONE` token and the run ends.

### 2.7 Robustness to noise and unreproducible spikes

Two mechanisms protect the optimization against bad measurements:

1. **The GP noise model.** The fitted observation-noise variance $\sigma_n^2$ means a
   single anomalous point is partly *explained away as noise* rather than believed as a
   true feature. The optimizer also re-samples promising regions, so a one-off spike is
   **self-correcting**: follow-up shots that come back normal pull the model back.

2. **Median aggregation per point (optional).** Set **Shots per point** $K>1$ and the
   GUI runs each suggested $\mathbf{x}$ $K$ times and reports the **median** cost

$$
c = \operatorname{median}\big(c^{(1)}, \dots, c^{(K)}\big)
$$

   to the optimizer. The median is **robust to outliers**: one wild value among $K$
   replicates does not move it (unlike the mean). For Gaussian per-shot noise of
   variance $\sigma^2$, the sampling variance of the median scales roughly as

$$
\operatorname{Var}\big[\operatorname{median}\big] \;\approx\; \frac{\pi}{2}\,\frac{\sigma^2}{K},
$$

   i.e. it shrinks like $1/K$ (a small constant-factor cost versus the mean, bought in
   exchange for outlier robustness). A smaller effective noise $\sigma_n$ lets the GP
   trust its data more and converge faster — at the price of $K\times$ more shots.

> **Caveat — "best observed" is not robust.** The panel's *Best so far* line and
> Python's `ml_best.txt` report the single **best observed** point, which by definition
> can latch onto a lucky outlier. Trust the **region** the optimizer converged to
> (see `ml_log.csv` and `plot_objective`), and **re-measure your final candidate** a
> few times before adopting it.

### 2.8 Multiple objectives

`gp_minimize` is **single-objective**. To optimize two signals at once (e.g. maximize
$N$ *and* minimize width $w$), combine them into one scalar **figure of merit** on the
fitting computer and write that one number, for example

$$
\text{merit} = \frac{N}{w^k}\quad(k=2,3),
\qquad\text{or}\qquad
\text{merit} = w_N \frac{N}{N_\text{ref}} - w_w \frac{w}{w_\text{ref}} ,
$$

with reference values $N_\text{ref}, w_\text{ref}$ chosen so each term is $O(1)$ before
weighting. Select **Maximize**. A true Pareto front would require a different optimizer
(e.g. Optuna NSGA-II / pymoo) and a vector-valued cost — not currently implemented.

---

## 3. Using the ML panel

### 3.1 One-time setup

- Install the optimizer on the machine that runs it: `pip install scikit-optimize`
  (optionally `matplotlib` for the diagnostic plot).
- Know the full path to the `python.exe` that has scikit-optimize installed, and to
  `ml_optimizer.py`.

### 3.2 Run procedure

1. **Build your sequence** normally.
2. **Select parameters:** right-click each analog/DDS/time/laser/digital cell to
   optimize → **"Scan cell value"** (the same action as MultiScan).
3. Open the panel: menu **MLOpt → Open ML Optimizer**.
4. Press **Refresh Params** — the table fills with one row per selected cell.
5. Enter **Lower bound** and **Upper bound** for each parameter (columns 5 and 6).
6. Fill in the settings (§3.3).
7. Press **Start** — confirm the *Save Settings* dialog (reused from MultiScan; it still
   says *"…Multi-parameter Scan?"*). The GUI saves the `.seq`, makes a run folder,
   launches Python, applies the first suggestion and begins cycling.
8. Watch **Status** / **Best so far**. **Stop** ends early and restores original cell
   values. **Export Log** copies `ml_log.csv` elsewhere.

### 3.3 Panel fields

| Field | Meaning | Typical |
|---|---|---|
| **Parameters table** | One row per selected cell. Cols: Parameter, Page, Column, Row (auto), **Lower bound**, **Upper bound** (you enter). | — |
| **Cost file template** | Path the fitting PC writes; **must contain a `printf` integer field** for the shot number, e.g. `Z:\fit\cost_%05d.txt`. | your path |
| **auto-generate path upon start?** | When checked, the cost template is overwritten at **Start** with `<run folder>\imgs\cost_%03d.txt` (the `imgs` subfolder created next to the saved `.seq`). On by default. | checked |
| **Cost parse format** | `sscanf` format to extract the number from the file's first line. | `%lf` |
| **Settle delay (ms)** | Dwell after each shot before reading its cost. | 200–1000 |
| **Cost timeout (ms)** | Max wait for a cost/suggestion file before aborting. | 30000 |
| **Init points (Sobol)** | $n_{\text{init}}$ exploration shots (§2.5). | $\approx 2^{\lceil\log_2 10n\rceil}$ |
| **Total iterations** | $n_{\text{calls}}$ optimizer evaluations. | 64–128 |
| **Objective** | Maximize or Minimize (handles the sign, §2.6). | Maximize |
| **Shots per point (median)** | $K$ physical shots per suggestion; median reported (§2.7). 1 = off. | 1, or 3–5 |
| **Python exe / Optimizer script** | Full paths to `python.exe` and `ml_optimizer.py`. | — |

> Total shots $= n_{\text{calls}} \times K$. With $K=5$ and 80 iterations that is 400
> shots — balance against long-term drift over the run.

---

## 4. The cost file (fitting-computer contract)

For **physical shot** $m$ (starting at 0), write the cost to the file named by your
template with $m$ substituted: `cost_00000.txt`, `cost_00001.txt`, …

- **One cost file per physical shot, in order, starting at 0.** The shot counter
  increments **every** shot regardless of $K$, so with $K=3$: suggestion 0 reads
  `cost_00000/00001/00002`, suggestion 1 reads `cost_00003/00004/00005`, … The fitting
  PC does not need to know $K$.
- **First line is the number.** With `%lf` the file is simply `12453.7`. Use a different
  parse format only to skip a label/columns:

  | First line | Cost parse format |
  |---|---|
  | `12453.7` | `%lf` |
  | `atomN=12453.7` | `atomN=%lf` |
  | `0.84, 12453.7` (want 2nd) | `%*lf, %lf` |

- **Sign:** write the **raw** value (positive for OD/atom number) and pick the Objective;
  do **not** pre-negate.
- **Failed fits:** write a finite "bad" value (e.g. `0` for a maximization), never a
  blank, `NaN`, or `inf` — a blank stops the run; `NaN`/`inf` corrupt the GP.
- **Freshness:** the GUI polls for the file's existence then reads it. The **Settle
  delay** absorbs write latency; for safety have the fitting PC write to a temp name and
  **rename** into place (rename is atomic) so the file appears only when complete.

---

## 5. File protocol (the working directory)

All handshake files live in the **`commands/`** subfolder of the run folder created at
the Save Settings step:

| File | Direction | Contents |
|---|---|---|
| `ml_config.txt` | GUI → Py | `n_params`, `n_initial_points`, `n_calls`, `direction`, then `bound LO HI` per parameter |
| `suggest_0000N.txt` | Py → GUI | line 1 `OK` or `DONE`; then one parameter value per line |
| `result_0000N.txt` | GUI → Py | the (median) cost for suggestion N |
| `ml_log.csv` | GUI | `iter,cost,best,p1..pN,time` — per-suggestion record |
| `ml_python.log` | Py | optimizer stdout/stderr (config read, each ask, errors) |
| `ml_result.csv`, `ml_best.txt` | Py | full evaluation history and best-observed point |
| `plots/ml_objective.png` | Py | `skopt.plots.plot_objective` diagnostic (if matplotlib present) |

Writes use a temp-then-rename pattern so the reader never sees a partial file.

---

## 6. Troubleshooting

- **"failed to launch python optimizer"** — `LaunchExecutableEx` couldn't start the
  process. Use the **full path** to `python.exe` and to `ml_optimizer.py`.
- **`'python' is not recognized`** (or no progress, then "no initial suggestion") — the
  Python exe isn't on `PATH`. Find it (`where python` in an Anaconda Prompt, or
  `py -0p`) and put the **full path** in the Python exe field; ensure that interpreter
  has scikit-optimize (`"C:\...\python.exe" -c "import skopt"`).
- **No console window appears** — expected: the GUI has no console, so optimizer output
  is redirected to **`commands/ml_python.log`**. Tail it with Notepad++ or
  `Get-Content -Wait`.
- **"no initial suggestion from optimizer"** — Python started but didn't produce
  `suggest_00000.txt` in time. Read `ml_python.log` (missing package, bad script path,
  config parse error).
- **"could not parse cost value"** — the cost file's first line didn't match the parse
  format. Check the file content and the **Cost parse format**.
- **"Each upper bound must exceed its lower bound."** — a row has `upper ≤ lower`; fix
  the bounds (press Enter to commit the cell before Start).

---

## 7. Verification

`test_ml_optimizer.py` exercises the Python optimizer and the file protocol with no
hardware: it emulates the GUI, scoring each suggestion with a known noisy paraboloid and
checking that the best-found point lands near the true optimum. Run:

```
python test_ml_optimizer.py
```

It prints `RESULT: PASS` on convergence — a good smoke test after changing
`ml_optimizer.py` or the file formats.
