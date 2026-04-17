#ifndef BELLMAN_FORD_H
#define BELLMAN_FORD_H

#include "graph.h"
#include <stdbool.h>

#ifndef ARBITRAGE_MIN_GAIN
#define ARBITRAGE_MIN_GAIN 1e-7
#endif

typedef struct {
  int *vertices;
  int len;
} ArbitrageCycle;

void arbitrage_cycles_free(ArbitrageCycle *cycles, int n);

/*
 * Runs Bellman–Ford, then checks every edge that still relaxes after V−1 passes.
 * keeps only those with compound factor > 1 + min_gain
 * Duplicates (same directed edges, up to rotation) are removed. 
 * Returns number of cycles stored in *out (0 if none).
 */
int bellman_ford_find_arbitrage_cycles(const Graph *g, ArbitrageCycle **out,
                                       int *n_out, double min_gain,
                                       bool log_enabled);

#endif
