# Graph-Based Watermarking for Structured Programs

Code and experimental results of the paper **"A Complete Graph-Based Watermarking Scheme for Structured Programs"**

The repository implements the structured watermark codec, which encodes an identifier as a Dijkstra graph and decodes it back, together with:

- the **improved decoder** of the paper, which receives only the watermark graph and the public length n of the encoded sequence and guarantees at most one wrong bit when a single non-Hamiltonian edge is removed;
- the **Reed–Solomon** layer used for exact recovery under single-edge removal;
- the **removal-test harness** and scripts that reproduce Tables 2–5 of the paper, and the results themselves in [`results/`](results/).

## Where things are

| Paper | Code |
| --- | --- |
| Encoding (Section 4.1) | `watermark_encode` in `src/encoder/encoder.c` |
| Original decoding [Bento et al.] | `watermark_decode` in `src/decoder/decoder.c` |
| Improved decoding, Algorithm 3 (Section 5.1) | `watermark_decode_improved` in `src/decoder/decoder.c` |
| Reed–Solomon integration (Section 5.2) | `watermark_rs_encode` in `src/encoder/encoder.c`, `watermark_rs_decode_improved` in `src/decoder/decoder.c`, `src/rs_api/` |
| Admissibility of back edges | `has_possible_backedge` in `src/utils/utils.c` |
| Recognition of Dijkstra graphs and their codes | `src/dijkstra/`, `src/checker/` |
| Removal experiments (Section 6) | `attack` and `_test_with_removed_connections` in `watermark.c` |

### Improved decoder

`watermark_decode_improved(graph, data, num_bits)` uses the buffer `data` only to obtain the length n of the encoded sequence (the position of its leading 1 and its size); the values of its bits are never read. It counts the forward edges expected from |V(G)| and n, compares them with the forward edges present in the graph, and uses the difference to detect a removed forward edge. The position of each vertex in its candidate list is recorded when the vertex is inserted, so the decoder runs in time linear in n.

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
sh scripts/run_tables.sh                                # writes results/raw/
python3 scripts/make_tables.py results/raw results/tables
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

## Other files

`test_improved.txt`, `test_rs.txt` and `tests/5_removal.txt` are outputs of earlier versions of the code and are not used in the paper. `cfg.sh` and `get_watermark_candidates.sh` extract the control-flow graphs of the functions in a C file (with clang and LLVM `opt`) to search for functions whose control-flow graphs are Dijkstra graphs; `examples/` contains sample programs for this search.
