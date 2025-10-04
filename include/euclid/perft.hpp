#pragma once
#include <cstdint>
#include <vector>
#include "euclid/types.hpp"
#include "euclid/board.hpp"
#include "euclid/movegen.hpp"   // <-- add this so Move is a complete type

namespace euclid {

std::uint64_t perft(const Board& b, int depth);

// Per-move breakdown at root
void perft_divide(const Board& b, int depth,
                  std::vector<std::pair<Move, std::uint64_t>>& out);

} // namespace euclid
