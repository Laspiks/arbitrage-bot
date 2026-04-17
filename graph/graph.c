#define _POSIX_C_SOURCE 200809L

#include "graph.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int grow_currencies(Graph *g) {
  int ncap = g->cap_curr ? g->cap_curr * 2 : 16;
  char **n = realloc(g->currencies, (size_t)ncap * sizeof(*n));
  if (!n)
    return -1;
  g->currencies = n;
  g->cap_curr = ncap;
  return 0;
}

static int grow_edges(Graph *g) {
  size_t ncap = g->cap_edges ? g->cap_edges * 2 : 32;
  Edge *e = realloc(g->edges, ncap * sizeof(*e));
  if (!e)
    return -1;
  g->edges = e;
  g->cap_edges = ncap;
  return 0;
}

void graph_init(Graph *g) {
  g->currencies = NULL;
  g->n_curr = 0;
  g->cap_curr = 0;
  g->edges = NULL;
  g->n_edges = 0;
  g->cap_edges = 0;
}

void graph_free(Graph *g) {
  for (int i = 0; i < g->n_curr; i++)
    free(g->currencies[i]);
  free(g->currencies);
  free(g->edges);
  graph_init(g);
}

int graph_currency_index(Graph *g, const char *code) {
  for (int i = 0; i < g->n_curr; i++) {
    if (strcmp(g->currencies[i], code) == 0)
      return i;
  }
  if (g->n_curr >= g->cap_curr && grow_currencies(g) != 0)
    return -1;
  g->currencies[g->n_curr] = strdup(code);
  if (!g->currencies[g->n_curr])
    return -1;
  return g->n_curr++;
}

static void upsert_edge(Graph *g, int from, int to, double rate) {
  if (from < 0 || to < 0 || rate <= 0.0)
    return;
  double w = -log(rate);
  for (size_t i = 0; i < g->n_edges; i++) {
    if (g->edges[i].from == from && g->edges[i].to == to) {
      if (w < g->edges[i].weight) {
        g->edges[i].rate = rate;
        g->edges[i].weight = w;
      }
      return;
    }
  }
  if (g->n_edges >= g->cap_edges && grow_edges(g) != 0)
    return;
  g->edges[g->n_edges].from = from;
  g->edges[g->n_edges].to = to;
  g->edges[g->n_edges].rate = rate;
  g->edges[g->n_edges].weight = w;
  g->n_edges++;
}

void graph_add_edge(Graph *g, const char *from, const char *to, double rate) {
  int fi = graph_currency_index(g, from);
  int ti = graph_currency_index(g, to);
  if (fi < 0 || ti < 0)
    return;
  upsert_edge(g, fi, ti, rate);
}

double graph_edge_rate(const Graph *g, int from, int to) {
  for (size_t i = 0; i < g->n_edges; i++) {
    if (g->edges[i].from == from && g->edges[i].to == to)
      return g->edges[i].rate;
  }
  return 1.0;
}
