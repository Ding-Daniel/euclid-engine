#include "euclid/eval.hpp"
#include "euclid/types.hpp"

namespace euclid {

// Conventional material values (centipawns)
static constexpr int VAL_NONE  = 0;
static constexpr int VAL_PAWN  = 100;
static constexpr int VAL_KNIGHT= 320;
static constexpr int VAL_BISHOP= 330;
static constexpr int VAL_ROOK  = 500;
static constexpr int VAL_QUEEN = 900;
static constexpr int VAL_KING  = 0; // King value omitted in eval; checkmate handled in search

static inline int piece_value(Piece p) {
  switch (p) {
    case Piece::Pawn:   return VAL_PAWN;
    case Piece::Knight: return VAL_KNIGHT;
    case Piece::Bishop: return VAL_BISHOP;
    case Piece::Rook:   return VAL_ROOK;
    case Piece::Queen:  return VAL_QUEEN;
    case Piece::King:   return VAL_KING;
    default:            return VAL_NONE;
  }
}

int evaluate(const Board& b) {
  int score = 0;
  for (int s = 0; s < 64; ++s) {
    Color c; Piece p = b.piece_at(s, &c);
    if (p == Piece::None) continue;
    int v = piece_value(p);
    score += (c == Color::White ? v : -v);
  }
  return score;
}

} // namespace euclid
