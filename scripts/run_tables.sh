#!/bin/sh
# Runs every experiment reported in Section 6 of the paper (Tables 2-5) and
# stores the harness output of each one in results/raw/, overwriting the
# outputs committed in the repository (use `git diff` to compare).
#
# Usage, from the repository root, after `make build`:
#   sh scripts/run_tables.sh [path/to/watermark/binary]
#
# Then build the LaTeX tables and CSV files with:
#   python3 scripts/make_tables.py results/raw results/tables
#
# All experiments take about 1 h 40 min on 2 cores; Table 4 (two removals,
# up to 24 bits) alone takes about 75 min. Set START_BITS to run a range of
# lengths separately (e.g. START_BITS=24 for the last column only).
set -e

BIN=${1:-build/main/watermark}
OUT=results/raw
mkdir -p "$OUT" tests

run() {
  name=$1
  input=$2
  echo "[$(date +%T)] $name"
  printf "$input" | "$BIN" > "$OUT/$name.txt" 2> "$OUT/$name.log"
}

# menu option, number of removals, maximum length (or symbols, parity, symbol size)
run table2_original_1edge  '5\n1\n24\n'
run table3_improved_1edge  '6\n1\n24\n'
run table4_improved_2edge  '6\n2\n24\n'
run table5_rs_p1_1edge     '7\n1\n3\n1\n3\n'
run table5_rs_p2_1edge     '7\n1\n3\n2\n3\n'
run table5_rs_p1_2edge     '7\n2\n3\n1\n3\n'
run table5_rs_p2_2edge     '7\n2\n3\n2\n3\n'

echo "[$(date +%T)] done"
