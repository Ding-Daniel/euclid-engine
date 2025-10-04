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

// Depth-limited negamax with alpha-beta. No TT/ID yet.
SearchResult search(const Board& root, int depth);

} // namespace euclid
