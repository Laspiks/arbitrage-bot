#include "bellman_ford.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double cycle_compound_factor(const Graph *g, const int *cycle, int len) {
  if (len < 2)
    return 1.0;
  double p = 1.0;
  for (int i = 0; i < len - 1; i++)
    p *= graph_edge_rate(g, cycle[i], cycle[i + 1]);
  return p;
}

static int cmp_pair(const void *a, const void *b) {
  const int *pa = a;
  const int *pb = b;
  if (pa[0] != pb[0])
    return pa[0] - pb[0];
  return pa[1] - pb[1];
}

static unsigned char *cycle_signature(const int *cycle, int len, int *sig_len_out) {
  int n_edges = len - 1;
  int *pairs = malloc((size_t)n_edges * 2 * sizeof(int));
  if (!pairs)
    return NULL;
  for (int i = 0; i < n_edges; i++) {
    pairs[2 * i] = cycle[i];
    pairs[2 * i + 1] = cycle[i + 1];
  }
  qsort(pairs, (size_t)n_edges, 2 * sizeof(int), cmp_pair);
  *sig_len_out = n_edges * 2 * (int)sizeof(int);
  return (unsigned char *)pairs;
}

static bool signature_seen(unsigned char **sigs, int *sig_lens, int n_sigs,
                           const unsigned char *sig, int sig_len) {
  for (int i = 0; i < n_sigs; i++) {
    if (sig_lens[i] == sig_len &&
        memcmp(sigs[i], sig, (size_t)sig_len) == 0)
      return true;
  }
  return false;
}

static bool extract_forward_cycle(const int *pred, int n, int relax_v, int **out_fwd,
                                  int *out_len) {
  *out_fwd = NULL;
  *out_len = 0;

  int y = relax_v;
  for (int i = 0; i < n; i++) {
    if (y < 0 || y >= n)
      return false;
    y = pred[y];
  }

  int path[256];
  int pc = 0;
  int cur = y;
  path[pc++] = cur;
  cur = pred[cur];
  while (cur != y && pc < (int)(sizeof(path) / sizeof(path[0])) - 1) {
    if (cur < 0 || cur >= n)
      return false;
    path[pc++] = cur;
    cur = pred[cur];
  }

  if (cur != y)
    return false;

  int *forward = malloc((size_t)(pc + 2) * sizeof(int));
  if (!forward)
    return false;
  int fc = 0;
  forward[fc++] = path[0];
  for (int i = pc - 1; i >= 1; i--)
    forward[fc++] = path[i];
  forward[fc++] = path[0];

  *out_fwd = forward;
  *out_len = fc;
  return true;
}

void arbitrage_cycles_free(ArbitrageCycle *cycles, int n) {
  if (!cycles)
    return;
  for (int i = 0; i < n; i++)
    free(cycles[i].vertices);
  free(cycles);
}

int bellman_ford_find_arbitrage_cycles(const Graph *g, ArbitrageCycle **out, int *n_out,
                                       double min_gain, bool log_enabled) {
  *out = NULL;
  *n_out = 0;

  int n = g->n_curr;
  if (n <= 0 || g->n_edges == 0)
    return 0;
  if (log_enabled) {
    fprintf(stderr, "[bf] start: vertices=%d edges=%zu min_gain=%.12g\n", n,
            g->n_edges, min_gain);
  }

  double *dist = calloc((size_t)n, sizeof(double));
  int *pred = malloc((size_t)n * sizeof(int));
  if (!dist || !pred) {
    free(dist);
    free(pred);
    return -1;
  }

  for (int i = 0; i < n; i++) {
    dist[i] = 0.0;
    pred[i] = -1;
  }

  for (int i = 0; i < n - 1; i++) {
    int changed = 0;
    for (size_t e = 0; e < g->n_edges; e++) {
      int u = g->edges[e].from;
      int v = g->edges[e].to;
      double w = g->edges[e].weight;
      if (dist[u] + w < dist[v]) {
        dist[v] = dist[u] + w;
        pred[v] = u;
        changed++;
      }
    }
    if (log_enabled) {
      fprintf(stderr, "[bf] iter %d/%d relaxations=%d\n", i + 1, n - 1,
              changed);
    }
    if (changed == 0)
      break;
  }

  int cap = 8;
  ArbitrageCycle *list = calloc((size_t)cap, sizeof(ArbitrageCycle));
  unsigned char **sigs = NULL;
  int *sig_lens = NULL;
  int n_sig = 0;

  if (!list) {
    free(dist);
    free(pred);
    return -1;
  }

  int *pred_work = malloc((size_t)n * sizeof(int));
  if (!pred_work) {
    free(list);
    free(dist);
    free(pred);
    return -1;
  }

  for (size_t e = 0; e < g->n_edges; e++) {
    int u = g->edges[e].from;
    int v = g->edges[e].to;
    double w = g->edges[e].weight;
    if (!(dist[u] + w < dist[v]))
      continue;
    if (log_enabled) {
      fprintf(stderr,
              "[bf] candidate edge %s -> %s improves: %.12g + %.12g < %.12g\n",
              g->currencies[u], g->currencies[v], dist[u], w, dist[v]);
    }

    memcpy(pred_work, pred, (size_t)n * sizeof(int));
    pred_work[v] = u;

    int *forward = NULL;
    int flen = 0;
    if (!extract_forward_cycle(pred_work, n, v, &forward, &flen))
      continue;

    double gain = cycle_compound_factor(g, forward, flen);
    if (!(gain > 1.0 + min_gain)) {
      if (log_enabled) {
        fprintf(stderr, "[bf] reject cycle: gain=%.12g threshold=%.12g\n", gain,
                1.0 + min_gain);
      }
      free(forward);
      continue;
    }

    int slen = 0;
    unsigned char *sig = cycle_signature(forward, flen, &slen);
    if (!sig) {
      free(forward);
      continue;
    }

    if (signature_seen(sigs, sig_lens, n_sig, sig, slen)) {
      if (log_enabled)
        fprintf(stderr, "[bf] skip duplicate cycle\n");
      free(sig);
      free(forward);
      continue;
    }
    if (log_enabled) {
      fprintf(stderr, "[bf] accept cycle #%d: len=%d gain=%.12g\n", *n_out + 1,
              flen, gain);
    }

    unsigned char **ns = realloc(sigs, (size_t)(n_sig + 1) * sizeof(*sigs));
    int *nl = realloc(sig_lens, (size_t)(n_sig + 1) * sizeof(*nl));
    if (!ns || !nl) {
      free(sig);
      free(forward);
      free(ns ? ns : sigs);
      free(nl ? nl : sig_lens);
      for (int i = 0; i < *n_out; i++)
        free(list[i].vertices);
      free(list);
      free(pred_work);
      free(dist);
      free(pred);
      return -1;
    }
    sigs = ns;
    sig_lens = nl;
    sigs[n_sig] = sig;
    sig_lens[n_sig] = slen;
    n_sig++;

    if (*n_out >= cap) {
      int ncap = cap * 2;
      ArbitrageCycle *nlst = realloc(list, (size_t)ncap * sizeof(ArbitrageCycle));
      if (!nlst) {
        free(forward);
        for (int i = 0; i < n_sig; i++)
          free(sigs[i]);
        free(sigs);
        free(sig_lens);
        for (int i = 0; i < *n_out; i++)
          free(list[i].vertices);
        free(list);
        free(pred_work);
        free(dist);
        free(pred);
        return -1;
      }
      list = nlst;
      cap = ncap;
    }

    list[*n_out].vertices = forward;
    list[*n_out].len = flen;
    (*n_out)++;
  }

  for (int i = 0; i < n_sig; i++)
    free(sigs[i]);
  free(sigs);
  free(sig_lens);

  free(pred_work);
  free(dist);
  free(pred);

  if (*n_out == 0) {
    free(list);
    *out = NULL;
  } else {
    *out = list;
  }
  if (log_enabled)
    fprintf(stderr, "[bf] done: cycles=%d\n", *n_out);
  return 0;
}
