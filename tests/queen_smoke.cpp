#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // Case 1: open-board queen on d5; kings placed so king has 0 moves.
  // Bishop(13) + Rook(14) = 27.
  {
    Board b;
    set_from_fen(b, "8/8/8/3Q4/8/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 27ULL);
  }

  // Case 2: blocker on d7 (black knight): up-ray becomes d6 (quiet) + d7 (capture), then stop.
  // Total 26.
  {
    Board b;
    set_from_fen(b, "8/3n4/8/3Q4/8/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 26ULL);
  }

  return 0;
}
