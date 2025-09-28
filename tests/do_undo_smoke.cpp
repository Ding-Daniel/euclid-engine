#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"

int main() {
  using namespace euclid;
  Board b;
  set_from_fen(b, STARTPOS_FEN);
  const auto h0  = b.hash();
  const int ep0  = b.ep_square();
  const int hm0  = b.halfmove_clock();
  const int fm0  = b.fullmove_number();
  const Color s0 = b.side_to_move();

  MoveList ml; generate_pseudo_legal(b, ml);
  assert(ml.size() > 0);

  // Try the first few moves and ensure exact restoration
  const std::size_t N = ml.size() < 12 ? ml.size() : 12;
  for (std::size_t i = 0; i < N; ++i) {
    State st{};
    do_move(b, ml.data[i], st);
    undo_move(b, ml.data[i], st);
    assert(b.hash() == h0);
    assert(b.ep_square() == ep0);
    assert(b.halfmove_clock() == hm0);
    assert(b.fullmove_number() == fm0);
    assert(b.side_to_move() == s0);
  }
  return 0;
}
