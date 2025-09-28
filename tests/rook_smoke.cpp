#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // Case 1: open board rook on d4; kings arranged so White king has 0 moves.
  // Expect 14 rook moves from d4 (7 rank + 7 file).
  {
    Board b;
    set_from_fen(b, "8/8/8/8/3R4/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 14ULL);
  }

  // Case 2: blocker ahead on d6 (black knight). Upward ray: d5 (quiet) + d6 (capture) then stop.
  // Expect 12 total.
  {
    Board b;
    set_from_fen(b, "8/8/3n4/8/3R4/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 12ULL);
  }

  return 0;
}
