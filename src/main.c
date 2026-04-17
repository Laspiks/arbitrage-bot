#include "bellman_ford.h"
#include "exchange_api.h"
#include "graph.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_cycle(const Graph *g, const int *cycle, int len, int index) {
  if (len < 2)
    return;
  double product = 1.0;
  printf("  [%d] ", index + 1);
  for (int i = 0; i < len - 1; i++) {
    int a = cycle[i];
    int b = cycle[i + 1];
    double r = graph_edge_rate(g, a, b);
    product *= r;
    printf("%s --(%.6g)--> %s", g->currencies[a], r, g->currencies[b]);
    if (i + 1 < len - 1)
      printf("\n       ");
  }
  printf("\n");
  double profit = product - 1.0;
  printf("       compound factor: %.6f  |  profit: %.6f (%.6f%%)\n", product,
         profit, profit * 100.0);
}

int main(int argc, char **argv) {
  const char *default_bases[] = {"GBP", "JPY", "USD", "EUR"};
  const char *const *bases = default_bases;
  int n_bases = (int)(sizeof(default_bases) / sizeof(default_bases[0]));
  double min_gain = ARBITRAGE_MIN_GAIN;
  bool log_enabled = false;

  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "--help") == 0) {
      printf("Usage: %s [--log] [--min-gain <value>] [BASE ...]\n", argv[0]);
      printf("Example: %s --log --min-gain 1e-9 USD CAD EUR JPY PLN\n",
             argv[0]);
      return 0;
    }
    if (strcmp(argv[i], "--log") == 0) {
      log_enabled = true;
      i++;
      continue;
    }
    if (strcmp(argv[i], "--min-gain") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value after --min-gain\n");
        return 1;
      }
      errno = 0;
      char *end = NULL;
      double v = strtod(argv[i + 1], &end);
      if (errno != 0 || end == argv[i + 1] || *end != '\0' || v < 0.0) {
        fprintf(stderr, "Invalid --min-gain value: %s\n", argv[i + 1]);
        return 1;
      }
      min_gain = v;
      i += 2;
      continue;
    }
    break;
  }

  if (i < argc) {
    bases = (const char *const *)&argv[i];
    n_bases = argc - i;
  }

  Graph g;
  graph_init(&g);

  if (exchange_rates_fetch_into_graph(&g, bases, n_bases) != 0) {
    fprintf(stderr, "Failed to download or parse exchange rates.\n");
    graph_free(&g);
    return 1;
  }

  if (g.n_curr == 0 || g.n_edges == 0) {
    fprintf(stderr, "No graph data after fetch.\n");
    graph_free(&g);
    return 1;
  }

  ArbitrageCycle *cycles = NULL;
  int n_cycles = 0;

  if (bellman_ford_find_arbitrage_cycles(&g, &cycles, &n_cycles,
                                         min_gain, log_enabled) != 0) {
    fprintf(stderr, "Out of memory while searching for cycles.\n");
    graph_free(&g);
    return 1;
  }

  if (n_cycles > 0) {
    printf("Arbitrage-like cycles (compound factor > 1 + %.1g):\n\n",
           min_gain);
    for (int i = 0; i < n_cycles; i++) {
      print_cycle(&g, cycles[i].vertices, cycles[i].len, i);
    }
    arbitrage_cycles_free(cycles, n_cycles);
  } else {
    printf("No arbitrage detected for the fetched bases.\n");
  }

  graph_free(&g);
  return 0;
}
