// include/euclid/uci.hpp
#pragma once
#include <string>
#include <iosfwd>

namespace euclid {

// Forward declarations (avoid needing full headers here)
struct Move;
class Board;

// Convert a Move to a UCI string like "e2e4" or "a7a8q"
std::string move_to_uci(const Move& m);

// Parse a UCI string into a legal move for the given board.
// Throws std::invalid_argument if not found among pseudo-legal moves.
Move uci_to_move(const Board& b, const std::string& uci);

// Minimal UCI driver loop
void uci_loop(std::istream& in, std::ostream& out);
void uci_loop(); // convenience overload

} // namespace euclid
