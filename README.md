Currency Arbitrage Detection using Bellman-Ford
Overview

This project detects currency arbitrage opportunities using the Bellman-Ford algorithm.
It models currencies as a weighted directed graph and identifies profitable exchange cycles.

Idea

Currency exchange rates are represented as a graph:

Nodes → currencies
Edges → exchange rates

To make the problem suitable for graph algorithms, rates are transformed using:

w = -ln(rate)

This converts multiplication of rates into addition of weights, allowing detection of negative cycles.

Algorithm

The Bellman-Ford algorithm is used to:

Compute shortest paths
Detect negative weight cycles

A negative cycle corresponds to a profitable arbitrage opportunity.

Complexity:

O(V · E)
Features
Fetches real exchange rates from public API
Builds a currency graph dynamically
Detects arbitrage cycles
Filters noise using precision threshold (min_gain)
Removes duplicate cycles
Outputs detailed arbitrage paths
Project Structure
arbitrage/
├── src/              # Main program
├── api/              # Exchange rate API handler
├── graph/            # Graph + Bellman-Ford implementation
├── include/          # Headers
├── third_party/      # JSON parser (cJSON)
└── Makefile
Output Example
USD → EUR → GBP → USD
Gain: 1.000142 (0.0142%)
Key Insight

A profitable arbitrage cycle exists if:

product of rates > 1

which corresponds to:

negative cycle in transformed graph
Future Improvements
Real-time trading integration
Transaction fees support
Performance optimization for large graphs
Web dashboard for monitoring opportunities
Author

Project developed as a study implementation of graph algorithms in real-world financial modeling.
