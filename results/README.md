# Experimental results

Results reported in Section 6 of the paper (Tables 2–5).

| Table | Experiment | Raw output (`raw/`) | Generated table (`tables/`) |
| --- | --- | --- | --- |
| 2 | Original decoder, removal of one non-Hamiltonian edge, n = 1–24 bits | `table2_original_1edge.txt` | `table2.csv` |
| 3 | Improved decoder, removal of one edge, n = 1–24 bits | `table3_improved_1edge.txt` | `table3.csv` |
| 4 | Improved decoder, removal of two edges, n = 1–24 bits | `table4_improved_2edge_n1-22.txt`, `_n23.txt`, `_n24.txt` | `table4.csv` |
| 5 | Improved decoder + Reed–Solomon (m = 3, K = 1–3), one or two parity symbols, removal of one or two edges | `table5_rs_p{1,2}_{1,2}edge.txt` | `table5.csv` |

`tables/tables.tex` contains the four tables as LaTeX environments.

## How to read the tables

Each cell counts the identifiers whose **worst-case** attack yields Δ wrong bits. For a given identifier, every combination of one (or two) non-Hamiltonian edges is removed, the graph is decoded, and the largest Δ over all combinations is recorded.

- Δ is the number of positions in which the decoded sequence differs from the identifier, aligned at the leading 1. Positions missing from the decoded sequence count as errors.
- Tables 2–4 cover all 2^(n−1) identifiers of each length n (b₁ = 1).
- Table 5 covers all identifiers with exactly 3K bits and b₁ = 1, that is, 4, 32 and 256 identifiers for K = 1, 2 and 3. Δ counts wrong message bits after Reed–Solomon decoding. The row `fail.` counts identifiers for which some attack makes Reed–Solomon report an uncorrectable codeword.
- When a graph has fewer non-Hamiltonian edges than the number of removals, all of them are removed.

## Environment

- Code: commit `c925846` of this repository (decoder and harness); the tables were built with `scripts/make_tables.py`.
- Runs: October 20–21, 2026, Linux x86-64 with 2 cores, gcc 13.3 with OpenMP, libfec from [quiet/libfec](https://github.com/quiet/libfec).
- Table 4 was run in three parts with `START_BITS` (lengths 1–22, 23 and 24). Running time: about 13 minutes for n ≤ 22, 15 minutes for n = 23 and 46 minutes for n = 24; Tables 2 and 3 took about 10 minutes each, and Table 5 a few seconds.

## Variation between runs

The encoder chooses back edges at random, with a `rand()` generator shared by the OpenMP threads, so the graphs, and therefore some counts, change slightly from one run to another. In repeated runs, Table 5 and the columns of Table 2 that were compared (n ≤ 16) were identical; Table 3 kept Δ ≤ 1 for every identifier, with small changes in the row Δ = 0 (e.g. 83 or 86 identifiers for n = 16); and the share of identifiers with Δ ≤ 2 in Table 4 changed by at most 0.2 percentage points (compared for n ≤ 12).
