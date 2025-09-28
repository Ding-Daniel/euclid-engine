#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // White bishop on d5, kings placed so White king has 0 legal moves
  // (adjacent-to-black constraint blocks Ka1 moves). Expect 12 bishop moves.
  // FEN ranks: 8/7/6/5/4/3/2/1  (top to bottom)
  //             8/8/8/3B4/8/8/1k6/K7 w - - 0 1
  Board b;
  set_from_fen(b, "8/8/8/3B4/8/8/1k6/K7 w - - 0 1");
  assert(perft(b, 1) == 12ULL);

  return 0;
}
