#pragma once
#include "euclid/board.hpp"
#include "euclid/movelist.hpp"

namespace euclid {

// Generate pseudo-legal moves for the side to move.
// Legality (king not left in check) is enforced by callers via do_move + in_check.
void generate_pseudo_legal(const Board& b, MoveList& out);

} // namespace euclid
