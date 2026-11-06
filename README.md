# Graph-Based Watermarking for Structured Programs

Code and experimental results of the paper **"A Complete Graph-Based Watermarking Scheme for Structured Programs"**

The repository implements the structured watermark codec, which encodes an identifier as a Dijkstra graph and decodes it back, together with:

- the **improved decoder** of the paper (Algorithm 4), which receives only the watermark graph and the public length n of the encoded sequence and guarantees at most one wrong bit when a single encoding edge is removed, and none when that edge is a forward edge;
- the **corrected recognition algorithm for Dijkstra graphs** (Algorithm 3), `dijkstra_check_labeled`;
- the **Reed–Solomon** layer used for exact recovery under single-edge removal;
- the **removal-test harness** and scripts that reproduce Tables 2–5 of the paper, and the results themselves in [`results/`](results/).

## Where things are

| Paper | Code |
| --- | --- |
| Encoding, Algorithm 1 (Section 4.1) | `watermark_encode` in `src/encoder/encoder.c` |
| Original decoding [Bento et al.] | `watermark_decode` in `src/decoder/decoder.c` |
| Improved decoding, Algorithm 4 (Section 5.1) | `watermark_decode_improved` in `src/decoder/decoder.c` |
| Reed–Solomon integration (Section 5.2) | `watermark_rs_encode` in `src/encoder/encoder.c`, `watermark_rs_decode_improved` in `src/decoder/decoder.c`, `src/rs_api/` |
| Admissibility of back edges | `has_possible_backedge` in `src/utils/utils.c` |
| Corrected recognition of Dijkstra graphs, Algorithm 3 (Section 4.4) | `dijkstra_check_labeled` in `src/dijkstra/dijkstra_labeled.c` |
| Recognition procedure of [Bento et al. 2019] and Dijkstra codes | `dijkstra_check`, `dijkstra_get_code` in `src/dijkstra/dijkstra.c`, `src/checker/` |
| Tests of Section 4.4 | `scripts/section44/` |
| Removal experiments (Section 6) | `attack` and `_test_with_removed_connections` in `watermark.c` |

### Improved decoder

`watermark_decode_improved(graph, data, num_bits)` implements Algorithm 4. It uses the buffer `data` only to obtain the length n of the encoded sequence (the position of its leading 1 and its size); the values of its bits are never read. It counts the forward edges expected from |V(G)| and n, compares them with the forward edges present in the graph, and uses the difference to detect removed forward edges (rules R4 and R5). The admissibility test of rule R5 uses candidate lists maintained on the distorted graph, so the decoder runs in time linear in n.

## Requirements

- A C compiler with OpenMP: clang, as configured in the `Makefile`, or gcc (see below).
- [libfec](https://www.ka9q.net/code/fec/) (`libfec-dev`); the runs in `results/` used [quiet/libfec](https://github.com/quiet/libfec).
- Python 3, only to generate the tables.
- `graphviz` (optional): the program writes `.dot` files that it can render.

`flake.nix` lists the dependencies for Nix users.

## Build

```sh
make build            # clang, as configured in the Makefile
```

With gcc, which does not accept some of the clang-specific flags of the `Makefile`:

```sh
make build COMPILER="gcc -fopenmp" FLAGS="-std=gnu99 -fPIC -Wall -Wextra -O3 -DNDEBUG" MAIN_LIBS="-lfec -lm"
```

The executable is `build/main/watermark`. `make lib` builds the shared library `build/lib/libwatermark.so`.

## Tests

```sh
make test
# with gcc:
make test COMPILER="gcc -fopenmp -Wl,--no-as-needed -lfec -lm" FLAGS="-std=gnu99 -fPIC -Wall -Wextra" DEBUGGER_COMMAND=
```

All 23 tests pass (checked with gcc 13.3).

## Reproducing Tables 2–5

```sh
make build
WATERMARK_SEED=1 sh scripts/run_tables.sh                                # writes results/raw/
python3 scripts/make_tables.py results/raw results/tables
```

## Reproducing the tests of Section 4.4

```sh
sh scripts/section44/run.sh       # see scripts/section44/README.md
```

## Interactive use

`build/main/watermark` (or `make run`) opens a menu:

```
1) encode string
2) encode number
3) encode number with reed solomon
4) generate graph from dijkstra code
5) run removal test
6) run removal test with improved decoding
7) run removal test with improved decoding and reed solomon
8) reed solomon encode
9) reed solomon decode
10) show report matrix
11) get .dot file dijkstra code
```

Options 5–7 are the removal experiments: they ask for the number of removed edges and the maximum identifier length (or, for option 7, the number of message symbols, of parity symbols and the symbol size) and print the report matrix used in the tables. The environment variable `START_BITS` makes them start at a given length.
