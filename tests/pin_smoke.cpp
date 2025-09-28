#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;
  // e-file pin: white Ke1, Pe2; black rook e8.
  // Pseudo-legal would include e2e3/e2e4, but legality filter removes them.
  // Legal: Ke1 -> d1, f1, d2, f2  (4 moves)
  Board b;
  set_from_fen(b, "4r3/8/8/8/8/8/4P3/4K3 w - - 0 1");
  assert(perft(b, 1) == 4ULL);
  return 0;
}
