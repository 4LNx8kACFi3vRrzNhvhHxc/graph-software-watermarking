"""Writes the inputs of the Section 4.4 tests:
  dgs8_rooted.txt  all Dijkstra graphs with at most 8 vertices (derivations, up to isomorphisms preserving the entry)
  rand60k.txt      60,000 random flow graphs (seed 1, at most 8 vertices, m <= 2n-2)
  rand60k_isdg.txt for each random graph, 1 if it is a Dijkstra graph with entry 0
  rand60k_anyentry.txt for each random graph, 1 if it is a Dijkstra graph for some choice of entry
Graphs are written as "n m u1 v1 ...", entry = vertex 0, vertices in a topological order of G - E_C
with the sink last (as required by dijkstra_check). Requires networkx."""
import random, networkx as nx
import lr, lr_gen_rooted as R

def relabel(V, E, s):
    ce = lr.cycle_edges(V, E, s); outd = {v: [b for (a, b) in E if a == v] for v in V}
    sinks = [v for v in V if not outd[v]]; snk = sinks[0] if len(sinks) == 1 else None
    D = nx.DiGraph(); D.add_nodes_from(V); D.add_edges_from(E - ce)
    indeg = {v: D.in_degree(v) for v in V}; order = []; ready = sorted(v for v in V if indeg[v] == 0 and v != snk)
    while ready:
        u = ready.pop(0); order.append(u)
        for w in sorted(D.successors(u)):
            indeg[w] -= 1
            if indeg[w] == 0 and w != snk: ready.append(w)
    if snk is not None and indeg[snk] == 0: order.append(snk)
    if len(order) != len(V) or order[0] != s: return len(V), sorted(E)
    mp = {v: i for i, v in enumerate(order)}; return len(V), [(mp[a], mp[b]) for (a, b) in E]

def write(f, n, E): f.write(f"{n} {len(E)} " + " ".join(f"{a} {b}" for a, b in E) + "\n")

out = R.generate_rooted(8)
# the same graphs without their entry, to test "Dijkstra graph for some choice of entry"
unrooted = {}
for b in out.values():
    for Q in b:
        P = nx.DiGraph(Q.edges); P.add_nodes_from(Q.nodes)
        unrooted.setdefault(nx.weisfeiler_lehman_graph_hash(P, iterations=5), []).append(P)
def any_entry_dg(V, E):
    G = nx.DiGraph(); G.add_nodes_from(V); G.add_edges_from(E)
    return any(nx.is_isomorphic(G, P) for P in unrooted.get(nx.weisfeiler_lehman_graph_hash(G, iterations=5), []))
with open('dgs8_rooted.txt', 'w') as f:
    for b in out.values():
        for Q in b: write(f, *relabel(set(Q.nodes), set(Q.edges), 0))
random.seed(1)
with open('rand60k.txt', 'w') as f, open('rand60k_isdg.txt', 'w') as g, open('rand60k_anyentry.txt', 'w') as h:
    for _ in range(60000):
        k = random.randint(2, 8); V = set(range(k)); mm = random.randint(k - 1, 2 * k - 2); E = set()
        for v in range(1, k): E.add((random.randrange(v), v))
        while len(E) < mm:
            a, b = random.randrange(k), random.randrange(k)
            if a != b: E.add((a, b))
        write(f, *relabel(V, E, 0)); g.write("1\n" if R.is_rooted_dg(out, V, E) else "0\n")
        h.write("1\n" if any_entry_dg(V, E) else "0\n")
