#include "euclid/attack.hpp"
#include "euclid/types.hpp"
#include "euclid/board.hpp"

namespace euclid {

static inline bool on_board(int s){ return s >= 0 && s < 64; }
static inline Color other(Color c){ return c==Color::White?Color::Black:Color::White; }

bool square_attacked(const Board& b, Square s, Color by) {
  // Knights (sources that can jump to s)
  {
    static constexpr int KN_OFF[8] = {+17,+15,+10,+6,-6,-10,-15,-17};
    for (int d : KN_OFF) {
      int q = s + d;
      if (!on_board(q)) continue;
      int df = std::abs(file_of(q) - file_of(s));
      int dr = std::abs(rank_of(q) - rank_of(s));
      if (!((df==1 && dr==2) || (df==2 && dr==1))) continue;
      Color c; Piece p = b.piece_at(q, &c);
      if (p == Piece::Knight && c == by) return true;
    }
  }

  // King (adjacent sources)
  {
    for (int df=-1; df<=1; ++df) for (int dr=-1; dr<=1; ++dr) {
      if (!df && !dr) continue;
      int f = file_of(s) + df, r = rank_of(s) + dr;
      if (f < 0 || f > 7 || r < 0 || r > 7) continue;
      int q = r*8 + f;
      Color c; Piece p = b.piece_at(q, &c);
      if (p == Piece::King && c == by) return true;
    }
  }

  // Pawns (sources that capture into s)
  if (by == Color::White) {
    if (file_of(s) > 0)  { int q = s - 9; if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p==Piece::Pawn && c==by) return true; } }
    if (file_of(s) < 7)  { int q = s - 7; if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p==Piece::Pawn && c==by) return true; } }
  } else {
    if (file_of(s) > 0)  { int q = s + 7; if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p==Piece::Pawn && c==by) return true; } }
    if (file_of(s) < 7)  { int q = s + 9; if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p==Piece::Pawn && c==by) return true; } }
  }

  // Sliders: first blocker along each ray from s
  // Diagonals -> bishop/queen
  {
    static constexpr int DF[4] = {+1, +1, -1, -1};
    static constexpr int DR[4] = {+1, -1, +1, -1};
    for (int dir=0; dir<4; ++dir) {
      int f = file_of(s) + DF[dir], r = rank_of(s) + DR[dir];
      while (f>=0 && f<8 && r>=0 && r<8) {
        int q = r*8 + f;
        Color c; Piece p = b.piece_at(q, &c);
        if (p != Piece::None) {
          if (c==by && (p==Piece::Bishop || p==Piece::Queen)) return true;
          break;
        }
        f += DF[dir]; r += DR[dir];
      }
    }
  }
  // Orthogonals -> rook/queen
  {
    static constexpr int DF[4] = {+1, -1,  0,  0}; // E, W,  -,  -
    static constexpr int DR[4] = { 0,  0, +1, -1}; // -,  -,  N,  S
    for (int dir=0; dir<4; ++dir) {
      int f = file_of(s) + DF[dir], r = rank_of(s) + DR[dir];
      while (f>=0 && f<8 && r>=0 && r<8) {
        int q = r*8 + f;
        Color c; Piece p = b.piece_at(q, &c);
        if (p != Piece::None) {
          if (c==by && (p==Piece::Rook || p==Piece::Queen)) return true;
          break;
        }
        f += DF[dir]; r += DR[dir];
      }
    }
  }

  return false;
}

bool in_check(const Board& b, Color side) {
  int ks = -1;
  for (int s=0; s<64; ++s) {
    Color c; Piece p = b.piece_at(s, &c);
    if (p == Piece::King && c == side) { ks = s; break; }
  }
  if (ks < 0) return false; // fail-safe
  return square_attacked(b, ks, other(side));
}

} // namespace euclid
