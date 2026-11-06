#!/bin/sh
# Reproduces the figures of Section 4.4 (recognition of Dijkstra graphs).
# Usage: sh scripts/section44/run.sh   (from the repository root; needs gcc, libfec, python3 + networkx)
set -e
ROOT=$(pwd); S=$ROOT/scripts/section44; B=$S/build; mkdir -p $B
CF="-std=gnu99 -O2 -fopenmp -D_DEFAULT_SOURCE -I$ROOT/include $EXTRA_CFLAGS"
for f in $(find src -name '*.c'); do
  o=$B/$(echo $f | tr / _).o
  # the two random choices of the encoder are replaced by the harness, which enumerates all of them
  if [ "$f" = "src/utils/utils.c" ]; then gcc $CF -Dget_backedge_index=orig_get_backedge_index -Dencoder_choose_forward_at_2=orig_encoder_choose_forward_at_2 -c $f -o $o
  else gcc $CF -c $f -o $o; fi
done
gcc $CF $S/section44.c $B/*.o $EXTRA_LDFLAGS -lfec -lm -o $B/section44
cd $S
python3 make_inputs.py
$B/section44 file dgs8_rooted.txt > dgs8_rooted.out
$B/section44 file rand60k.txt > rand60k.out
python3 summarize.py
$B/section44 wm 14
