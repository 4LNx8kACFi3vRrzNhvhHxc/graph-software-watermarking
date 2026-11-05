#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>

struct NODE;

typedef enum {
    INVALID,
    TRIVIAL=1,
    SEQUENCE=2,
    IF_THEN=3,
    WHILE=4,
    REPEAT=5,
    IF_THEN_ELSE=6,
    P_CASE=7,
} STATEMENT_GRAPH;

typedef struct PRIME_SUBGRAPH {

    struct NODE* sink;
    STATEMENT_GRAPH type;
} PRIME_SUBGRAPH;

#include "graph/graph.h"

int dijkstra_check(GRAPH* graph);

char* dijkstra_get_code(GRAPH* watermark);

int dijkstra_is_equal(GRAPH*, GRAPH*);

GRAPH* dijkstra_generate(char* code);
// labeled contraction recognition (Algorithm 3 of the revised paper); sound for any vertex order
int dijkstra_check_labeled(GRAPH* graph);
int dijkstra_labeled_core(unsigned long n, const uint8_t* adj, unsigned long source);
// recognition procedure of [BENTO:2019] as published (closedness (1)-(3), no labels); once = 1 performs
// at most one contraction per vertex, as in its pseudo-code; used only for comparison
int dijkstra_check_bento2019(GRAPH* graph, int once);

#endif
