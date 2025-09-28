#include "euclid/movegen.hpp"
#include "euclid/attack.hpp" 
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

  const Color us  = b.side_to_move();
  const Color opp = (us == Color::White ? Color::Black : Color::White);
  const Square oppK = find_king(b, opp);

  // -----------------
  // Knights
  // -----------------
  static constexpr int KN_OFF[8] = {+17, +15, +10, +6, -6, -10, -15, -17};
  for (int s = 0; s < 64; ++s) {
    Color pcColor; Piece pc = b.piece_at(s, &pcColor);
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
        out.push(Move{ s, t, MoveFlag::Quiet,  Piece::None });
      } else if (oc != us && op != Piece::King) {
        out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
      }
    }
  }

  // -----------------
  // Bishops (diagonals)
  // -----------------
  static constexpr int DFb[4] = {+1, +1, -1, -1};
  static constexpr int DRb[4] = {+1, -1, +1, -1};
  for (int s = 0; s < 64; ++s) {
    Color pcColor; Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Bishop || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);
    for (int dir = 0; dir < 4; ++dir) {
      int f = f0 + DFb[dir], r = r0 + DRb[dir];
      while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        const int t = r * 8 + f;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op == Piece::None) {
          out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
        } else {
          if (oc != us && op != Piece::King) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
          break; // blocked
        }
        f += DFb[dir];
        r += DRb[dir];
      }
    }
  }

  // -----------------
  // Rooks (orthogonals)
  // -----------------
  static constexpr int DFr[4] = {+1, -1,  0,  0};
  static constexpr int DRr[4] = { 0,  0, +1, -1};
  for (int s = 0; s < 64; ++s) {
    Color pcColor; Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Rook || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);
    for (int dir = 0; dir < 4; ++dir) {
      int f = f0 + DFr[dir], r = r0 + DRr[dir];
      while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        const int t = r * 8 + f;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op == Piece::None) {
          out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
        } else {
          if (oc != us && op != Piece::King) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
          break; // blocked
        }
        f += DFr[dir];
        r += DRr[dir];
      }
    }
  }

  // -----------------
  // Queens (diagonals + orthogonals)
  // -----------------
  for (int s = 0; s < 64; ++s) {
    Color pcColor; Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Queen || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);

    // Diagonals
    for (int dir = 0; dir < 4; ++dir) {
      int f = f0 + DFb[dir], r = r0 + DRb[dir];
      while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        const int t = r * 8 + f;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op == Piece::None) {
          out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
        } else {
          if (oc != us && op != Piece::King) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
          break;
        }
        f += DFb[dir];
        r += DRb[dir];
      }
    }
    // Orthogonals
    for (int dir = 0; dir < 4; ++dir) {
      int f = f0 + DFr[dir], r = r0 + DRr[dir];
      while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        const int t = r * 8 + f;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op == Piece::None) {
          out.push(Move{ s, t, MoveFlag::Quiet, Piece::None });
        } else {
          if (oc != us && op != Piece::King) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
          break;
        }
        f += DFr[dir];
        r += DRr[dir];
      }
    }
  }

  // -----------------
  // Pawns (push, double, captures, promotions, EP gen)
  // -----------------
  const int ep = b.ep_square();
  for (int s = 0; s < 64; ++s) {
    Color pcColor; Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::Pawn || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);

    if (us == Color::White) {
      // forward
      int one = s + 8;
      if (r0 < 7 && on_board(one)) {
        Color oc; Piece op = b.piece_at(one, &oc);
        if (op == Piece::None) {
          if (r0 == 6) {
            // promotions (to rank 8)
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Queen });
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Rook  });
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Bishop});
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Knight});
          } else {
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::None });
            // double
            if (r0 == 1) {
              int two = s + 16;
              Color oc2; Piece op2 = b.piece_at(two, &oc2);
              if (op2 == Piece::None) {
                out.push(Move{ s, two, MoveFlag::DoublePush, Piece::None });
              }
            }
          }
        }
      }
      // captures (including promo)
      for (int df : {-1, +1}) {
        int nf = f0 + df;
        if (nf < 0 || nf > 7) continue;
        int t = s + (df == -1 ? 7 : 9);
        if (!on_board(t)) continue;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op != Piece::None && oc == opp) {
          if (r0 == 6) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Queen });
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Rook  });
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Bishop});
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Knight});
          } else {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
        }
      }
      // en-passant gen (FEN guarantees validity if ep >= 0)
      if (ep >= 0) {
        if (s + 7 == ep && f0 > 0)  out.push(Move{ s, ep, MoveFlag::EnPassant, Piece::None });
        if (s + 9 == ep && f0 < 7)  out.push(Move{ s, ep, MoveFlag::EnPassant, Piece::None });
      }
    } else {
      // Black
      int one = s - 8;
      if (r0 > 0 && on_board(one)) {
        Color oc; Piece op = b.piece_at(one, &oc);
        if (op == Piece::None) {
          if (r0 == 1) {
            // promotions (to rank 1)
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Queen });
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Rook  });
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Bishop});
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::Knight});
          } else {
            out.push(Move{ s, one, MoveFlag::Quiet, Piece::None });
            if (r0 == 6) {
              int two = s - 16;
              Color oc2; Piece op2 = b.piece_at(two, &oc2);
              if (op2 == Piece::None) {
                out.push(Move{ s, two, MoveFlag::DoublePush, Piece::None });
              }
            }
          }
        }
      }
      // captures (including promo)
      for (int df : {-1, +1}) {
        int nf = f0 + df;
        if (nf < 0 || nf > 7) continue;
        int t = s + (df == -1 ? -9 : -7);
        if (!on_board(t)) continue;
        Color oc; Piece op = b.piece_at(t, &oc);
        if (op != Piece::None && oc == opp) {
          if (r0 == 1) {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Queen });
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Rook  });
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Bishop});
            out.push(Move{ s, t, MoveFlag::Capture, Piece::Knight});
          } else {
            out.push(Move{ s, t, MoveFlag::Capture, Piece::None });
          }
        }
      }
      // en-passant gen
      if (ep >= 0) {
        if (s - 9 == ep && f0 > 0)  out.push(Move{ s, ep, MoveFlag::EnPassant, Piece::None });
        if (s - 7 == ep && f0 < 7)  out.push(Move{ s, ep, MoveFlag::EnPassant, Piece::None });
      }
    }
  }

  // -----------------
  // Kings (no adjacency to enemy king)
  // -----------------
  static constexpr int K_OFF[8] = {+8, -8, +1, -1, +9, +7, -7, -9};
  for (int s = 0; s < 64; ++s) {
    Color pcColor; Piece pc = b.piece_at(s, &pcColor);
    if (pc != Piece::King || pcColor != us) continue;

    const int f0 = file_of(s), r0 = rank_of(s);
    for (int d : K_OFF) {
      const int t = s + d;
      if (!on_board(t)) continue;
      const int f1 = file_of(t), r1 = rank_of(t);
      const int df = absd(f1, f0), dr = absd(r1, r0);
      if (df > 1 || dr > 1) continue; // wrap guard

      Color oc; Piece op = b.piece_at(t, &oc);
      if (oc == us) continue;
      if (op == Piece::King) continue;           // never capture king
      if (kings_adjacent(t, oppK)) continue;     // simple legality

      const bool isCap = (op != Piece::None && oc == opp);
      out.push(Move{ s, t, isCap ? MoveFlag::Capture : MoveFlag::Quiet, Piece::None });
    }
  }
  // -----------------
  // Castling (new)
  // -----------------
  const unsigned cr = b.castling().rights;
  if (us == Color::White) {
    // king must be on e1
    Color kc; if (b.piece_at(4, &kc) == Piece::King && kc == Color::White) {
      // O-O: rights K, squares f1(5), g1(6) empty, e1/f1/g1 not attacked, rook h1(7)
      if (cr & 0x1) {
        Color rc; Piece r = b.piece_at(7, &rc);
        if (r == Piece::Rook && rc == Color::White) {
          Color c1; Piece p1 = b.piece_at(5, &c1);
          Color c2; Piece p2 = b.piece_at(6, &c2);
          if (p1 == Piece::None && p2 == Piece::None) {
            if (!in_check(b, Color::White) &&
                !square_attacked(b, 5, Color::Black) &&
                !square_attacked(b, 6, Color::Black)) {
              out.push(Move{ 4, 6, MoveFlag::Castle, Piece::None });
            }
          }
        }
      }
      // O-O-O: rights Q, squares d1(3), c1(2), b1(1) empty, e1/d1/c1 not attacked, rook a1(0)
      if (cr & 0x2) {
        Color rc; Piece r = b.piece_at(0, &rc);
        if (r == Piece::Rook && rc == Color::White) {
          Color c1; Piece p1 = b.piece_at(3, &c1);
          Color c2; Piece p2 = b.piece_at(2, &c2);
          Color c3; Piece p3 = b.piece_at(1, &c3);
          if (p1 == Piece::None && p2 == Piece::None && p3 == Piece::None) {
            if (!in_check(b, Color::White) &&
                !square_attacked(b, 3, Color::Black) &&
                !square_attacked(b, 2, Color::Black)) {
              out.push(Move{ 4, 2, MoveFlag::Castle, Piece::None });
            }
          }
        }
      }
    }
  } else {
    // Black: king on e8 (60)
    Color kc; if (b.piece_at(60, &kc) == Piece::King && kc == Color::Black) {
      // O-O: rights k, squares f8(61), g8(62)
      if (cr & 0x4) {
        Color rc; Piece r = b.piece_at(63, &rc);
        if (r == Piece::Rook && rc == Color::Black) {
          Color c1; Piece p1 = b.piece_at(61, &c1);
          Color c2; Piece p2 = b.piece_at(62, &c2);
          if (p1 == Piece::None && p2 == Piece::None) {
            if (!in_check(b, Color::Black) &&
                !square_attacked(b, 61, Color::White) &&
                !square_attacked(b, 62, Color::White)) {
              out.push(Move{ 60, 62, MoveFlag::Castle, Piece::None });
            }
          }
        }
      }
      // O-O-O: rights q, squares d8(59), c8(58), b8(57)
      if (cr & 0x8) {
        Color rc; Piece r = b.piece_at(56, &rc);
        if (r == Piece::Rook && rc == Color::Black) {
          Color c1; Piece p1 = b.piece_at(59, &c1);
          Color c2; Piece p2 = b.piece_at(58, &c2);
          Color c3; Piece p3 = b.piece_at(57, &c3);
          if (p1 == Piece::None && p2 == Piece::None && p3 == Piece::None) {
            if (!in_check(b, Color::Black) &&
                !square_attacked(b, 59, Color::White) &&
                !square_attacked(b, 58, Color::White)) {
              out.push(Move{ 60, 58, MoveFlag::Castle, Piece::None });
            }
          }
        }
      }
    }
  }
}

} // namespace euclid
