#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;

  // Startpos: with knight-only movegen, depth 1 -> 4 moves (two knights, two options each)
  Board b;
  set_from_fen(b, STARTPOS_FEN);

  assert(perft(b, 0) == 1ULL);
  assert(perft(b, 1) == 4ULL);
  assert(perft(b, 2) == 16ULL);

  return 0;
}
