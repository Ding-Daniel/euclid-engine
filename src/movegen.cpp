#include "euclid/movegen.hpp"
#include <cstdlib> // std::abs

namespace euclid {

static inline bool on_board(int s) { return s >= 0 && s < 64; }

void generate_pseudo_legal(const Board& b, MoveList& out) {
  out.sz = 0;

  const Color us = b.side_to_move();

  // Knight-only for Step 4 (king & others come next)
  static constexpr int KN_OFF[8] = {+17, +15, +10, +6, -6, -10, -15, -17};

  for (int s = 0; s < 64; ++s) {
    Color pcColor;
    Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Knight || pcColor != us) continue;

    const int f0 = file_of(s);
    const int r0 = rank_of(s);

    for (int d : KN_OFF) {
      const int t = s + d;
      if (!on_board(t)) continue;

      const int f1 = file_of(t);
      const int r1 = rank_of(t);

      // Validate L-shape (guards against wrap-around)
      const int df = std::abs(f1 - f0);
      const int dr = std::abs(r1 - r0);
      if (!((df == 1 && dr == 2) || (df == 2 && dr == 1))) continue;

      Color occColor;
      Piece occ = b.piece_at(t, &occColor);
      if (occ == Piece::None) {
        out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
      } else if (occColor != us) {
        out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
      }
      // else own piece: skip
    }
  }
}

} // namespace euclid
