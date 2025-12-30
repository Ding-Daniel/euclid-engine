#include <atomic>
#include <cassert>

#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/search.hpp"

using namespace euclid;

int main() {
  Board b;

  // EP + castling rights + nontrivial stm, clocks.
  // After 1.e4 (black to move, ep square e3).
  set_from_fen(b, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

  const U64   h0   = b.hash();
  const Color stm0 = b.side_to_move();
  const Square ep0 = b.ep_square();
  const auto  c0   = b.castling().rights;
  const int   hm0  = b.halfmove_clock();
  const int   fm0  = b.fullmove_number();

  // Depth high enough that null-move paths are likely exercised internally.
  (void)search(b, 6);

  assert(b.hash() == h0);
  assert(b.side_to_move() == stm0);
  assert(b.ep_square() == ep0);
  assert(b.castling().rights == c0);
  assert(b.halfmove_clock() == hm0);
  assert(b.fullmove_number() == fm0);

  return 0;
}
