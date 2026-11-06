// Labeled contraction recognition of Dijkstra graphs (Algorithm 3 of the revised paper).
//
// Differences from the procedure of Bento et al. (DAM 2019):
//   - closedness condition (3) on cycle edges entering s(H) is not used;
//   - (P3) a composite vertex (result of an earlier contraction) may only occupy an
//     X position of the prime (sinks, middle vertices except the repeat middle, repeat source);
//   - (P4) the sink of the prime has at most one out-neighbour;
//   - (P5) the entry s(G) may only occupy the source position of a prime;
//   - after a successful contraction, further contractions are attempted at the same vertex.
// Soundness (Theorem 8): if this function returns 1, the graph is a Dijkstra graph with entry s(G).
// The vertices may be given in any order; the source is vertex `source`.

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dijkstra/dijkstra.h"

typedef struct {
    unsigned long n;
    unsigned long source;   // entry vertex s(G)
    uint8_t* a;      // adjacency matrix, a[u*n+v] = 1 if edge u->v
    uint8_t* alive;
    uint8_t* comp;   // composite vertices
    int literal;     // 1: procedure of Bento et al. (DAM 2019) as published: closedness (3), no (P3)-(P5)
} LG;

#define E(g,u,v) ((g)->a[(u)*(g)->n+(v)])

static unsigned long lg_outdeg(LG* g, unsigned long u) {
    unsigned long d = 0;
    for (unsigned long v = 0; v < g->n; v++) d += g->alive[v] && E(g,u,v);
    return d;
}

// slots: H[0] = source, H[k-1] = sink; xslot[j] = 1 if slot j is an X vertex.
// want: list of (i,j) slot pairs that must be edges; all other ordered pairs inside H must not be.
static int lg_try(LG* g, const unsigned long* H, unsigned long k,
                  const uint8_t* want /* k*k */, const uint8_t* xslot) {
    // distinct vertices
    for (unsigned long i = 0; i < k; i++)
        for (unsigned long j = i + 1; j < k; j++)
            if (H[i] == H[j]) return 0;
    // induced subgraph equals the statement graph
    for (unsigned long i = 0; i < k; i++)
        for (unsigned long j = 0; j < k; j++) {
            if (i == j) continue;
            if (!!E(g, H[i], H[j]) != !!want[i*k+j]) return 0;
        }
    // closedness (1) and (2)
    for (unsigned long w = 0; w < g->n; w++) {
        if (!g->alive[w]) continue;
        int inH = 0;
        for (unsigned long i = 0; i < k; i++) if (H[i] == w) inH = 1;
        if (inH) continue;
        for (unsigned long i = 0; i < k; i++) {
            if (i != 0 && E(g, w, H[i])) return 0;       // (1) in-edge from outside to a non-source
            if (i != k - 1 && E(g, H[i], w)) return 0;   // (2) out-edge to outside from a non-sink
        }
    }
    if (g->literal) {
        // closedness (3) of [BENTO:2019]: every cycle edge entering s(H) comes from an out-neighbour of s(H).
        // Cycle edges are those of a depth-first search of the current graph from its source.
        unsigned long n = g->n, x = H[0];
        uint8_t* state = calloc(n, 1); unsigned long* st = malloc(n * sizeof(unsigned long));
        unsigned long* nx = calloc(n, sizeof(unsigned long)); unsigned long sp = 0; int ok = 1;
        st[sp++] = g->source; state[g->source] = 1;
        while (sp && ok) {
            unsigned long v = st[sp-1]; int pushed = 0;
            while (nx[v] < n) {
                unsigned long w = nx[v]++;
                if (!g->alive[w] || !E(g, v, w)) continue;
                if (state[w] == 1) { if (w == x && !E(g, x, v)) ok = 0; }
                else if (state[w] == 0) { state[w] = 1; st[sp++] = w; pushed = 1; break; }
            }
            if (!pushed) { state[v] = 2; sp--; }
        }
        free(state); free(st); free(nx);
        (void)xslot;
        return ok;
    }
    // (P3) composite vertices only in X slots
    for (unsigned long i = 0; i < k; i++)
        if (g->comp[H[i]] && !xslot[i]) return 0;
    // (P4) sink has at most one out-neighbour
    if (lg_outdeg(g, H[k-1]) > 1) return 0;
    // (P5) the entry s(G) may only be the source of a prime
    for (unsigned long i = 1; i < k; i++) if (H[i] == g->source) return 0;
    return 1;
}

static void lg_contract(LG* g, const unsigned long* H, unsigned long k) {
    unsigned long x = H[0], t = H[k-1];
    unsigned long n = g->n;
    // out-edges of t to outside H become out-edges of x
    uint8_t* tout = calloc(n, 1);
    for (unsigned long w = 0; w < n; w++) tout[w] = g->alive[w] && E(g, t, w);
    for (unsigned long i = 0; i < k; i++) {
        unsigned long v = H[i];
        for (unsigned long w = 0; w < n; w++) {
            if (v != x) { E(g, v, w) = 0; E(g, w, v) = 0; }
        }
        E(g, x, v) = 0;
    }
    for (unsigned long w = 0; w < n; w++) {
        int inH = 0;
        for (unsigned long i = 0; i < k; i++) if (H[i] == w) inH = 1;
        if (tout[w] && !inH) E(g, x, w) = 1;
    }
    E(g, x, x) = 0;
    for (unsigned long i = 1; i < k; i++) { g->alive[H[i]] = 0; if (H[i] == g->source) g->source = x; }
    g->comp[x] = 1;
    free(tout);
}

// try every statement graph with source x; contract the first valid prime found
static int lg_step(LG* g, unsigned long x) {
    unsigned long n = g->n;
    unsigned long O[n]; unsigned long no = 0;
    for (unsigned long v = 0; v < n; v++) if (g->alive[v] && v != x && E(g, x, v)) O[no++] = v;
    if (no == 0) return 0;

    if (no == 1) {
        unsigned long y = O[0];
        // sequence: x(R) -> y(X)
        { unsigned long H[2] = {x, y}; uint8_t w[4] = {0,1, 0,0}; uint8_t xs[2] = {0,1};
          if (lg_try(g, H, 2, w, xs)) { lg_contract(g, H, 2); return 1; } }
        // repeat: x(X) -> m(R) -> x, m -> t(X)
        for (unsigned long t = 0; t < n; t++) {
            if (!g->alive[t] || t == x || t == y || !E(g, y, t)) continue;
            unsigned long H[3] = {x, y, t};
            uint8_t w[9] = {0,1,0, 1,0,1, 0,0,0}; uint8_t xs[3] = {1,0,1};
            if (lg_try(g, H, 3, w, xs)) { lg_contract(g, H, 3); return 1; }
        }
        return 0;
    }
    if (no == 2) {
        for (int r = 0; r < 2; r++) {
            unsigned long m = O[r], t = O[1-r];
            // while: x(R) -> m(X) -> x, x -> t(X)
            { unsigned long H[3] = {x, m, t}; uint8_t w[9] = {0,1,1, 1,0,0, 0,0,0}; uint8_t xs[3] = {0,1,1};
              if (lg_try(g, H, 3, w, xs)) { lg_contract(g, H, 3); return 1; } }
            // if-then: x(R) -> m(X) -> t(X), x -> t
            { unsigned long H[3] = {x, m, t}; uint8_t w[9] = {0,1,1, 0,0,1, 0,0,0}; uint8_t xs[3] = {0,1,1};
              if (lg_try(g, H, 3, w, xs)) { lg_contract(g, H, 3); return 1; } }
        }
    }
    // if-then-else / p-case: x(R) -> y_1..y_p (X) -> t(X)
    {
        unsigned long y0 = O[0], t = n;
        for (unsigned long v = 0; v < n; v++) if (g->alive[v] && v != y0 && E(g, y0, v)) { t = v; break; }
        if (t == n) return 0;
        unsigned long k = no + 2;
        unsigned long H[k]; H[0] = x; for (unsigned long i = 0; i < no; i++) H[i+1] = O[i]; H[k-1] = t;
        uint8_t* w = calloc(k*k, 1); uint8_t xs[k];
        for (unsigned long i = 1; i <= no; i++) { w[0*k+i] = 1; w[i*k+(k-1)] = 1; }
        xs[0] = 0; for (unsigned long i = 1; i < k; i++) xs[i] = 1;
        int ok = lg_try(g, H, k, w, xs);
        free(w);
        if (ok) { lg_contract(g, H, k); return 1; }
    }
    return 0;
}

// literal = 1: recognition procedure of [BENTO:2019]; once = 1: at most one contraction per vertex,
// as in the pseudo-code of [BENTO:2019] (otherwise contractions are repeated at each vertex).
static int recognize(unsigned long n, const uint8_t* adj, unsigned long source, int literal, int once) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    unsigned long m = 0;
    for (unsigned long i = 0; i < n*n; i++) m += !!adj[i];
    for (unsigned long i = 0; i < n; i++) if (adj[i*n+i]) return 0;   // no loops
    if (m > 2*n - 2) return 0;

    LG g = { n, source, malloc(n*n), malloc(n), calloc(n, 1), literal };
    memcpy(g.a, adj, n*n);
    memset(g.alive, 1, n);

    // cycle edges of a DFS from the source
    uint8_t* state = calloc(n, 1);          // 0 new, 1 on stack, 2 done
    uint8_t* ce = calloc(n*n, 1);
    unsigned long* st = malloc(n * sizeof(unsigned long));
    unsigned long* nx = calloc(n, sizeof(unsigned long));
    unsigned long sp = 0; st[sp++] = source; state[source] = 1;
    while (sp) {
        unsigned long v = st[sp-1];
        int pushed = 0;
        while (nx[v] < n) {
            unsigned long w = nx[v]++;
            if (!adj[v*n+w]) continue;
            if (state[w] == 1) ce[v*n+w] = 1;
            else if (state[w] == 0) { state[w] = 1; st[sp++] = w; pushed = 1; break; }
        }
        if (!pushed) { state[v] = 2; sp--; }
    }
    int result = 0;
    for (unsigned long v = 0; v < n; v++) if (!state[v]) goto done;   // not a flow graph

    // topological sort of G - E_C (Kahn)
    {
        unsigned long* indeg = calloc(n, sizeof(unsigned long));
        unsigned long* order = malloc(n * sizeof(unsigned long));
        for (unsigned long u = 0; u < n; u++) for (unsigned long v = 0; v < n; v++)
            if (adj[u*n+v] && !ce[u*n+v]) indeg[v]++;
        unsigned long qh = 0, qt = 0;
        for (unsigned long v = 0; v < n; v++) if (!indeg[v]) order[qt++] = v;
        while (qh < qt) {
            unsigned long u = order[qh++];
            for (unsigned long v = 0; v < n; v++)
                if (adj[u*n+v] && !ce[u*n+v] && --indeg[v] == 0) order[qt++] = v;
        }
        if (qt == n) {
            for (unsigned long i = n; i-- > 0;) {
                unsigned long x = order[i];
                if (once) { if (g.alive[x]) lg_step(&g, x); }
                else while (g.alive[x] && lg_step(&g, x)) ;
            }
            unsigned long alive = 0;
            for (unsigned long v = 0; v < n; v++) alive += g.alive[v];
            result = literal ? (alive == 1) : (alive == 1 && g.alive[source]);
        }
        free(indeg); free(order);
    }
done:
    free(state); free(ce); free(st); free(nx);
    free(g.a); free(g.alive); free(g.comp);
    return result;
}

int dijkstra_labeled_core(unsigned long n, const uint8_t* adj, unsigned long source) {
    return recognize(n, adj, source, 0, 0);
}

// Recognition procedure of Bento et al. (DAM 2019) as published, for comparison (Section 4.4):
// closedness conditions (1)-(3), no labels, no condition on the entry.
int dijkstra_check_bento2019(GRAPH* graph, int once) {
    unsigned long n = graph->num_nodes;
    uint8_t* adj = calloc(n*n, 1);
    for (unsigned long u = 0; u < n; u++)
        for (CONNECTION* c = graph->nodes[u]->out; c; c = c->next)
            adj[u*n + c->node->graph_idx] = 1;
    int r = recognize(n, adj, 0, 1, once);
    free(adj);
    return r;
}

int dijkstra_check_labeled(GRAPH* graph) {
    unsigned long n = graph->num_nodes;
    uint8_t* adj = calloc(n*n, 1);
    for (unsigned long u = 0; u < n; u++)
        for (CONNECTION* c = graph->nodes[u]->out; c; c = c->next)
            adj[u*n + c->node->graph_idx] = 1;
    int r = dijkstra_labeled_core(n, adj, 0);
    free(adj);
    return r;
}
