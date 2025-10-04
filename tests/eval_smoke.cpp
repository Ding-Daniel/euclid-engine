#include <cassert>
#include "euclid/fen.hpp"
#include "euclid/eval.hpp"
#include "euclid/board.hpp"

using namespace euclid;

int main() {
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);
    assert(evaluate(b) == 0);
  }
  {
    // White up a queen (both kings present for sanity)
    Board b;
    set_from_fen(b, "7k/8/8/8/8/8/8/6KQ w - - 0 1");
    assert(evaluate(b) == 900);
  }
  {
    // Black up a rook (both kings present)
    Board b;
    set_from_fen(b, "r6k/8/8/8/8/8/8/6K1 w - - 0 1");
    assert(evaluate(b) == -500);
  }
  {
    // White up a pawn
    Board b;
    set_from_fen(b, "7k/8/8/8/8/8/7P/6K1 w - - 0 1");
    assert(evaluate(b) == 100);
  }
  return 0;
}
