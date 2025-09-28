#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // 1) Double + single from start square (d2), both empty: 2 moves
  {
    Board b;
    set_from_fen(b, "8/8/8/8/8/8/1k1P4/K7 w - - 0 1");
    assert(perft(b, 1) == 2ULL);
  }

  // 2) Captures only from d4: c5 and e5, with d5 blocked
  {
    Board b;
    set_from_fen(b, "8/8/8/2nbn3/3P4/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 2ULL);
  }

  // 3) Promotions (forward only) from a7 to a8: 4 promotions
  {
    Board b;
    set_from_fen(b, "8/P7/8/8/8/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 4ULL);
  }

  // 4) Promotions with capture on b8 present: forward (4) + capture (4) = 8
  {
    Board b;
    set_from_fen(b, "1n6/P7/8/8/8/8/1k6/K7 w - - 0 1");
    assert(perft(b, 1) == 8ULL);
  }

  // 5) En passant available: white pawn d5, black pawn e5, ep=e6 -> {d6, dxe6 ep} = 2
  {
    Board b;
    set_from_fen(b, "8/8/8/3Pp3/8/8/1k6/K7 w - e6 0 1");
    assert(perft(b, 1) == 2ULL);
  }

  return 0;
}
