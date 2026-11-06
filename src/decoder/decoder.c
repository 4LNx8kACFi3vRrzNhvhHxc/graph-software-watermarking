#include "decoder/decoder.h"

#define show_bits(bits,len) fprintf(stderr, "%s:%d:" #bits ":", __FILE__, __LINE__);\
  for(unsigned long i = 0; i < len; i++) {\
    fprintf(stderr, "%hhu", get_bit(bits, i));\
  }\
  fprintf(stderr, "\n");

void* watermark2014_decode(GRAPH* graph, unsigned long* num_bytes) {

    unsigned long n_bits = graph->num_nodes-1;
    uint8_t bits[n_bits];
    bits[0] = 1;
    for(unsigned long i = 1; i < n_bits; i++) {

        // if this node has two out connections
        if(graph->nodes[i]->num_out_neighbours == 2) {

            // get backedge
            CONNECTION* conn = graph_get_backedge(graph->nodes[i]);
            bits[i] = ( graph->nodes[i]->graph_idx - conn->node->graph_idx ) & 1;
        } else {
            bits[i] = 0;
        }
    }
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}

void* watermark_decode_improved8(GRAPH* graph, uint8_t* data, unsigned long* num_bytes) {

  // convert to bits
  *num_bytes *= 8;
  void* res = watermark_decode_improved(graph, data, num_bytes);
  // convert to bytes
  *num_bytes = *num_bytes / 8 + !!(*num_bytes % 8);
  return res;
}

// Improved decoding: Algorithm 4 of the paper.
// BLIND decoding: only the length n of `data` is used (position of its leading 1 and total size);
// its bit values are never read. Vertices are numbered 1..N along the backbone (assumption (A1)).
//
// Candidate lists (one per parity) are maintained on the possibly distorted graph G' for every
// vertex, mute or not, exactly as in the encoder: a vertex u is inserted after it is processed if it
// is not the origin of a back edge and [u-1 -> u+1] is not in G'; when a back edge [w <- u] is met,
// w and every later candidate are removed from both lists. Together with conditions (i) and (ii),
// tested explicitly, this decides in O(1) whether an odd-span back edge [w <- v] is admissible.
static void wm_list_truncate(unsigned long* list, unsigned long* size, unsigned long w) {
    while(*size && list[*size - 1] >= w) (*size)--;
}

void* watermark_decode_improved(GRAPH* graph, uint8_t* data, unsigned long* num_bits) {
    unsigned long data_num_bits = *num_bits;
    unsigned long num_bytes = data_num_bits / 8 + !!(data_num_bits % 8);
    unsigned long n = data_num_bits - get_first_positive_bit_index(data, num_bytes);
    unsigned long N = graph->num_nodes;

    // 1-based edge arrays of G'
    uint8_t* fwd = calloc(N + 3, 1);           // fwd[u]: [u -> u+2]
    uint8_t* path = calloc(N + 3, 1);          // path[u]: [u -> u+1]
    uint8_t* in_S = calloc(N + 3, 1);          // inferred mute vertices
    unsigned long* back = calloc(N + 3, sizeof(unsigned long)); // back[u] = w for [w <- u], 0 if none
    long num_fwd = 0;
    for(unsigned long u = 1; u <= N; u++) {
        for(CONNECTION* c = graph->nodes[u-1]->out; c; c = c->next) {
            unsigned long w = c->node->graph_idx + 1;
            if(w == u + 1) path[u] = 1;
            else if(w == u + 2) { fwd[u] = 1; num_fwd++; }
            else if(w < u) back[u] = w;
        }
    }
    // number of forward edges expected from |V(G)| and n, minus those present in G'
    long missing = (long)N - 2 - (long)n - num_fwd;

    unsigned long* list[2]; unsigned long list_n[2] = {0, 0};
    list[0] = malloc((N + 1) * sizeof(unsigned long));
    list[1] = malloc((N + 1) * sizeof(unsigned long));
    list[1][list_n[1]++] = 1; // vertex 1 (odd)

    uint8_t* bits = calloc(n + 1, 1);
    unsigned long cnt = 0;
    bits[cnt++] = 1;

#define WM_HAS_OUT(u) ((u) <= N && (fwd[(u)] || back[(u)]))
    for(unsigned long v = 2; N >= 2 && v <= N - 2; v++) {
        if(cnt >= n) break;
        uint8_t mute = (v >= 3 && fwd[v-2]) || in_S[v];
        if(!mute) {
            uint8_t b;
            if(fwd[v]) {                                          // R1
                b = 1;
            } else if(back[v]) {                                  // R2
                b = (v - back[v]) & 1;
            } else if(!path[v]) {                                 // R3
                b = 1;
            } else if(!path[v+1]) {                               // R4
                b = 1; in_S[v+2] = 1; missing--;
            } else {
                uint8_t no_odd_admissible;
                if(v == 2) no_odd_admissible = 1;                 // the encoder may choose [2 -> 4] freely
                else if(back[v-1]) no_odd_admissible = 1;         // condition (ii)
                else if(fwd[v-1]) no_odd_admissible = 0;          // condition (i): [v-1 <- v] is admissible
                else no_odd_admissible = list_n[(v + 1) & 1] == 0; // odd span: opposite parity
                if(missing > 0 && !WM_HAS_OUT(v) && !WM_HAS_OUT(v+1) && !WM_HAS_OUT(v+2) && no_odd_admissible) {
                    b = 1; in_S[v+2] = 1; missing--;              // R5
                } else {
                    b = 0;
                }
            }
            bits[cnt++] = b;
        }
        // update the candidate lists with vertex v
        if(back[v]) {
            wm_list_truncate(list[0], &list_n[0], back[v]);
            wm_list_truncate(list[1], &list_n[1], back[v]);
        } else if(!fwd[v-1]) {
            list[v & 1][list_n[v & 1]++] = v;
        }
    }
#undef WM_HAS_OUT

    void* res = get_sequence_from_bit_arr(bits, cnt, &num_bytes);
    *num_bits = cnt;
    free(fwd); free(path); free(in_S); free(back); free(list[0]); free(list[1]); free(bits);
    return res;
}

void* watermark_decode(GRAPH* graph, unsigned long* num_bytes) {

    unsigned long n_bits = graph->num_nodes-2;
    uint8_t bits[n_bits];
    bits[0] = 1;
    unsigned long i = 1;

    for(unsigned long graph_idx = 1; graph_idx < n_bits; graph_idx++, i++) {

        // if it isn't a mute node
        if(!( graph_idx > 1 && graph_get_connection(graph->nodes[graph_idx-2], graph->nodes[graph_idx])) ) {

            // if it has a forward edge
            if( graph_idx < n_bits && graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+2]) ) {
                bits[i]=1;
            } else {
                CONNECTION* backedge = graph_get_backedge(graph->nodes[graph_idx]);
                bits[i] = backedge && (( graph_idx - backedge->node->graph_idx ) & 1);
            }
        } else {
            i--;
        }
    }
    // if the second last node is a forward edge destination, the
    // third last node also needs to be ignored
    uint8_t is_prev_last_forward_destination = !!( n_bits > 2 && graph_get_connection(graph->nodes[n_bits-2], graph->nodes[n_bits]) );
    n_bits = i-is_prev_last_forward_destination;
    // bit sequence may be smaller than expected due to mute nodes
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}

void* watermark2014_rs_decode(GRAPH* graph, unsigned long* num_parity_symbols) {

    unsigned long data_len;
    uint8_t* data = watermark2014_decode(graph, &data_len);
    return remove_rs_code8(data, data_len, num_parity_symbols);
}

void* watermark_rs_decode(GRAPH* graph, unsigned long* num_parity_symbols) {

    unsigned long data_len;
    uint8_t* data = watermark_decode(graph, &data_len);
    return remove_rs_code8(data, data_len, num_parity_symbols);
}

void* watermark_rs_decode_improved8(GRAPH* graph, void* key, unsigned long* num_bytes, unsigned long num_parity_symbols) {

  void* key_with_parity = append_rs_code8(key, num_bytes, num_parity_symbols);
  uint8_t* data = watermark_decode_improved8(graph, key_with_parity, num_bytes);
  void* result = remove_rs_code8(data, *num_bytes, &num_parity_symbols);
  *num_bytes = num_parity_symbols;
  free(key_with_parity);
  return result;
}

void* watermark_rs_decode_improved(GRAPH* graph, void* key, unsigned long* num_data_symbols, unsigned long num_parity_symbols, unsigned long symsize) {
  // 0. initialize variables
  unsigned long num_key_bits_with_parity = *num_data_symbols;
  // show_bits(key, (*num_data_symbols) * symsize);
  // 1. get key with RS code
  void* key_with_parity = append_rs_code(key, &num_key_bits_with_parity, num_parity_symbols, symsize);
  // show_bits(key_with_parity, num_key_bits_with_parity);
  unsigned long num_result_bits_with_parity = num_key_bits_with_parity;
  // 2. decode graph
  uint8_t* result_with_parity = watermark_decode_improved(graph, key_with_parity, &num_result_bits_with_parity);
  free(key_with_parity);
  // 3. remove all left zeros from result (actual data start at first positive index)
  unsigned long num_result_bytes_with_parity = num_result_bits_with_parity / 8 + !!(num_result_bits_with_parity % 8);
  remove_left_zeros(result_with_parity, &num_result_bytes_with_parity);
  // show_bits(result_with_parity, num_result_bits_with_parity);
  // 4. if there are more bytes in the result than it should, return NULL
  // if(num_result_bytes_with_parity > num_key_bytes_with_parity) {
  //   free(result_with_parity);
  //   #ifdef DEBUG
  //     fprintf(stderr, "watermark_rs_decode_improved:ERROR ['result_num_bytes' > 'n_bytes']\n");
  //   #endif
  //   return NULL;
  // }
  // 5. realloc result to the right number of bytes
  // if(num_key_bytes_with_parity != num_result_bytes_with_parity) {
  //   result_with_parity = realloc(result_with_parity, num_key_bytes_with_parity);
  //   memset(result_with_parity + num_result_bytes_with_parity, 0x00, num_key_bytes_with_parity - num_result_bytes_with_parity);
  //   num_result_bytes_with_parity = num_key_bytes_with_parity;
  // }
  // 6. insert the right number of zeros in the left (decoding throws the result the right, filling the left with zeros)
  long diff = (((int)(*num_data_symbols) + num_parity_symbols) * symsize) - (int)num_result_bits_with_parity;
  if(diff < 0) {
    free(result_with_parity);
    #ifdef DEBUG
      fprintf(stderr, "watermark_rs_decode_improved:ERROR ['diff' < 0] (result has more zeros than key)\n");
    #endif
    return NULL;
  } else if(diff) {
    add_left_zeros(&result_with_parity, &num_result_bytes_with_parity, diff);
    num_result_bits_with_parity += diff;
  }
  // 7. remove rs code
  uint8_t* result_without_rs = remove_rs_code(result_with_parity, *num_data_symbols, num_parity_symbols, symsize);
  free(result_with_parity);
  unsigned long data_bits = (*num_data_symbols) * symsize;
  *num_data_symbols = data_bits / 8 + !!(data_bits % 8);
  return result_without_rs;
}

void* _watermark_decode_analysis(GRAPH* graph, unsigned long* num_bytes) {

    // load UTILS_NODE in every node
    for(unsigned long i = 0; i < graph->num_nodes; i++) {
        UTILS_NODE* utils_node = malloc(sizeof(UTILS_NODE));
        node_load_info(graph->nodes[i], utils_node, sizeof(UTILS_NODE));
    }

    unsigned long n_bits = graph->num_nodes-2;
    uint8_t bits[n_bits];
    bits[0] = 1;
    unsigned long i = 1;

    for(unsigned long graph_idx = 1; graph_idx < n_bits; graph_idx++, i++) {

        // if it isn't a mute node
        if(!( graph_idx > 1 && graph_get_connection(graph->nodes[graph_idx-2], graph->nodes[graph_idx])) ) {

            // if it has a forward edge
            if( graph_idx < n_bits && graph_get_connection(graph->nodes[graph_idx], graph->nodes[graph_idx+2]) ) {
                ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = BIT_1;
                bits[i]=1;
            } else {
                CONNECTION* backedge = graph_get_backedge(graph->nodes[graph_idx]);
                bits[i] = backedge && (( graph_idx - backedge->node->graph_idx ) & 1);
                ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = bits[i] == '1' ? BIT_1 : BIT_0;
            }
        } else {
            ((UTILS_NODE*)node_get_info(graph->nodes[graph_idx]))->checker_bit = BIT_MUTE;
            i--;
        }
    }
    // if the second last node is a forward edge destination, the
    // third last node also needs to be ignored
    uint8_t is_prev_last_forward_destination = !!( n_bits > 2 && graph_get_connection(graph->nodes[n_bits-2], graph->nodes[n_bits]) );
    n_bits = i-is_prev_last_forward_destination;
    // bit sequence may be smaller than expected due to mute nodes
    void* data = get_sequence_from_bit_arr(bits, n_bits, num_bytes);
    return data;
}
void* watermark_decode_analysis(GRAPH* graph, unsigned long* num_bytes) {

    uint8_t* data = _watermark_decode_analysis(graph, num_bytes);
    unsigned long starting_idx = get_first_positive_bit_index(data, *num_bytes);
    unsigned long n_bits = (*num_bytes)*8 - starting_idx;
    
    uint8_t* bits = malloc(n_bits);
    for(unsigned long i = 0; i < n_bits; i++) {
        bits[i] = get_bit(data, starting_idx+i);
    }
    free(data);
    *num_bytes = n_bits;
    return bits;
}

void* watermark_rs_decode_analysis(GRAPH* graph, unsigned long* num_parity_symbols) {

    uint8_t* data = watermark_rs_decode(graph, num_parity_symbols);

    unsigned long starting_idx = get_first_positive_bit_index(data, *num_parity_symbols);
    unsigned long n_bits = (*num_parity_symbols)*8 - starting_idx;
    
    uint8_t* bits = malloc(n_bits);
    for(unsigned long i = 0; i < n_bits; i++) {
        bits[i] = get_bit(data, starting_idx+i);
    }
    free(data);
    *num_parity_symbols = n_bits;
    return bits;
}
