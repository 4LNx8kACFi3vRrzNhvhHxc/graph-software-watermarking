#!/bin/sh
# Checks that watermark_encode and watermark_decode_improved coincide with a direct Python
# transcription of Algorithms 1 and 4 (reference.py): same set of graphs for every identifier
# over all random choices, and same decoded sequence after the removal of each encoding edge.
# Usage: sh scripts/verify_codec/run.sh [N]   (from the repository root; default N = 12)
set -e
N=${1:-12}
ROOT=$(pwd); V=$ROOT/scripts/verify_codec; B=$V/build; mkdir -p $B
CF="-std=gnu99 -O2 -fopenmp -D_DEFAULT_SOURCE -I$ROOT/include $EXTRA_CFLAGS"
for f in $(find src -name '*.c'); do
  o=$B/$(echo $f | tr / _).o
  if [ "$f" = "src/utils/utils.c" ]; then
    gcc $CF -Dget_backedge_index=orig_get_backedge_index -Dencoder_choose_forward_at_2=orig_encoder_choose_forward_at_2 -c $f -o $o
  else gcc $CF -c $f -o $o; fi
done
gcc $CF $V/verify.c $B/*.o $EXTRA_LDFLAGS -lfec -lm -o $B/verify
cd $V
$B/verify $N out.txt
python3 compare.py out.txt $N
