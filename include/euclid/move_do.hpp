#pragma once
#include "euclid/move.hpp"
#include "euclid/board.hpp"

namespace euclid {

// Minimal reversible state for undo
struct State {
  Color   us;                // side who moved
  int     prev_ep;
  int     prev_halfmove;
  int     prev_fullmove;
  unsigned prev_castling;

  Piece   moved{Piece::None};      // piece that moved (pre-promo)
  Piece   captured{Piece::None};   // captured piece type (if any)
  Square  captured_sq{-1};         // where the captured piece sat
};

void do_move(Board& b, const Move& m, State& st);
void undo_move(Board& b, const Move& m, const State& st);

} // namespace euclid
