"""Labeled contraction recognizer (LR) for Dijkstra graphs, plus a generator of Dijkstra graphs by
labeled expansions (definition of Bento et al., DAM 2019) to test soundness/completeness empirically.

LR = contraction of primes as in DAM 2019, with:
  - closedness condition (3) on cycle edges into s(H) REMOVED;
  - (P3) a composite vertex (result of an earlier contraction) may occupy only an X position of the prime;
  - (P4) the sink of the prime has out-degree <= 1 (necessary: X vertices have out-degree <= 1).
Strategy: bottom-up over a topological order of G - E_C, contracting repeatedly at each vertex.
"""
import itertools, random
import networkx as nx

# statement graphs: (edges among slots, labels per slot); slot 0 = source, slot -1 = sink
def statement_graphs(pmax=3):
    S = {}
    S['seq'] = ([(0, 1)], ['R', 'X'])
    S['if'] = ([(0, 1), (1, 2), (0, 2)], ['R', 'X', 'X'])
    S['ifelse'] = ([(0, 1), (0, 2), (1, 3), (2, 3)], ['R', 'X', 'X', 'X'])
    for p in range(3, pmax + 1):
        S[f'case{p}'] = ([(0, j) for j in range(1, p + 1)] + [(j, p + 1) for j in range(1, p + 1)], ['R'] + ['X'] * p + ['X'])
    S['while'] = ([(0, 1), (1, 0), (0, 2)], ['R', 'X', 'X'])
    S['repeat'] = ([(0, 1), (1, 0), (1, 2)], ['X', 'R', 'X'])
    return S

ST = statement_graphs(6)


def cycle_edges(V, E, s):
    out = {v: sorted(b for (a, b) in E if a == v) for v in V}
    state = {v: 0 for v in V}; ce = set()
    stack = [(s, iter(out[s]))]; state[s] = 1
    while stack:
        v, it = stack[-1]
        for w in it:
            if state[w] == 1: ce.add((v, w))
            elif state[w] == 0:
                state[w] = 1; stack.append((w, iter(out[w]))); break
        else:
            state[v] = 2; stack.pop()
    return ce


def topo(V, E):
    g = nx.DiGraph(); g.add_nodes_from(V); g.add_edges_from(E)
    if not nx.is_directed_acyclic_graph(g): return None
    return list(nx.topological_sort(g))


def primes_at(x, V, E, out, inn, cond3, ce):
    """Yield (H as dict slot->vertex list, type) for every prime with source x (DAM closedness (1),(2); (3) optional)."""
    res = []
    O = out[x]
    def induced(H):
        return {(a, b) for (a, b) in E if a in H and b in H}
    def closed(H, src, snk):
        for v in H:
            if v != src and not inn[v] <= H: return False
            if v != snk and not out[v] <= H: return False
        if cond3:
            for (a, b) in ce:
                if b == src and a not in out[src]: return False
        return True
    for name, (sedges, labels) in ST.items():
        k = len(labels)
        # enumerate assignments of slots to vertices consistent with source x
        cand = [v for v in V if v != x]
        # cheap: slot vertices must be within out(x) U out(out(x))
        near = set(O) | {w for u in O for w in out[u]}
        near.discard(x)
        for perm in itertools.permutations(sorted(near), k - 1):
            m = [x] + list(perm)
            H = set(m)
            want = {(m[a], m[b]) for (a, b) in sedges}
            if induced(H) == want and closed(H, m[0], m[-1]):
                res.append((name, m, labels))
    return res


def LR(V, E, s, cond3=False, labels=True, p4=True, strategy='bottomup', p5=True):
    V = set(V); E = set(E)
    comp = set()
    ce = cycle_edges(V, E, s)
    order = topo(V, E - ce)
    if order is None: return False
    for x in reversed(order):
        while x in V and len(V) > 1:
            out = {v: set() for v in V}; inn = {v: set() for v in V}
            for a, b in E: out[a].add(b); inn[b].add(a)
            ce_now = cycle_edges(V, E, s) if cond3 else set()
            done = False
            for name, m, lab in primes_at(x, V, E, out, inn, cond3, ce_now):
                if labels and any(m[j] in comp and lab[j] != 'X' for j in range(len(m))): continue
                if p4 and len(out[m[-1]]) > 1: continue
                if p5 and s in m[1:]: continue
                H = set(m)
                E = {(x if a in H else a, x if b in H else b) for (a, b) in E}
                E = {(a, b) for (a, b) in E if a != b}
                if s in H: s = x
                V = (V - H) | {x}; comp.add(x)
                done = True; break
            if not done: break
    return len(V) == 1


# ---------- generator of Dijkstra graphs (labeled expansions) ----------
def expand(G, lab, v, name):
    sedges, slabels = ST[name]
    k = len(slabels)
    nv = max(G.nodes) + 1
    new = [v] + list(range(nv, nv + k - 1))   # reuse v as source
    ins = list(G.in_edges(v)); outs = list(G.out_edges(v))
    H = G.copy(); L = dict(lab)
    H.remove_edges_from(outs)
    for j in range(1, k): H.add_node(new[j])
    for (a, b) in sedges: H.add_edge(new[a], new[b])
    for (_, w) in outs: H.add_edge(new[-1], w)
    for j in range(k): L[new[j]] = slabels[j]
    return H, L


def canon(G):
    return nx.weisfeiler_lehman_graph_hash(G, iterations=4)


def generate(maxv):
    G0 = nx.DiGraph(); G0.add_node(0)
    seen = {}  # hash -> list of graphs
    frontier = [(G0, {0: 'X'})]
    allg = []
    def add(G):
        h = canon(G)
        for H in seen.get(h, []):
            if nx.is_isomorphic(G, H): return False
        seen.setdefault(h, []).append(G); allg.append(G); return True
    add(G0)
    # BFS over labeled graphs; dedupe labeled states by (hash of graph + labels)
    lseen = set(); lbuckets = {}
    while frontier:
        nxt = []
        for G, lab in frontier:
            for v in [u for u in G.nodes if lab[u] == 'X']:
                for name, (se, sl) in ST.items():
                    if G.number_of_nodes() + len(sl) - 1 > maxv: continue
                    H, L = expand(G, lab, v, name)
                    key = nx.weisfeiler_lehman_graph_hash(H, node_attr='l', iterations=4) if False else None
                    nx.set_node_attributes(H, L, 'l')
                    key = nx.weisfeiler_lehman_graph_hash(H, node_attr='l', iterations=5)
                    bucket = lbuckets.setdefault(key, [])
                    if any(nx.is_isomorphic(H, K, node_match=lambda x, y: x['l'] == y['l']) for K in bucket): continue
                    bucket.append(H)
                    add(nx.DiGraph(H.edges) if H.number_of_edges() else H)
                    nxt.append((H, L))
        frontier = nxt
    return allg, seen


def source_of(G):
    srcs = [v for v in G.nodes if all(nx.has_path(G, v, u) for u in G.nodes)]
    return srcs
