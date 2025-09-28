#include "euclid/movegen.hpp"
#include <cstdlib> // std::abs

namespace euclid {

static inline bool on_board(int s) { return s >= 0 && s < 64; }
static inline Color them(Color us) { return us == Color::White ? Color::Black : Color::White; }
static inline int absd(int a, int b) { return std::abs(a - b); }

static inline bool kings_adjacent(Square a, Square b) {
  if (a < 0 || b < 0) return false;
  return absd(file_of(a), file_of(b)) <= 1 && absd(rank_of(a), rank_of(b)) <= 1;
}

static inline Square find_king(const Board& b, Color who) {
  for (int s = 0; s < 64; ++s) {
    Color c; Piece p = b.piece_at(s, &c);
    if (p == Piece::King && c == who) return s;
  }
  return -1;
}

void generate_pseudo_legal(const Board& b, MoveList& out) {
  out.sz = 0;

  const Color us = b.side_to_move();
  const Color opp = them(us);
  const Square oppK = find_king(b, opp);

  // -----------------
  // Knights
  // -----------------
  static constexpr int KN_OFF[8] = {+17, +15, +10, +6, -6, -10, -15, -17};
  for (int s = 0; s < 64; ++s) {
    Color pcColor;
    Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Knight || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);
    for (int d : KN_OFF) {
      const int t = s + d;
      if (!on_board(t)) continue;
      const int f1 = file_of(t), r1 = rank_of(t);
      const int df = absd(f1, f0), dr = absd(r1, r0);
      if (!((df == 1 && dr == 2) || (df == 2 && dr == 1))) continue;

      Color oc; Piece op = b.piece_at(t, &oc);
      if (op == Piece::None) {
        out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
      } else if (oc != us && op != Piece::King) {
        out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
      }
    }
  }

  // -----------------
  // Bishops (new)
  // -----------------
  static constexpr int DF[4] = {+1, +1, -1, -1};
  static constexpr int DR[4] = {+1, -1, +1, -1};
  for (int s = 0; s < 64; ++s) {
    Color pcColor;
    Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Bishop || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);
    for (int dir = 0; dir < 4; ++dir) {
      int f = f0 + DF[dir];
      int r = r0 + DR[dir];
      while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        const int t = r * 8 + f;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op == Piece::None) {
          out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
        } else {
          if (oc != us && op != Piece::King) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
          break; // blocked by any piece
        }
        f += DF[dir];
        r += DR[dir];
      }
    }
  }

  // -----------------
  // Kings
  // -----------------
  static constexpr int K_OFF[8] = {+8, -8, +1, -1, +9, +7, -7, -9};
  for (int s = 0; s < 64; ++s) {
    Color pcColor;
    Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::King || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);
    for (int d : K_OFF) {
      const int t = s + d;
      if (!on_board(t)) continue;
      const int f1 = file_of(t), r1 = rank_of(t);
      const int df = absd(f1, f0), dr = absd(r1, r0);
      if (df > 1 || dr > 1) continue; // guard wrap-around

      Color oc; Piece op = b.piece_at(t, &oc);
      if (oc == us) continue;           // own piece
      if (op == Piece::King) continue;  // never capture king

      // simple legality: moving king cannot end adjacent to enemy king
      if (kings_adjacent(t, oppK)) continue;

      const bool isCap = (op != Piece::None && oc == opp);
      out.push(Move{ s, t, isCap ? MoveFlag::Capture : MoveFlag::Quiet, Piece::None });
    }
  }

  // (Rooks/Queens/Pawns come in later steps.)
}

} // namespace euclid
