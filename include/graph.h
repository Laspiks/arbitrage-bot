#ifndef GRAPH_H
#define GRAPH_H

#include <stddef.h>

typedef struct {
  int from;
  int to;
  double rate;
  double weight;
} Edge;

typedef struct {
  char **currencies;
  int n_curr;
  int cap_curr;
  Edge *edges;
  size_t n_edges;
  size_t cap_edges;
} Graph;

void graph_init(Graph *g);
void graph_free(Graph *g);

int graph_currency_index(Graph *g, const char *code);

void graph_add_edge(Graph *g, const char *from, const char *to, double rate);

double graph_edge_rate(const Graph *g, int from, int to);

#endif
