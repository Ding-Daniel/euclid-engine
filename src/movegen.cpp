#include "euclid/movegen.hpp"

#include "euclid/attack.hpp" // square_attacked, in_check
#include <cstdint>
#include <cstdlib>

namespace euclid {
namespace {

static inline Color other(Color c){ return c==Color::White?Color::Black:Color::White; }

static inline bool kings_adjacent(Square a, Square b) {
  int fa = file_of(a), ra = rank_of(a);
  int fb = file_of(b), rb = rank_of(b);
  return std::abs(fa - fb) <= 1 && std::abs(ra - rb) <= 1;
}

static inline Square find_king(const Board& b, Color c) {
  for (int s = 0; s < 64; ++s) {
    Color cc;
    Piece p = b.piece_at(s, &cc);
    if (p == Piece::King && cc == c) return s;
  }
  return -1;
}

static inline void push_promo_or_quiet(MoveList& out, Square from, Square to, MoveFlag fl) {
  // For now, only queen promotions are generated (sufficient for early engine phases).
  int r = rank_of(to);
  if (r == 0 || r == 7) {
    Move m{};
    m.from = from; m.to = to; m.flags = fl; m.promo = Piece::Queen;
    out.push(m);
  } else {
    Move m{};
    m.from = from; m.to = to; m.flags = fl; m.promo = Piece::None;
    out.push(m);
  }
}

// ---- Pawn move generation (includes EP target square; legality checked by caller) ----
static void gen_pawn_moves(const Board& b, MoveList& out) {
  const Color us  = b.side_to_move();
  const Color opp = other(us);
  const int ep = b.ep_square();

  for (int s = 0; s < 64; ++s) {
    Color pc;
    Piece p = b.piece_at(s, &pc);
    if (p != Piece::Pawn || pc != us) continue;

    const int f0 = file_of(s);
    const int r0 = rank_of(s);

    if (us == Color::White) {
      // forward 1
      const int fwd = s + 8;
      if (fwd < 64) {
        Color oc; Piece occ = b.piece_at(fwd, &oc);
        if (occ == Piece::None) {
          push_promo_or_quiet(out, s, fwd, MoveFlag::Quiet);

          // forward 2 from rank 2 (r0 == 1), if both empty
          if (r0 == 1) {
            const int fwd2 = s + 16;
            if (fwd2 < 64) {
              Color oc2; Piece occ2 = b.piece_at(fwd2, &oc2);
              if (occ2 == Piece::None) {
                Move m{}; m.from = s; m.to = fwd2; m.flags = MoveFlag::Quiet; m.promo = Piece::None;
                out.push(m);
              }
            }
          }
        }
      }

      // captures
      if (f0 > 0) {
        const int cap = s + 7;
        if (cap < 64) {
          Color oc; Piece tp = b.piece_at(cap, &oc);
          if (tp != Piece::None && oc == opp && tp != Piece::King) {
            push_promo_or_quiet(out, s, cap, MoveFlag::Capture);
          }
        }
      }
      if (f0 < 7) {
        const int cap = s + 9;
        if (cap < 64) {
          Color oc; Piece tp = b.piece_at(cap, &oc);
          if (tp != Piece::None && oc == opp && tp != Piece::King) {
            push_promo_or_quiet(out, s, cap, MoveFlag::Capture);
          }
        }
      }

      // en passant (destination is ep square; captured pawn handled in do_move via ep_square)
      if (ep >= 0) {
        if (f0 > 0 && (s + 7) == ep) {
          Move m{}; m.from = s; m.to = ep; m.flags = MoveFlag::EnPassant; m.promo = Piece::None;
          out.push(m);
        }
        if (f0 < 7 && (s + 9) == ep) {
          Move m{}; m.from = s; m.to = ep; m.flags = MoveFlag::EnPassant; m.promo = Piece::None;
          out.push(m);
        }
      }
    } else {
      // Black pawns
      const int fwd = s - 8;
      if (fwd >= 0) {
        Color oc; Piece occ = b.piece_at(fwd, &oc);
        if (occ == Piece::None) {
          push_promo_or_quiet(out, s, fwd, MoveFlag::Quiet);

          if (r0 == 6) {
            const int fwd2 = s - 16;
            if (fwd2 >= 0) {
              Color oc2; Piece occ2 = b.piece_at(fwd2, &oc2);
              if (occ2 == Piece::None) {
                Move m{}; m.from = s; m.to = fwd2; m.flags = MoveFlag::Quiet; m.promo = Piece::None;
                out.push(m);
              }
            }
          }
        }
      }

      // captures
      if (f0 > 0) {
        const int cap = s - 9;
        if (cap >= 0) {
          Color oc; Piece tp = b.piece_at(cap, &oc);
          if (tp != Piece::None && oc == opp && tp != Piece::King) {
            push_promo_or_quiet(out, s, cap, MoveFlag::Capture);
          }
        }
      }
      if (f0 < 7) {
        const int cap = s - 7;
        if (cap >= 0) {
          Color oc; Piece tp = b.piece_at(cap, &oc);
          if (tp != Piece::None && oc == opp && tp != Piece::King) {
            push_promo_or_quiet(out, s, cap, MoveFlag::Capture);
          }
        }
      }

      // en passant
      if (ep >= 0) {
        if (f0 > 0 && (s - 9) == ep) {
          Move m{}; m.from = s; m.to = ep; m.flags = MoveFlag::EnPassant; m.promo = Piece::None;
          out.push(m);
        }
        if (f0 < 7 && (s - 7) == ep) {
          Move m{}; m.from = s; m.to = ep; m.flags = MoveFlag::EnPassant; m.promo = Piece::None;
          out.push(m);
        }
      }
    }
  }
}

// ---- Knight moves ----
static void gen_knight_moves(const Board& b, MoveList& out) {
  const Color us  = b.side_to_move();
  const Color opp = other(us);

  static const int d[8] = { 17, 15, 10, 6, -17, -15, -10, -6 };

  for (int s = 0; s < 64; ++s) {
    Color pc;
    Piece p = b.piece_at(s, &pc);
    if (p != Piece::Knight || pc != us) continue;

    const int f0 = file_of(s);
    const int r0 = rank_of(s);

    for (int k = 0; k < 8; ++k) {
      int t = s + d[k];
      if (t < 0 || t >= 64) continue;
      const int f1 = file_of(t);
      const int r1 = rank_of(t);

      // Ensure valid L-shape without wrap
      if (std::abs(f1 - f0) + std::abs(r1 - r0) != 3) continue;

      Color oc;
      Piece op = b.piece_at(t, &oc);
      if (op == Piece::None || oc != us) {
        if (op == Piece::King) continue;
        Move m{};
        m.from = s; m.to = t;
        m.flags = (op == Piece::None ? MoveFlag::Quiet : MoveFlag::Capture);
        m.promo = Piece::None;
        out.push(m);
      }
    }
  }
}

// ---- Sliding pieces (bishop/rook/queen) ----
static void gen_slider(const Board& b, MoveList& out, Piece which) {
  const Color us  = b.side_to_move();
  const Color opp = other(us);

  const int dirsB[4] = { 9, 7, -9, -7 };
  const int dirsR[4] = { 8, -8, 1, -1 };

  const int* dirs = nullptr;
  int nd = 0;

  if (which == Piece::Bishop) { dirs = dirsB; nd = 4; }
  if (which == Piece::Rook)   { dirs = dirsR; nd = 4; }

  for (int s = 0; s < 64; ++s) {
    Color pc;
    Piece p = b.piece_at(s, &pc);
    if (pc != us) continue;
    if (which == Piece::Bishop && p != Piece::Bishop) continue;
    if (which == Piece::Rook   && p != Piece::Rook)   continue;
    if (which == Piece::Queen  && p != Piece::Queen)  continue;

    auto emit_ray = [&](int step) {
      int t = s;
      int f0 = file_of(s);

      for (;;) {
        int nxt = t + step;
        if (nxt < 0 || nxt >= 64) break;

        // file wrap check for horizontal/diagonal
        int f1 = file_of(nxt);
        if (std::abs(f1 - file_of(t)) > 1 && (step == 1 || step == -1 || step == 9 || step == -9 || step == 7 || step == -7))
          break;
        if ((step == 1 || step == -1) && std::abs(f1 - f0) != std::abs(nxt - s) % 8) {
          // conservative; the earlier wrap guard is usually enough
        }

        t = nxt;

        Color oc;
        Piece op = b.piece_at(t, &oc);

        if (op != Piece::None && oc == us) break;
        if (op == Piece::King) break;

        Move m{};
        m.from = s; m.to = t;
        m.flags = (op == Piece::None ? MoveFlag::Quiet : MoveFlag::Capture);
        m.promo = Piece::None;
        out.push(m);

        if (op != Piece::None) break; // capture stops ray
      }
    };

    if (which == Piece::Queen) {
      // queen = bishop + rook
      for (int i = 0; i < 4; ++i) emit_ray(dirsB[i]);
      for (int i = 0; i < 4; ++i) emit_ray(dirsR[i]);
    } else {
      for (int i = 0; i < nd; ++i) emit_ray(dirs[i]);
    }
  }
}

// ---- King moves (adjacency restriction is correct here) ----
static void gen_king_moves(const Board& b, MoveList& out) {
  const Color us  = b.side_to_move();
  const Color opp = other(us);

  Square ks = find_king(b, us);
  Square ok = find_king(b, opp);

  static const int d[8] = { 8, -8, 1, -1, 9, -9, 7, -7 };

  for (int k = 0; k < 8; ++k) {
    int t = ks + d[k];
    if (t < 0 || t >= 64) continue;

    // avoid wrap for horizontal/diag
    if (std::abs(file_of(t) - file_of(ks)) > 1) continue;

    Color oc;
    Piece op = b.piece_at(t, &oc);
    if (op != Piece::None && oc == us) continue;
    if (op == Piece::King) continue;

    // King cannot move adjacent to enemy king
    if (kings_adjacent(t, ok)) continue;

    // King cannot move into check
    if (square_attacked(b, t, opp)) continue;

    Move m{};
    m.from = ks; m.to = t;
    m.flags = (op == Piece::None ? MoveFlag::Quiet : MoveFlag::Capture);
    m.promo = Piece::None;
    out.push(m);
  }

  // Castling (basic: empty squares, not in check, and squares passed through not attacked)
  Castling cr = b.castling();
  if (us == Color::White) {
    // White O-O: e1->g1, squares f1,g1 empty, not in check, f1/g1 not attacked
    if (cr.rights & 0x1u) {
      if (b.piece_at(5) == Piece::None && b.piece_at(6) == Piece::None) {
        if (!in_check(b, us) &&
            !square_attacked(b, 5, opp) &&
            !square_attacked(b, 6, opp)) {
          Move m{}; m.from = 4; m.to = 6; m.flags = MoveFlag::Castle; m.promo = Piece::None;
          out.push(m);
        }
      }
    }
    // White O-O-O: e1->c1, squares d1,c1,b1 empty, not in check, d1/c1 not attacked
    if (cr.rights & 0x2u) {
      if (b.piece_at(3) == Piece::None && b.piece_at(2) == Piece::None && b.piece_at(1) == Piece::None) {
        if (!in_check(b, us) &&
            !square_attacked(b, 3, opp) &&
            !square_attacked(b, 2, opp)) {
          Move m{}; m.from = 4; m.to = 2; m.flags = MoveFlag::Castle; m.promo = Piece::None;
          out.push(m);
        }
      }
    }
  } else {
    // Black O-O: e8->g8
    if (cr.rights & 0x4u) {
      if (b.piece_at(61) == Piece::None && b.piece_at(62) == Piece::None) {
        if (!in_check(b, us) &&
            !square_attacked(b, 61, opp) &&
            !square_attacked(b, 62, opp)) {
          Move m{}; m.from = 60; m.to = 62; m.flags = MoveFlag::Castle; m.promo = Piece::None;
          out.push(m);
        }
      }
    }
    // Black O-O-O: e8->c8
    if (cr.rights & 0x8u) {
      if (b.piece_at(59) == Piece::None && b.piece_at(58) == Piece::None && b.piece_at(57) == Piece::None) {
        if (!in_check(b, us) &&
            !square_attacked(b, 59, opp) &&
            !square_attacked(b, 58, opp)) {
          Move m{}; m.from = 60; m.to = 58; m.flags = MoveFlag::Castle; m.promo = Piece::None;
          out.push(m);
        }
      }
    }
  }
}

} // namespace

void generate_pseudo_legal(const Board& b, MoveList& out) {
  out.sz = 0;
  gen_pawn_moves(b, out);
  gen_knight_moves(b, out);
  gen_slider(b, out, Piece::Bishop);
  gen_slider(b, out, Piece::Rook);
  gen_slider(b, out, Piece::Queen);
  gen_king_moves(b, out);
}

} // namespace euclid
