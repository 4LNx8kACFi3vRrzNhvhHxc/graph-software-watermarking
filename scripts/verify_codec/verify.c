// Enumerates every random choice of watermark_encode (both choice points overridden) and, for each
// graph, writes: id, edges, and the output of watermark_decode_improved after removing each encoding edge.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph/graph.h"
#include "encoder/encoder.h"
#include "decoder/decoder.h"

#define MAXD 512
static unsigned long script[MAXD], counts[MAXD], depth, script_len;
static unsigned long choose(unsigned long n) {
    unsigned long c = depth < script_len ? script[depth] : 0;
    if(depth >= script_len) { script[depth] = 0; script_len = depth + 1; }
    counts[depth++] = n;
    return c;
}
unsigned long get_backedge_index(STACK* s, GRAPH* g, unsigned long idx) { (void)g; (void)idx; return choose(s->n); }
uint8_t encoder_choose_forward_at_2(void) { return (uint8_t)choose(2); }

static void decode_print(FILE* f, GRAPH* g, uint8_t* data, unsigned long n) {
    unsigned long nb = n;
    uint8_t* r = watermark_decode_improved(g, data, &nb);
    unsigned long bytes = nb / 8 + !!(nb % 8);
    unsigned long start = bytes * 8 - nb;
    for(unsigned long j = 0; j < nb; j++) fputc('0' + get_bit(r, start + j), f);
    free(r);
}

int main(int argc, char** argv) {
    unsigned long N = strtoul(argv[1], 0, 10);
    FILE* f = fopen(argv[2], "w");
    unsigned long total = 0;
    for(unsigned long n = 1; n <= N; n++) {
        for(unsigned long id = 1UL << (n-1); id < (1UL << n); id++) {
            uint8_t data[8] = {0};
            for(unsigned long j = 0; j < n; j++) set_bit(data, j, (id >> (n-1-j)) & 1);
            script_len = 0;
            for(;;) {
                depth = 0;
                GRAPH* g = watermark_encode(data, n);
                unsigned long used = depth;
                total++;
                fprintf(f, "G %lu %lu", id, g->num_nodes);
                for(unsigned long u = 0; u < g->num_nodes; u++)
                    for(CONNECTION* c = g->nodes[u]->out; c; c = c->next) fprintf(f, " %lu,%lu", u+1, c->node->graph_idx+1);
                fprintf(f, "\nD - ");
                decode_print(f, g, data, n); fputc('\n', f);
                if(getenv("PAIRS")) {
                    unsigned long eu[64], ev[64], ne = 0;
                    for(unsigned long u = 0; u < g->num_nodes; u++)
                        for(CONNECTION* c = g->nodes[u]->out; c; c = c->next)
                            if(c->node->graph_idx != u + 1) { eu[ne] = u; ev[ne] = c->node->graph_idx; ne++; }
                    for(unsigned long a1 = 0; a1 < ne; a1++) for(unsigned long a2 = a1 + 1; a2 < ne; a2++) {
                        GRAPH* h = graph_copy(g);
                        graph_oriented_disconnect(h->nodes[eu[a1]], h->nodes[ev[a1]]);
                        graph_oriented_disconnect(h->nodes[eu[a2]], h->nodes[ev[a2]]);
                        fprintf(f, "P %lu,%lu;%lu,%lu ", eu[a1]+1, ev[a1]+1, eu[a2]+1, ev[a2]+1);
                        decode_print(f, h, data, n); fputc('\n', f);
                        graph_free(h);
                    }
                }
                for(unsigned long u = 0; u < g->num_nodes; u++) {
                    for(CONNECTION* c = g->nodes[u]->out; c; c = c->next) {
                        unsigned long v = c->node->graph_idx;
                        if(v == u + 1) continue;           // backbone edge
                        GRAPH* h = graph_copy(g);
                        graph_oriented_disconnect(h->nodes[u], h->nodes[v]);
                        fprintf(f, "D %lu,%lu ", u+1, v+1);
                        decode_print(f, h, data, n); fputc('\n', f);
                        graph_free(h);
                    }
                }
                graph_free(g);
                script_len = used;
                long d = (long)used - 1;
                while(d >= 0 && script[d] + 1 >= counts[d]) d--;
                if(d < 0) break;
                script[d]++; script_len = d + 1;
            }
        }
    }
    fclose(f);
    printf("graphs: %lu\n", total);
    return 0;
}
