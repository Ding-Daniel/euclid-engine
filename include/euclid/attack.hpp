#pragma once
#include "euclid/board.hpp"

namespace euclid {

// Is square s attacked by side 'by'?
bool square_attacked(const Board& b, Square s, Color by);

// Is 'side' currently in check?
bool in_check(const Board& b, Color side);

} // namespace euclid
