# Experimental results

Results reported in Section 6 of the paper (Tables 2–5).

| Table | Experiment | Raw output (`raw/`) | Generated table (`tables/`) |
| --- | --- | --- | --- |
| 2 | Original decoder, removal of one encoding edge, n = 1–24 bits | `table2_original_1edge.txt` | `table2.csv` |
| 3 | Improved decoder, removal of one edge, n = 1–24 bits | `table3_improved_1edge.txt` | `table3.csv` |
| 4 | Improved decoder, removal of two edges, n = 1–24 bits | `table4_improved_2edge.txt` | `table4.csv` |
| 5 | Improved decoder + Reed–Solomon (m = 3, K = 1–3), one or two parity symbols, removal of one or two edges | `table5_rs_p{1,2}_{1,2}edge.txt` | `table5.csv` |

`tables/tables.tex` contains the four tables as LaTeX environments.

## How to read the tables

Each cell counts the identifiers whose **worst-case** attack yields Δ wrong bits. For a given identifier, every combination of one (or two) encoding edges (forward and back edges) is removed, the graph is decoded, and the largest Δ over all combinations is recorded.

- Δ is the number of positions in which the decoded sequence differs from the identifier, aligned at the leading 1. Positions missing from the decoded sequence count as errors.
- Tables 2–4 cover all 2^(n−1) identifiers of each length n (b₁ = 1).
- Table 5 covers all identifiers with exactly 3K bits and b₁ = 1, that is, 4, 32 and 256 identifiers for K = 1, 2 and 3. Δ counts wrong message bits after Reed–Solomon decoding. The row `fail.` counts identifiers for which some attack makes Reed–Solomon report an uncorrectable codeword.
- When a graph has fewer encoding edges than the number of removals, all of them are removed.

## Environment

- Code: encoder aligned with Algorithm 1 of the paper (random choice between [1 ← 2] and [2 → 4] for the second bit; removal of v−1 from the candidate lists when a while block is built) and improved decoder implementing Algorithm 4; the tables were built with `scripts/make_tables.py`.
- Encoder randomness: `WATERMARK_SEED=1` (see the main README); each identifier is encoded by the graph determined by the seed and the identifier, independently of the number of threads.
- Runs: November 5, 2026, Linux x86-64 with 2 cores, gcc 13.3 with OpenMP, libfec from [quiet/libfec](https://github.com/quiet/libfec). Running time: about 12 minutes for each of Tables 2 and 3, 70 minutes for Table 4 and a few seconds for Table 5.
- `scripts/verify_codec/run.sh` checks that the encoder and the improved decoder coincide with a direct transcription of Algorithms 1 and 4 (same graphs for every identifier over all random choices, same decoded sequences after the removal of each encoding edge); it reported no difference for identifiers of up to 14 bits.

## Reproducibility

The results depend only on `WATERMARK_SEED`. With another seed, each identifier is encoded by another of its possible watermark graphs; the bounds of the paper (Δ ≤ 1 under the removal of one encoding edge, Δ = 0 when that edge is a forward edge) hold for every graph, but the counts of each row may change.
