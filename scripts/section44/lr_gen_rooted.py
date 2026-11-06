"""Rooted generator: Dijkstra graphs (with their entry = vertex 0) by labeled derivations,
deduplicated up to isomorphism FIXING the entry vertex."""
import networkx as nx
import lr

def generate_rooted(maxv):
    G0 = nx.DiGraph(); G0.add_node(0)
    lb = {}; frontier = [(G0, {0: 'X'})]; out = {}
    def mark(H, L=None):
        K = H.copy()
        nx.set_node_attributes(K, {v: (1 if v == 0 else 0) for v in K}, 'src')
        if L is not None: nx.set_node_attributes(K, L, 'l')
        return K
    def add_unlabeled(H):
        K = mark(H); h = nx.weisfeiler_lehman_graph_hash(K, node_attr='src', iterations=5)
        b = out.setdefault(h, [])
        if any(nx.is_isomorphic(K, Q, node_match=lambda a, c: a['src'] == c['src']) for Q in b): return
        b.append(K)
    add_unlabeled(G0)
    while frontier:
        nxt = []
        for G, lab in frontier:
            for v in [u for u in G.nodes if lab[u] == 'X']:
                for name, (se, sl) in lr.ST.items():
                    if G.number_of_nodes() + len(sl) - 1 > maxv: continue
                    H, L = lr.expand(G, lab, v, name)
                    K = mark(H, L)
                    for u in K: K.nodes[u]['k'] = f"{K.nodes[u]['l']}{K.nodes[u]['src']}"
                    key = nx.weisfeiler_lehman_graph_hash(K, node_attr='k', iterations=5)
                    b = lb.setdefault(key, [])
                    if any(nx.is_isomorphic(K, Q, node_match=lambda a, c: a['k'] == c['k']) for Q in b): continue
                    b.append(K)
                    add_unlabeled(H)
                    nxt.append((H, L))
        frontier = nxt
    return out   # hash -> list of rooted graphs (attr 'src')

def is_rooted_dg(out, V, E, s=0):
    G = nx.DiGraph(); G.add_nodes_from(V); G.add_edges_from(E)
    nx.set_node_attributes(G, {v: (1 if v == s else 0) for v in G}, 'src')
    h = nx.weisfeiler_lehman_graph_hash(G, node_attr='src', iterations=5)
    return any(nx.is_isomorphic(G, Q, node_match=lambda a, c: a['src'] == c['src']) for Q in out.get(h, []))
