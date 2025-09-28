#include "euclid/attack.hpp"
#include <cstdlib>

namespace euclid {

static inline bool on_board(int s) { return s >= 0 && s < 64; }
static inline Color them(Color us) { return us == Color::White ? Color::Black : Color::White; }
//static inline int absd(int a, int b) { return std::abs(a - b); } // shows annoying error when building

bool square_attacked(const Board& b, Square s, Color by) {
  // Knights
  static constexpr int KN[8] = {+17, +15, +10, +6, -6, -10, -15, -17};
  for (int d : KN) {
    int q = s - d; // inverse: a knight at q could jump to s
    if (!on_board(q)) continue;
    int df = std::abs(file_of(q) - file_of(s));
    int dr = std::abs(rank_of(q) - rank_of(s));
    if (!((df == 1 && dr == 2) || (df == 2 && dr == 1))) continue;
    Color c; Piece p = b.piece_at(q, &c);
    if (p == Piece::Knight && c == by) return true;
  }

  // King
  for (int df = -1; df <= 1; ++df)
    for (int dr = -1; dr <= 1; ++dr) {
      if (!df && !dr) continue;
      int f = file_of(s) + df, r = rank_of(s) + dr;
      if (f < 0 || f > 7 || r < 0 || r > 7) continue;
      int q = r * 8 + f;
      Color c; Piece p = b.piece_at(q, &c);
      if (p == Piece::King && c == by) return true;
    }

  // Pawns (note: check from source squares that could capture into s)
  if (by == Color::White) {
    // white captures down-right (+9) and down-left (+7) into s => source squares s-7, s-9
    if (file_of(s) > 0) {
      int q = s - 9;
      if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p == Piece::Pawn && c == by) return true; }
    }
    if (file_of(s) < 7) {
      int q = s - 7;
      if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p == Piece::Pawn && c == by) return true; }
    }
  } else {
    // black captures up-right (-7) and up-left (-9) into s => source squares s+7, s+9
    if (file_of(s) > 0) {
      int q = s + 7;
      if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p == Piece::Pawn && c == by) return true; }
    }
    if (file_of(s) < 7) {
      int q = s + 9;
      if (on_board(q)) { Color c; Piece p = b.piece_at(q, &c); if (p == Piece::Pawn && c == by) return true; }
    }
  }

  // Diagonals (bishop/queen)
  static constexpr int DFb[4] = {+1, +1, -1, -1};
  static constexpr int DRb[4] = {+1, -1, +1, -1};
  for (int dir = 0; dir < 4; ++dir) {
    int f = file_of(s) + DFb[dir], r = rank_of(s) + DRb[dir];
    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
      int q = r * 8 + f;
      Color c; Piece p = b.piece_at(q, &c);
      if (p != Piece::None) {
        if (c == by && (p == Piece::Bishop || p == Piece::Queen)) return true;
        break;
      }
      f += DFb[dir]; r += DRb[dir];
    }
  }

  // Orthogonals (rook/queen)
  static constexpr int DFr[4] = {+1, -1,  0,  0};
  static constexpr int DRr[4] = { 0,  0, +1, -1};
  for (int dir = 0; dir < 4; ++dir) {
    int f = file_of(s) + DFr[dir], r = rank_of(s) + DRr[dir];
    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
      int q = r * 8 + f;
      Color c; Piece p = b.piece_at(q, &c);
      if (p != Piece::None) {
        if (c == by && (p == Piece::Rook || p == Piece::Queen)) return true;
        break;
      }
      f += DFr[dir]; r += DRr[dir];
    }
  }

  return false;
}

bool in_check(const Board& b, Color side) {
  // find our king
  Square ks = -1;
  for (int s = 0; s < 64; ++s) {
    Color c; Piece p = b.piece_at(s, &c);
    if (p == Piece::King && c == side) { ks = s; break; }
  }
  if (ks < 0) return false; // (non-standard positions) fail-safe
  return square_attacked(b, ks, them(side));
}

} // namespace euclid
