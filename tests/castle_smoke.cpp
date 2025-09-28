#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"

using namespace euclid;

static int count_legal_castles(Board& b) {
  MoveList ml; generate_pseudo_legal(b, ml);
  int n = 0; const Color us = b.side_to_move();
  for (const auto& m : ml) {
    if (m.flags != MoveFlag::Castle) continue;
    State st{}; do_move(b, m, st);
    if (!in_check(b, us)) ++n;
    undo_move(b, m, st);
  }
  return n;
}

int main() {
  // Both sides have both castles available
  {
    Board b; set_from_fen(b, "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    assert(count_legal_castles(b) == 2); // white can O-O and O-O-O
  }
  // Kingside blocked by attack on f1 (black bishop on a6): only O-O-O remains
  {
    Board b; set_from_fen(b, "r3k2r/8/b7/8/8/8/8/R3K2R w KQkq - 0 1");
    assert(count_legal_castles(b) == 1);
  }
  // No rook at h1: K-right must be ignored even if K present in FEN rights
  {
    Board b; set_from_fen(b, "r3k2r/8/8/8/8/8/8/R3K3 w KQkq - 0 1"); // no h1 rook
    int n = count_legal_castles(b);
    assert(n == 1); // only O-O-O available
  }
  return 0;
}
