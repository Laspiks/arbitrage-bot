#ifndef EXCHANGE_API_H
#define EXCHANGE_API_H

#include "graph.h"

int exchange_rates_fetch_into_graph(Graph *g, const char *const *bases,
                                    int n_bases);

#endif
