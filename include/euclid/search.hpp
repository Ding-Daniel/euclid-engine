#pragma once
#include <cstdint>
#include <vector>
#include "euclid/board.hpp"
#include "euclid/movegen.hpp"

namespace euclid {

struct SearchResult {
  Move best{};
  int score{0};            // centipawns, POV = side to move
  std::uint64_t nodes{0};
  int depth{0};
  std::vector<Move> pv;    // principal variation, best line
};
// Search limits for UCI/time mgmt
struct SearchLimits {
  int depth = 0;               // 0 = use time
  int movetime_ms = 0;         // exact slice
  int wtime_ms = 0, btime_ms = 0;
  int winc_ms  = 0, binc_ms  = 0;
  int movestogo = 0;           // optional for classical
  std::atomic<bool>* stop = nullptr; // optional external stop
};

SearchResult search(const Board& root, int maxDepth);                 // existing
SearchResult search(const Board& root, const SearchLimits& limits);   // new

// Depth-limited negamax with alpha-beta. No TT/ID yet.
SearchResult search(const Board& root, int depth);

} // namespace euclid
