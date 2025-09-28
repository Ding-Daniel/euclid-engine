#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // White bishop on d5; kings placed so White king has 0 moves.
  // Bishop has 13 legal moves on an open board from d5.
  Board b;
  set_from_fen(b, "8/8/8/3B4/8/8/1k6/K7 w - - 0 1");
  assert(perft(b, 1) == 13ULL);

  return 0;
}
