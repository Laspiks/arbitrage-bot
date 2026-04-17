#include "exchange_api.h"

#include "cJSON.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t write_memory_cb(void *contents, size_t size, size_t nmemb,
                              void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr)
    return 0;

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static char *http_get(const char *url) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return NULL;

  struct MemoryStruct chunk = {0};
  chunk.memory = malloc(1);
  if (!chunk.memory) {
    curl_easy_cleanup(curl);
    return NULL;
  }
  chunk.memory[0] = 0;
  chunk.size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "arbitrage-c/1.0");

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    free(chunk.memory);
    return NULL;
  }
  return chunk.memory;
}

static int parse_frankfurter_json_into_graph(Graph *g, const char *json) {
  cJSON *root = cJSON_Parse(json);
  if (!root)
    return -1;

  cJSON *base = cJSON_GetObjectItemCaseSensitive(root, "base");
  cJSON *rates = cJSON_GetObjectItemCaseSensitive(root, "rates");
  if (!cJSON_IsString(base) || !cJSON_IsObject(rates)) {
    cJSON_Delete(root);
    return -1;
  }

  const char *base_code = base->valuestring;
  cJSON *rate = NULL;
  cJSON_ArrayForEach(rate, rates) {
    if (!cJSON_IsNumber(rate))
      continue;
    const char *quote = rate->string;
    if (!quote)
      continue;
    double r = rate->valuedouble;
    if (r <= 0.0)
      continue;
    graph_add_edge(g, base_code, quote, r);
    graph_add_edge(g, quote, base_code, 1.0 / r);
  }

  cJSON_Delete(root);
  return 0;
}

int exchange_rates_fetch_into_graph(Graph *g, const char *const *bases,
                                    int n_bases) {
  if (!g || !bases || n_bases <= 0)
    return -1;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  int ok = 0;
  for (int i = 0; i < n_bases; i++) {
    char url[256];
    if (snprintf(url, sizeof(url),
                 "https://api.frankfurter.app/latest?from=%s", bases[i]) >=
        (int)sizeof(url)) {
      fprintf(stderr, "[API] skip base=%s (url too long)\n", bases[i]);
      continue;
    }
    fprintf(stderr, "[API] GET base=%s...   ", bases[i]);
    char *body = http_get(url);
    if (!body) {
      fprintf(stderr, "FAIL\n");
      continue;
    }
    if (parse_frankfurter_json_into_graph(g, body) == 0) {
      ok++;
      fprintf(stderr, "OK\n");
    } else {
      fprintf(stderr, "FAIL (parse error)\n");
    }
    free(body);
  }

  curl_global_cleanup();
  return ok > 0 ? 0 : -1;
}
