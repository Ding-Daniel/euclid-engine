#pragma once
#include <string>
#include "euclid/board.hpp"
#include "euclid/movegen.hpp"  // <-- brings in Move / MoveList / MoveFlag

namespace euclid {

// Convert a Move to UCI string like "e2e4" or "a7a8q"
std::string move_to_uci(const Move& m);

// Parse a UCI string into a pseudo-legal move for the given board.
// Throws std::invalid_argument if not found among pseudo-legal moves.
Move uci_to_move(const Board& b, const std::string& uci);

} // namespace euclid
