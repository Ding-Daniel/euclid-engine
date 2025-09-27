#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // Kings far apart: e4 (white) vs a8 (black)
  // Expect: depth1 = 8 king moves from e4, depth2 = 8 * 3 = 24 replies from black king at a8.
  Board b1;
  set_from_fen(b1, "k7/8/8/8/4K3/8/8/8 w - - 0 1");
  assert(perft(b1, 1) == 8ULL);
  assert(perft(b1, 2) == 24ULL);

  // Kings vertically adjacent if white moves b2->b3; we must forbid adjacency and king-captures.
  // White: Kb2, Black: Kb3. Legal white king moves = {a1,a2,b1,c1,c2} => 5.
  Board b2;
  set_from_fen(b2, "8/8/8/8/8/1k6/1K6/8 w - - 0 1");
  assert(perft(b2, 1) == 5ULL);

  return 0;
}
