#pragma once
#include "euclid/board.hpp"
#include "euclid/movelist.hpp"


namespace euclid {


// For Step 3 we provide a stub generator that yields 0 moves.
// We'll flesh this out piece-by-piece in Step 4.
void generate_pseudo_legal(const Board& b, MoveList& out);


} // namespace euclid