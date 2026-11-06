"""Faithful re-implementation of Algorithm 1 (encoding) and Algorithm 3 (improved decoding)
as written in the manuscript, used to check Theorem 1 exhaustively over all random choices."""
import sys
from itertools import product


class G:
    def __init__(self, nV, path, fwd, back):
        self.nV = nV
        self.path = set(path)      # (u, u+1)
        self.fwd = set(fwd)        # (u, u+2)
        self.back = dict(back)     # origin -> dest

    def copy(self):
        return G(self.nV, self.path, self.fwd, self.back)


def admissible(g, w, v):
    """Back edge [w <- v] admissible: conditions (i)-(v) of Section 4.1 (strengthened (ii))."""
    if (v - 1, v + 1) in g.fwd and w != v - 1:          # (i)
        return False
    if (v - 1) in g.back:                                # (ii) strengthened
        return False
    if w in g.back:                                      # (iii)
        return False
    if (w - 1, w + 1) in g.fwd:                          # (iv)
        return False
    for z in range(w + 1, v):                            # (v)
        if z in g.back and g.back[z] <= w:
            return False
    return True


def encode_all(B):
    """Yield every graph Algorithm 1 can output for B (all random branches)."""
    n = len(B)
    g0 = G(n + 2, [(v, v + 1) for v in range(1, n + 2)], [], {})
    starts = []
    if n >= 2 and B[1] == 1:
        a = g0.copy(); a.back[2] = 1; starts.append(a)
        b = g0.copy(); b.nV = n + 3; b.path.add((n + 2, n + 3)); b.fwd.add((2, 4)); starts.append(b)
    else:
        starts.append(g0)

    def rec(g, i, v):
        while i <= n:
            if (v - 2, v) in g.fwd:
                v += 1
                continue
            bit = B[i - 1]
            cands = [w for w in range(1, v) if (v - w) % 2 == bit and admissible(g, w, v)]
            if cands:
                if (v - 1, v + 1) in g.fwd:
                    if bit == 1:
                        g.back[v] = v - 1
                        g.path.discard((v, v + 1))
                else:
                    for w in cands[:-1]:
                        h = g.copy(); h.back[v] = w
                        yield from rec(h, i + 1, v + 1)
                    g.back[v] = cands[-1]
            else:
                if bit == 1:
                    t = g.nV
                    g.nV += 1
                    g.path.add((t, t + 1))
                    g.fwd.add((v, v + 2))
            i += 1; v += 1
        yield g

    for s in starts:
        yield from rec(s, 3, 3)


def has_out(g, u):
    return (u, u + 2) in g.fwd or u in g.back


def improved_decode(g, n, fix_v2=False):
    missing = g.nV - 2 - n - len(g.fwd)
    S = set()
    out = [1]
    i = 2
    rules = []
    for v in range(2, g.nV - 1):
        if i > n:
            break
        if (v - 2, v) in g.fwd or v in S:
            continue
        if (v, v + 2) in g.fwd:
            b = 1; r = 'R1'
        elif v in g.back:
            b = (v - g.back[v]) % 2; r = 'R2'
        elif (v, v + 1) not in g.path:
            b = 1; r = 'R3'
        elif (v + 1, v + 2) not in g.path:
            b = 1; S.add(v + 2); missing -= 1; r = 'R4'
        elif (missing > 0 and not has_out(g, v) and not has_out(g, v + 1) and not has_out(g, v + 2)
              and ((fix_v2 and v == 2) or not any(admissible(g, w, v) for w in range(1, v) if (v - w) % 2 == 1))):
            b = 1; S.add(v + 2); missing -= 1; r = 'R5'
        else:
            b = 0; r = 'R0'
        out.append(b); rules.append((v, r))
        i += 1
    return out, rules


def original_decode(g):
    out = [1]
    for v in range(2, g.nV - 1):
        if (v - 2, v) in g.fwd:
            continue
        if (v, v + 2) in g.fwd:
            out.append(1)
        elif v in g.back:
            out.append((v - g.back[v]) % 2)
        else:
            out.append(0)
    return out


def delta(B, Bh):
    n = len(B)
    return sum(1 for j in range(n) if j >= len(Bh) or Bh[j] != B[j])


def enc_edges(g):
    return [('F', e) for e in sorted(g.fwd)] + [('R', (v, w)) for v, w in sorted(g.back.items())]


def remove(g, kind, e):
    h = g.copy()
    if kind == 'F':
        h.fwd.discard(e)
    else:
        del h.back[e[0]]
    return h


if __name__ == '__main__':
    NMAX = int(sys.argv[1]) if len(sys.argv) > 1 else 12
    fix = len(sys.argv) > 2 and sys.argv[2] == 'fix'
    for n in range(1, NMAX + 1):
        ngraphs = 0; worst = 0; bad_bound = []; bad_fwd = []; bad_while = []; nobij = 0; undistorted_bad = 0
        orig_clean_bad = 0
        for bits in product([0, 1], repeat=n - 1):
            B = [1] + list(bits)
            for g in encode_all(B):
                ngraphs += 1
                # sanity: undistorted decoding
                if improved_decode(g, n, fix)[0] != B:
                    undistorted_bad += 1
                if original_decode(g) != B:
                    orig_clean_bad += 1
                # bijection check: number of non-mute vertices
                mute = {g.nV - 1, g.nV} | {d for (_, d) in g.fwd}
                if g.nV - len(mute) != n:
                    nobij += 1
                for kind, e in enc_edges(g):
                    h = remove(g, kind, e)
                    Bh, rules = improved_decode(h, n, fix)
                    d = delta(B, Bh)
                    worst = max(worst, d)
                    if d > 1:
                        bad_bound.append((B, kind, e, Bh))
                    if kind == 'F' and d != 0:
                        bad_fwd.append((B, e, Bh))
                    if kind == 'R' and (e[1] == e[0] - 1) and (e[1], e[0] + 1) in g.fwd and d != 0:
                        bad_while.append((B, e, Bh))
        print(f"n={n:2d} graphs={ngraphs:7d} worstDelta={worst} |d>1|={len(bad_bound)} "
              f"fwdNotExact={len(bad_fwd)} whileNotExact={len(bad_while)} "
              f"bijectionBroken={nobij} cleanImprovedWrong={undistorted_bad} cleanOriginalWrong={orig_clean_bad}")
        for x in (bad_bound[:2] + bad_fwd[:2]):
            print('   example:', x)
