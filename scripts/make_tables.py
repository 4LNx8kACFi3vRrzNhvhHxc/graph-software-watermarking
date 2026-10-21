#!/usr/bin/env python3
"""Build the LaTeX tables (Tables 2-5) and CSV files from the harness outputs.

Usage:
    python3 scripts/make_tables.py RESULTS_DIR OUT_DIR

RESULTS_DIR holds the files written by scripts/run_tables.sh. Table 4 may be
given as one file (table4_improved_2edge.txt) or split into several runs made
with START_BITS (any file named table4_improved_2edge*.txt); each column is
taken from the file that contains it.

Outputs, in OUT_DIR: table2..5.csv and tables.tex (complete table environments,
requiring the booktabs, multirow and graphicx packages).
"""
import csv
import glob
import os
import re
import sys

NMAX = 24          # identifier lengths of Tables 2-4
SYMSIZE = 3        # Reed-Solomon symbol size of Table 5
KMAX = 3           # message symbols of Table 5


def parse(path):
    """Read the report matrix printed by the harness.

    Returns (rows, total): rows[delta] = list of counts per column (one column
    per identifier length, or per number of message symbols), total = column sums.
    Columns that were not run (START_BITS) have total 0.
    """
    lines = open(path).read().splitlines()
    start = max(i for i, l in enumerate(lines) if l.startswith("lost bits"))
    rows, total = {}, None
    for line in lines[start + 2:]:
        cells = [c.replace("\\\\", "").replace("\\hline", "").strip() for c in line.split("&")]
        if cells[0] == "Total":
            total = [int(c) for c in cells[1:]]
            break
        if re.fullmatch(r"\d+", cells[0]):
            rows[int(cells[0])] = [int(c) for c in cells[1:]]
    if total is None:
        sys.exit(f"{path}: incomplete run (no Total row)")
    return rows, total


def column(rows, c):
    return {d: v[c] for d, v in rows.items()}


def check(col, expected, what):
    got = sum(col.values())
    if got != expected:
        sys.exit(f"{what}: {got} identifiers counted, {expected} expected")


def fmt(x):
    return f"{x:,}".replace(",", "{,}")


def wide_table(columns, n_cols):
    """columns[c] = {delta: count}; returns the tabular body and CSV rows."""
    deltas = sorted(d for d in set().union(*columns) if any(col.get(d, 0) for col in columns))
    body = [r"$\Delta$ & " + " & ".join(str(n) for n in range(1, n_cols + 1)) + r" \\", r"\midrule"]
    rows = [["delta"] + [f"n={n}" for n in range(1, n_cols + 1)]]
    for d in deltas:
        vals = [col.get(d, 0) for col in columns]
        body.append(f"{d} & " + " & ".join(fmt(v) for v in vals) + r" \\")
        rows.append([d] + vals)
    totals = [sum(col.values()) for col in columns]
    body += [r"\midrule", "Total & " + " & ".join(fmt(v) for v in totals) + r" \\"]
    rows.append(["total"] + totals)
    return "\n".join(body), rows


def wide_env(label, caption, body):
    return (
        "\\begin{table}[htbp]\n\\centering\n"
        f"\\caption{{{caption}}}\\label{{{label}}}\n"
        "\\resizebox{\\textwidth}{!}{%\n"
        f"\\begin{{tabular}}{{r*{{{NMAX}}}{{r}}}}\n\\toprule\n"
        f" & \\multicolumn{{{NMAX}}}{{c}}{{Identifier length $n$ (bits)}} \\\\\n"
        f"\\cmidrule(lr){{2-{NMAX + 1}}}\n{body}\n\\bottomrule\n\\end{{tabular}}}}\n\\end{{table}}\n"
    )


def write_csv(path, rows):
    with open(path, "w", newline="") as f:
        csv.writer(f).writerows(rows)


def single_run_columns(path):
    rows, _ = parse(path)
    cols = [column(rows, c) for c in range(NMAX)]
    for n, col in enumerate(cols, 1):
        check(col, 2 ** (n - 1), f"{os.path.basename(path)}, n={n}")
    return cols


def table4_columns(results):
    """Merge Table 4 columns from one or more runs."""
    cols = [None] * NMAX
    for path in sorted(glob.glob(os.path.join(results, "table4_improved_2edge*.txt"))):
        rows, total = parse(path)
        for c in range(min(NMAX, len(total))):
            if total[c] and cols[c] is None:
                cols[c] = column(rows, c)
    missing = [c + 1 for c, col in enumerate(cols) if col is None]
    if missing:
        sys.exit(f"Table 4: no run covers n = {missing}")
    for n, col in enumerate(cols, 1):
        check(col, 2 ** (n - 1), f"Table 4, n={n}")
    return cols


# Captions: adjust \cite and \ref keys to those of the manuscript.
CAPTIONS = {
    "tab:original": (
        r"Single-edge removal (one non-Hamiltonian edge): original decoding~\cite{bento2024structured}. "
        r"Each cell reports the number of identifiers whose worst-case attack yields $\Delta$ unrecovered "
        r"bits, for a given identifier length $n$ (in bits). Since the codec fixes $b_1 = 1$, there are "
        r"exactly $2^{n-1}$ distinct identifiers of length $n$. $\Delta$ is the number of positions that "
        r"differ from the original sequence (Section~\ref{sec:setup}). Rows with all-zero entries are omitted."
    ),
    "tab:improved": (
        r"Single-edge removal (one non-Hamiltonian edge): improved decoding given $n$ (no ECC). Each cell "
        r"reports the number of identifiers whose worst-case attack yields $\Delta$ unrecovered bits, for a "
        r"given identifier length $n$ (in bits). Under single-edge removal, the improved decoder guarantees "
        r"$\Delta \le 1$ for all identifiers, consistent with Theorem~\ref{thm:singleedge}."
    ),
    "tab:improved2": (
        r"Double-edge removal (two non-Hamiltonian edges): improved decoding given $n$ (no ECC). Each cell "
        r"reports the number of identifiers whose worst-case attack yields $\Delta$ unrecovered bits, for a "
        r"given identifier length $n$ (in bits). For identifiers whose watermark graphs contain fewer than "
        r"two non-Hamiltonian edges, all of them are removed. Rows with all-zero entries are omitted."
    ),
    "tab:rs": (
        r"Reed--Solomon error correction under single- and double-edge removal (symbol size $m = 3$ bits). "
        r"Identifiers have exactly $3K$ bits with $b_1 = 1$, that is, $2^{3K-1}$ identifiers for each "
        r"number $K$ of message symbols. Each cell reports the number of identifiers whose worst-case attack "
        r"yields $\Delta$ unrecovered message bits; the row \emph{fail.} counts identifiers for which some "
        r"attack leads Reed--Solomon decoding to report an uncorrectable codeword."
    ),
}


def table5(results):
    """Body and CSV rows of Table 5. The last row of the RS report matrix counts decoding failures."""
    fail_row = KMAX * SYMSIZE + 1
    body = [
        r" & & \multicolumn{3}{c}{1-edge removal} & \multicolumn{3}{c}{2-edge removal} \\",
        r"\cmidrule(lr){3-5}\cmidrule(lr){6-8}",
        r"ECC & $\Delta$ & $K=1$ & $K=2$ & $K=3$ & $K=1$ & $K=2$ & $K=3$ \\",
        r"\midrule",
    ]
    rows = [["ecc", "delta"] + [f"{e}edge_K{k}" for e in (1, 2) for k in range(1, KMAX + 1)]]
    totals = None
    for label, key in (("RS + 1 parity", "p1"), ("RS + 2 parities", "p2")):
        runs = []
        for edges in (1, 2):
            r, _ = parse(os.path.join(results, f"table5_rs_{key}_{edges}edge.txt"))
            for k in range(1, KMAX + 1):
                check(column(r, k - 1), 2 ** (SYMSIZE * k - 1), f"Table 5, {key}, {edges} edge(s), K={k}")
            runs.append(r)
        deltas = sorted(d for d in set(runs[0]) | set(runs[1])
                        if any(any(r.get(d, [0] * KMAX)[:KMAX]) for r in runs))
        for j, d in enumerate(deltas):
            vals = runs[0].get(d, [0] * KMAX)[:KMAX] + runs[1].get(d, [0] * KMAX)[:KMAX]
            name = "fail." if d == fail_row else str(d)
            first = rf"\multirow{{{len(deltas)}}}{{*}}{{{label}}}" if j == 0 else ""
            body.append(f"{first} & {name} & " + " & ".join(map(str, vals)) + r" \\")
            rows.append([label, "failure" if name == "fail." else d] + vals)
        body.append(r"\midrule")
        totals = [2 ** (SYMSIZE * k - 1) for k in range(1, KMAX + 1)] * 2
    body.append("Total & & " + " & ".join(map(str, totals)) + r" \\")
    rows.append(["total", ""] + totals)
    env = (
        "\\begin{table}[htbp]\n\\centering\n"
        f"\\caption{{{CAPTIONS['tab:rs']}}}\\label{{tab:rs}}\n"
        "\\begin{tabular}{llrrrrrr}\n\\toprule\n" + "\n".join(body) + "\n\\bottomrule\n\\end{tabular}\n\\end{table}\n"
    )
    return env, rows


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    results, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    tex = ["% Generated by scripts/make_tables.py. Requires booktabs, multirow and graphicx.\n"]
    sources = [
        ("table2", "tab:original", single_run_columns(os.path.join(results, "table2_original_1edge.txt"))),
        ("table3", "tab:improved", single_run_columns(os.path.join(results, "table3_improved_1edge.txt"))),
        ("table4", "tab:improved2", table4_columns(results)),
    ]
    for name, label, cols in sources:
        body, rows = wide_table(cols, NMAX)
        write_csv(os.path.join(out, f"{name}.csv"), rows)
        tex.append(wide_env(label, CAPTIONS[label], body))
    env, rows = table5(results)
    write_csv(os.path.join(out, "table5.csv"), rows)
    tex.append(env)
    open(os.path.join(out, "tables.tex"), "w").write("\n".join(tex))
    print(f"tables written to {out}/")


if __name__ == "__main__":
    main()
