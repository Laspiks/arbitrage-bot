CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude -Ithird_party/cjson
LDFLAGS = -lm

PKG_CF := $(shell pkg-config --cflags libcurl 2>/dev/null)
PKG_LF := $(shell pkg-config --libs libcurl 2>/dev/null)
ifneq ($(PKG_CF),)
CFLAGS += $(PKG_CF)
LDFLAGS := $(PKG_LF) $(LDFLAGS)
else
LDFLAGS := -lcurl $(LDFLAGS)
endif

OBJS = src/main.o graph/graph.o graph/bellman_ford.o api/exchange_api.o \
       third_party/cjson/cJSON.o

arbitrage: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

src/main.o: src/main.c include/graph.h include/bellman_ford.h include/exchange_api.h
graph/graph.o: graph/graph.c include/graph.h
graph/bellman_ford.o: graph/bellman_ford.c include/graph.h include/bellman_ford.h
api/exchange_api.o: api/exchange_api.c include/exchange_api.h include/graph.h third_party/cjson/cJSON.h
third_party/cjson/cJSON.o: third_party/cjson/cJSON.c third_party/cjson/cJSON.h

clean:
	rm -f $(OBJS) arbitrage

.PHONY: clean
