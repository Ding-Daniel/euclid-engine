#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // Kings far apart: e4 (white) vs a8 (black)
  Board b1;
  set_from_fen(b1, "k7/8/8/8/4K3/8/8/8 w - - 0 1");
  assert(perft(b1, 1) == 8ULL);
  assert(perft(b1, 2) == 24ULL);

  // Kings on b2 (white) and b3 (black): white king cannot move adjacent to black king.
  // Legal white king moves are {a1, b1, c1} -> 3.
  Board b2;
  set_from_fen(b2, "8/8/8/8/8/1k6/1K6/8 w - - 0 1");
  assert(perft(b2, 1) == 3ULL);

  return 0;
}
