#pragma once
#include "euclid/board.hpp"

namespace euclid {

// Returns a side-to-move agnostic material score: positive = White better.
int evaluate(const Board& b);

} // namespace euclid
