#pragma once
#include <cstdint>
#include "euclid/board.hpp"
#include "euclid/move.hpp"


namespace euclid {


std::uint64_t perft(const Board& b, int depth);


} // namespace euclid