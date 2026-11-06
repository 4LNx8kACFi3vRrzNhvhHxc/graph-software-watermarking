// Harness for Section 4.4 of the revised paper, linked against the repository's code.
//  h wm N        : all watermark graphs of watermark_encode for identifiers of 1..N bits,
//                  over ALL random choices of the encoder; runs dijkstra_check (repository)
//                  and dijkstra_check_labeled (Algorithm 3). With DUMP=file, writes the graphs.
//  h file PATH   : graphs from a file, one per line: "n m u1 v1 ... um vm", source = 0,
//                  vertices already in a topological order of G - E_C with the sink last;
//                  prints "<repo> <labeled> <bento2019> <bento2019 once>" per line.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "graph/graph.h"
#include "encoder/encoder.h"
#include "dijkstra/dijkstra.h"

// ---- replacement of the two random choices of watermark_encode: enumerates all of them --------
// get_backedge_index returns a uniform index among the n admissible candidates, and
// encoder_choose_forward_at_2 chooses between [1 <- 2] and [2 -> 4]; enumerating 0..n-1 and {0,1}
// covers exactly their possible outputs.
#define MAXD 512
static unsigned long script[MAXD], counts[MAXD], depth, script_len;
static unsigned long choose(unsigned long n) {
    unsigned long c = depth < script_len ? script[depth] : 0;
    if (depth >= script_len) { script[depth] = 0; script_len = depth + 1; }
    counts[depth++] = n;
    return c;
}
unsigned long get_backedge_index(STACK* possible_backedges, GRAPH* graph, unsigned long current_idx) {
    (void)graph; (void)current_idx;
    return choose(possible_backedges->n);
}
uint8_t encoder_choose_forward_at_2(void) { return (uint8_t)choose(2); }

static void dump(FILE* f, GRAPH* g) {
    unsigned long m = 0;
    for (unsigned long u = 0; u < g->num_nodes; u++) for (CONNECTION* c = g->nodes[u]->out; c; c = c->next) m++;
    fprintf(f, "%lu %lu", g->num_nodes, m);
    for (unsigned long u = 0; u < g->num_nodes; u++) for (CONNECTION* c = g->nodes[u]->out; c; c = c->next)
        fprintf(f, " %lu %lu", u, c->node->graph_idx);
    fprintf(f, "\n");
}

int main(int argc, char** argv) {
    if (argc < 3) return 1;
    if (!strcmp(argv[1], "wm")) {
        unsigned long N = strtoul(argv[2], 0, 10);
        FILE* df = getenv("DUMP") ? fopen(getenv("DUMP"), "w") : NULL;
        for (unsigned long n = 1; n <= N; n++) {
            unsigned long total = 0, rej_repo = 0, rej_lab = 0, rej_2019 = 0;
            for (unsigned long id = 1UL << (n-1); id < (1UL << n); id++) {
                uint8_t data[8] = {0};
                for (unsigned long j = 0; j < n; j++) set_bit(data, j, (id >> (n-1-j)) & 1);
                script_len = 0;
                for (;;) {
                    depth = 0;
                    GRAPH* g = watermark_encode(data, n);
                    unsigned long used = depth;
                    total++;
                    if (df) { fprintf(df, "%lu ", id); dump(df, g); }
                    if (!dijkstra_check(g)) rej_repo++;
                    if (!dijkstra_check_labeled(g)) rej_lab++;
                    if (!dijkstra_check_bento2019(g, 0)) rej_2019++;
                    graph_free(g);
                    // next script (odometer over the choices actually used)
                    script_len = used;
                    long d = (long)used - 1;
                    while (d >= 0 && script[d] + 1 >= counts[d]) d--;
                    if (d < 0) break;
                    script[d]++;
                    script_len = d + 1;
                }
            }
            printf("n=%2lu graphs=%7lu rejected_by_repo_dijkstra_check=%lu rejected_by_labeled=%lu rejected_by_bento2019=%lu\n", n, total, rej_repo, rej_lab, rej_2019);
            fflush(stdout);
        }
        if (df) fclose(df);
        return 0;
    }
    if (!strcmp(argv[1], "file")) {
        FILE* f = fopen(argv[2], "r");
        unsigned long n, m;
        while (fscanf(f, "%lu %lu", &n, &m) == 2) {
            GRAPH* g = graph_create(n);
            uint8_t* adj = calloc(n*n, 1);
            for (unsigned long i = 0; i < m; i++) {
                unsigned long u, v; if (fscanf(f, "%lu %lu", &u, &v) != 2) return 2;
                graph_oriented_connect(g->nodes[u], g->nodes[v]);
                adj[u*n+v] = 1;
            }
            int r1;
            pid_t pid = fork();
            if (pid == 0) { ualarm(100000, 0); _exit(dijkstra_check(g) ? 1 : 0); }
            int status; waitpid(pid, &status, 0);
            if (WIFEXITED(status)) r1 = WEXITSTATUS(status); else r1 = 9;   // 9 = did not terminate (killed by alarm)
            int r2 = dijkstra_check_labeled(g);
            int r3 = dijkstra_check_bento2019(g, 0);
            int r4 = dijkstra_check_bento2019(g, 1);
            printf("%d %d %d %d\n", r1, r2, r3, r4); fflush(stdout);
            free(adj); graph_free(g);
        }
        fclose(f);
        return 0;
    }
    return 1;
}
