#!/usr/bin/env python3
"""MIP lower bounds and seed partitions for ChipletPart.

Builds a mixed-integer relaxation of the chiplet cost model with HiGHS,
reports a lower bound on the cost of any partition into K chiplets (floorplan
feasibility ignored), and writes the MIP's partition as a seed that can be
handed to chipletPart with --init-partition.

Requirements: python3 with numpy and highspy (pip install numpy highspy), and
the cost_probe binary built next to chipletPart (CMake target cost_probe).

Example:
    tools/mip_seed.py test_data/48_4_14_4_1600_1600 --k 8 --time 900 --out seeds
    ./run_chiplet_test.sh 48_4_14_4_1600_1600 --init-partition seeds/48_4_14_4_1600_1600.K8.part

Cost structure (verified against the cost model with cost_probe):
    cost = [ sum_k f(W_k) + g(U) + asm_K ] / ( Y0_K * exp(-lam * S) )
    W_k  = sum_{v in k} a_v + IO_k         chiplet area incl. IO cells
    U    = sum_k (sqrt(W_k) + s)^2          s = die separation
    S    = 2 * sum_{cut (i->j)} c_ij        root signal count
f (per-chiplet cost incl. yield), g (interposer cost) and lam are sampled from
the cost model. The relaxation replaces f + gamma*(sqrt(W)+s)^2 and g by
supporting lines of their lower convex hulls, and the yield factor by a tangent
of exp(lam*S)/Y0 at --s0 combined with a McCormick bound on N*S, so the MIP
optimum (its dual bound when the time limit hits) is a valid lower bound up to
the sampling grid of f (0.25 mm^2). The MIP's own partition is reported as a
seed; its cost is the exact cost model value for square chiplets.
"""
import argparse
import collections
import math
import os
import re
import subprocess
import sys
import time

import numpy as np

try:
    import highspy
except ImportError:  # pragma: no cover
    sys.exit("mip_seed.py needs the highspy package (pip install highspy)")


# ----------------------------------------------------------------------------
# design data and cost-model samples
# ----------------------------------------------------------------------------
def run_probe(probe, *args):
    out = subprocess.run([probe, *args], check=True, capture_output=True, text=True).stdout
    return [l for l in out.splitlines() if l and not l.startswith("[")]


def load_design(probe, design_dir, tech):
    blocks, ios, adj = [], {}, []
    for l in run_probe(probe, "dump", design_dir, tech):
        p = l.split()
        if p[0] == "block":
            blocks.append((p[2], float(p[3]), float(p[4])))
        elif p[0] == "io":
            kv = dict(x.split("=") for x in p[2:])
            ios[p[1]] = (float(kv["tx"]), float(kv["rx"]))
        elif p[0] == "adj":
            adj.append((int(p[2]), int(p[3]), int(p[4]), p[1]))
    return blocks, ios, adj


def assembly_margins(design_dir):
    """(die_separation, edge_exclusion) of the assembly process the model uses."""
    txt = open(os.path.join(design_dir, "assembly_process_definitions.xml")).read()
    m = re.search(r'<assembly name="silicon_individual_bonding"(.*?)>', txt, re.S)
    body = m.group(1) if m else txt
    sep = re.search(r'die_separation="([0-9.eE+-]+)"', body)
    edge = re.search(r'edge_exclusion="([0-9.eE+-]+)"', body)
    return (float(sep.group(1)) if sep else 0.1, float(edge.group(1)) if edge else 0.1)


class Curves:
    """f(W, ar), g_K(U) and the yield decay, sampled from the cost model."""

    def __init__(self, probe, design_dir, tech, w_max, ks, sep):
        self.sep = sep
        ars = "1,1.25,1.5,2,2.5,3,4,5"
        f = collections.defaultdict(dict)
        for l in run_probe(probe, "fcurve", design_dir, tech, "0.25", f"{w_max:.2f}", "0.25", ars):
            _, W, ar, c = l.split()
            f[float(W)][float(ar)] = float(c)
        self.W = np.array(sorted(f))
        self.f1 = np.array([f[w][1.0] for w in self.W])
        self.fmin = np.array([min(f[w].values()) for w in self.W])
        self.g = {}
        for K in ks:
            rows = []
            a_hi = max(2.0, 1.2 * w_max / K + 2.0)
            for l in run_probe(probe, "gcurve", design_dir, tech, str(K), "0.5", f"{a_hi:.2f}", "0.25"):
                _, k, A, st, rs, asmc, y = l.split()
                rows.append((float(A), float(st), float(rs), float(asmc), float(y)))
            arr = np.array(rows)
            self.g[K] = dict(U=K * (np.sqrt(arr[:, 0]) + sep) ** 2, rootself=arr[:, 2],
                             asm=float(arr[0, 3]), Y0=float(arr[0, 4]))
        lam = []
        for l in run_probe(probe, "ycurve", design_dir, tech, "4", "40", "0", "40000", "10000"):
            _, k, S, y = l.split()
            if int(S) == 0:
                y0 = float(y)
            else:
                lam.append(-math.log(float(y) / y0) / int(S))
        self.lam = float(np.mean(lam))

    def f_at(self, W, use_min=False):
        return np.interp(W, self.W, self.fmin if use_min else self.f1)

    def g_at(self, K, U):
        return np.interp(U, self.g[K]["U"], self.g[K]["rootself"])

    def Y(self, K, S):
        return self.g[K]["Y0"] * math.exp(-self.lam * S)


def evaluate(blocks, ios, adj, part, cv):
    """Surrogate cost of a partition (square chiplets)."""
    K = max(part) + 1
    A = np.zeros(K); IO = np.zeros(K); S = 0
    for v, (_, a, _) in enumerate(blocks):
        A[part[v]] += a
    for i, j, c, t in adj:
        if part[i] != part[j]:
            tx, rx = ios[t]
            IO[part[i]] += c * tx
            IO[part[j]] += c * rx
            S += 2 * c
    W = A + IO
    U = float(np.sum((np.sqrt(W) + cv.sep) ** 2))
    N = float(np.sum(cv.f_at(W))) + float(cv.g_at(K, U)) + cv.g[K]["asm"]
    return dict(cost=N / cv.Y(K, S), S=S, W=W, U=U)


# ----------------------------------------------------------------------------
# convex hulls
# ----------------------------------------------------------------------------
def lower_hull(xs, ys):
    pts = sorted(zip(xs, ys))
    hull = []
    for p in pts:
        while len(hull) >= 2:
            (x1, y1), (x2, y2) = hull[-2], hull[-1]
            if (y2 - y1) * (p[0] - x1) >= (p[1] - y1) * (x2 - x1):
                hull.pop()
            else:
                break
        hull.append(p)
    return hull


def hull_support(hull, x0):
    for (x1, y1), (x2, y2) in zip(hull, hull[1:]):
        if x1 <= x0 <= x2:
            a = (y2 - y1) / (x2 - x1)
            return a, y1 - a * x1
    (x1, y1), (x2, y2) = hull[-2], hull[-1]
    a = (y2 - y1) / (x2 - x1)
    return a, y1 - a * x1


def phi_lines(cv, gamma, W_lo, W_hi, nlines=150):
    """Supporting lines of the lower hull of f_min(W) + gamma*(sqrt(W)+s)^2."""
    phi = cv.fmin + gamma * (np.sqrt(cv.W) + cv.sep) ** 2
    m = (cv.W >= W_lo - 1e-9) & (cv.W <= W_hi + 1e-9)
    hull = lower_hull(cv.W[m], phi[m])
    lines = []
    for (x1, y1), (x2, y2) in zip(hull, hull[1:]):
        a = (y2 - y1) / (x2 - x1)
        lines.append((x1, a, y1 - a * x1))
    if len(lines) <= nlines:
        return [(a, b) for _, a, b in lines]
    xs = np.array([x for x, _, _ in lines])
    pick = sorted(set(int(np.searchsorted(xs, t)) for t in np.linspace(W_lo, W_hi, nlines)))
    return [lines[min(p, len(lines) - 1)][1:] for p in pick]


def interchangeable_groups(blocks, adj):
    """Vertices with equal area/power whose swap leaves the netlist unchanged."""
    n = len(blocks)
    C = collections.Counter()
    for i, j, c, _ in adj:
        C[(i, j)] += c

    def swap_ok(u, v):
        if abs(blocks[u][1] - blocks[v][1]) > 1e-9 or abs(blocks[u][2] - blocks[v][2]) > 1e-9:
            return False
        if C[(u, v)] != C[(v, u)]:
            return False
        return all(C[(u, w)] == C[(v, w)] and C[(w, u)] == C[(w, v)]
                   for w in range(n) if w not in (u, v))

    used, groups = set(), []
    for u in range(n):
        if u in used:
            continue
        g = [u] + [v for v in range(u + 1, n) if v not in used and swap_ok(u, v)]
        used.update(g)
        if len(g) > 1:
            groups.append(g)
    return groups


# ----------------------------------------------------------------------------
# MIP
# ----------------------------------------------------------------------------
def solve(blocks, ios, adj, K, lines, const_N, time_limit, topology=None, wN=1.0, wS=0.0,
          verbose=False):
    n = len(blocks)
    areas = np.array([b[1] for b in blocks])
    pairs = {}
    for i, j, c, t in adj:
        tx, rx = ios[t]
        key = (min(i, j), max(i, j))
        d = pairs.setdefault(key, [0.0, 0.0, 0])  # IO area on i side, on j side, signals
        if i < j:
            d[0] += c * tx; d[1] += c * rx
        else:
            d[1] += c * tx; d[0] += c * rx
        d[2] += 2 * c
    P = list(pairs.items())

    h = highspy.Highs()
    h.setOptionValue("output_flag", verbose)
    h.setOptionValue("time_limit", float(time_limit))
    h.setOptionValue("mip_rel_gap", 1e-4)
    inf = highspy.kHighsInf

    def X(v, k): return v * K + k
    nx = n * K
    def E(p, s, k): return nx + (p * 2 + s) * K + k
    ne = len(P) * 2 * K
    def Wv(k): return nx + ne + k
    def Cv(k): return nx + ne + K + k
    zpairs = [(k, l) for k in range(K) for l in range(k + 1, K)] if topology is not None else []
    zidx = {kl: nx + ne + 2 * K + t for t, kl in enumerate(zpairs)}
    Sv = nx + ne + 2 * K + len(zpairs)
    nvars = Sv + 1

    lb = np.zeros(nvars); ub = np.ones(nvars); cost = np.zeros(nvars)
    for k in range(K):
        lb[Wv(k)] = areas.min(); ub[Wv(k)] = inf
        lb[Cv(k)] = -inf; ub[Cv(k)] = inf
        cost[Cv(k)] = wN
    lb[Sv] = 0.0; ub[Sv] = inf; cost[Sv] = wS
    if topology is None:
        for v in range(n):          # chiplets numbered by first vertex
            for k in range(v + 1, K):
                ub[X(v, k)] = 0.0
    else:
        for kl in zpairs:
            if kl not in topology:
                ub[zidx[kl]] = 0.0
    h.addVars(nvars, lb, ub)
    h.changeColsCost(nvars, np.arange(nvars, dtype=np.int32), cost)
    integ = ([highspy.HighsVarType.kInteger] * nx +
             [highspy.HighsVarType.kContinuous] * (ne + 2 * K) +
             [highspy.HighsVarType.kInteger] * len(zpairs) +
             [highspy.HighsVarType.kContinuous])
    h.changeColsIntegrality(nvars, np.arange(nvars, dtype=np.int32), np.array(integ))

    lo, hi, starts, idx, val = [], [], [], [], []
    def row(l, u, cols, vals):
        starts.append(len(idx)); idx.extend(cols); val.extend(vals); lo.append(l); hi.append(u)

    for grp in interchangeable_groups(blocks, adj):
        for u, w in zip(grp, grp[1:]):   # chiplet index of u <= chiplet index of w
            row(0.0, inf, [X(u, k) for k in range(K)] + [X(w, k) for k in range(K)],
                [-float(k) for k in range(K)] + [float(k) for k in range(K)])
    for v in range(n):
        row(1.0, 1.0, [X(v, k) for k in range(K)], [1.0] * K)
    for k in range(K):
        row(1.0, inf, [X(v, k) for v in range(n)], [1.0] * n)
    for p, ((i, j), _) in enumerate(P):
        for k in range(K):
            for s, (u, w) in enumerate(((i, j), (j, i))):
                row(0.0, inf, [E(p, s, k), X(u, k), X(w, k)], [1.0, -1.0, 1.0])
                row(-inf, 0.0, [E(p, s, k), X(u, k)], [1.0, -1.0])
                row(-inf, 1.0, [E(p, s, k), X(w, k)], [1.0, 1.0])
    for k in range(K):
        cols = [X(v, k) for v in range(n)]; vals = list(areas)
        for p, (_, (ai, aj, _)) in enumerate(P):
            cols += [E(p, 0, k), E(p, 1, k)]; vals += [ai, aj]
        cols.append(Wv(k)); vals.append(-1.0)
        row(0.0, 0.0, cols, vals)
    for k in range(K):
        for a, b in lines:
            row(b, inf, [Cv(k), Wv(k)], [1.0, -a])
    if zpairs:
        for (i, j), _ in P:
            for k in range(K):
                for l in range(K):
                    if k != l:
                        row(-inf, 1.0, [X(i, k), X(j, l), zidx[(min(k, l), max(k, l))]], [1.0, 1.0, -1.0])
    cols, vals = [Sv], [-1.0]
    for p, (_, (_, _, sc)) in enumerate(P):
        for k in range(K):
            cols.append(E(p, 0, k)); vals.append(float(sc))
    row(0.0, 0.0, cols, vals)
    starts.append(len(idx))
    h.addRows(len(lo), np.array(lo), np.array(hi), len(idx), np.array(starts[:-1], dtype=np.int32),
              np.array(idx, dtype=np.int32), np.array(val))
    h.run()
    info = h.getInfo()
    status = h.modelStatusToString(h.getModelStatus())
    part = None
    if info.primal_solution_status == 2:
        xs = np.array(h.getSolution().col_value[:nx]).reshape(n, K)
        part = [int(np.argmax(xs[v])) for v in range(n)]
    dual = inf if status == "Infeasible" else info.mip_dual_bound + wN * const_N
    return dict(status=status, dual=dual, primal=info.objective_function_value + wN * const_N, part=part)


def bound_and_seed(blocks, ios, adj, K, cv, args, topology):
    areas = np.array([b[1] for b in blocks]); Atot = areas.sum()
    S_all = 2 * sum(c for _, _, c, _ in adj)
    IO_all = sum(c * (ios[t][0] + ios[t][1]) for _, _, c, t in adj)
    W_lo, W_hi = areas.min(), Atot + IO_all
    gd = cv.g[K]
    s = cv.sep
    U_min = Atot + K * s ** 2 + 2 * s * math.sqrt(Atot)
    U_max = Atot + IO_all + K * s ** 2 + 2 * s * math.sqrt(K * (Atot + IO_all))
    um = (gd["U"] >= U_min - 2.0) & (gd["U"] <= U_max + 2.0)
    ghull = lower_hull(gd["U"][um], gd["rootself"][um])
    U0 = args.u0 if args.u0 else K * (math.sqrt(Atot / K) + s) ** 2
    gamma, delta = hull_support(ghull, U0)
    const_N = delta + gd["asm"]
    lines = phi_lines(cv, gamma, W_lo, W_hi)

    S0 = args.s0 if args.s0 is not None else 0.3 * S_all
    hS0 = math.exp(cv.lam * S0) / gd["Y0"]
    acoef, bcoef = hS0 - cv.lam * hS0 * S0, cv.lam * hS0
    t0 = time.time()
    r1 = solve(blocks, ios, adj, K, lines, const_N, args.time, topology, verbose=args.verbose)
    N_lo = r1["dual"]
    if not math.isfinite(N_lo):
        return dict(K=K, lb=math.inf, part=None, status=r1["status"], time=time.time() - t0)
    r2 = solve(blocks, ios, adj, K, lines, const_N, args.time, topology, wN=acoef,
               wS=bcoef * N_lo, verbose=args.verbose)
    return dict(K=K, lb=r2["dual"], part=r2["part"], status=r2["status"],
                nlo_status=r1["status"], time=time.time() - t0)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("design_dir")
    ap.add_argument("--tech", default="7nm")
    ap.add_argument("--k", default="8", help="chiplet count, or a range like 4-8")
    ap.add_argument("--time", type=float, default=120, help="HiGHS time limit per MIP (s)")
    ap.add_argument("--probe", default=None, help="cost_probe binary (default: build/bin/cost_probe)")
    ap.add_argument("--out", default=None, help="directory for seed partitions")
    ap.add_argument("--topology-from", default=None,
                    help="partition file; only its chiplet adjacencies may carry cut nets")
    ap.add_argument("--s0", type=float, default=None, help="yield linearization point (signals)")
    ap.add_argument("--u0", type=float, default=None, help="interposer linearization point (mm^2)")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    probe = args.probe or os.path.join(root, "build", "bin", "cost_probe")
    if not os.path.exists(probe):
        sys.exit(f"cost_probe not found at {probe}; build it with `make cost_probe`")
    design_dir = args.design_dir.rstrip("/")
    name = os.path.basename(design_dir)
    ks = list(range(int(args.k.split("-")[0]), int(args.k.split("-")[-1]) + 1))

    blocks, ios, adj = load_design(probe, design_dir, args.tech)
    areas = np.array([b[1] for b in blocks])
    IO_all = sum(c * (ios[t][0] + ios[t][1]) for _, _, c, t in adj)
    sep, _ = assembly_margins(design_dir)
    print(f"[{name}] {len(blocks)} blocks, {len(adj)} adjacency entries, total area {areas.sum():.2f} mm^2, "
          f"sampling cost model...", flush=True)
    cv = Curves(probe, design_dir, args.tech, areas.sum() + IO_all + 1.0, ks, sep)
    print(f"[{name}] yield decay {cv.lam:.3e}/signal", flush=True)

    topology = None
    if args.topology_from:
        tp = [int(x) for x in open(args.topology_from).read().split()]
        topology = {(min(tp[i], tp[j]), max(tp[i], tp[j])) for i, j, _, _ in adj if tp[i] != tp[j]}
        ks = [max(tp) + 1]
        print(f"[{name}] topology fixed to {len(topology)} chiplet adjacencies of {args.topology_from}")

    results = []
    for K in ks:
        r = bound_and_seed(blocks, ios, adj, K, cv, args, topology)
        line = f"[{name}] K={K}: lower bound {r['lb']:.4f} ({r['status']}, {r['time']:.0f}s)"
        if r["part"] is not None:
            ev = evaluate(blocks, ios, adj, r["part"], cv)
            line += f"; seed surrogate cost {ev['cost']:.4f}, {ev['S']} signals"
            if args.out:
                os.makedirs(args.out, exist_ok=True)
                path = os.path.join(args.out, f"{name}.K{K}.part")
                with open(path, "w") as f:
                    f.write("\n".join(map(str, r["part"])) + "\n")
                exact = [l for l in run_probe(probe, "eval", design_dir, args.tech, path) if l.startswith("total")]
                line += f", exact {exact[0].split()[1] if exact else '?'} -> {path}"
        print(line, flush=True)
        results.append(r)
    finite = [r["lb"] for r in results if math.isfinite(r["lb"])]
    if finite:
        print(f"[{name}] lower bound over K in {ks[0]}..{ks[-1]}: {min(finite):.4f} "
              f"(floorplan feasibility not included)")


if __name__ == "__main__":
    main()
