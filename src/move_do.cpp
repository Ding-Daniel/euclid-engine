#include "euclid/move_do.hpp"
#include <cstdlib>

namespace euclid {

static inline Color other(Color c){ return c==Color::White?Color::Black:Color::White; }

static inline void clear_rights_after_move(unsigned& r, Color side, Piece moved, Square from){
  if (moved == Piece::King) {
    if (side == Color::White) r &= ~(0x1u | 0x2u);
    else                      r &= ~(0x4u | 0x8u);
  } else if (moved == Piece::Rook) {
    if (side == Color::White) {
      if (from == 7)  r &= ~0x1u; // h1 -> K
      if (from == 0)  r &= ~0x2u; // a1 -> Q
    } else {
      if (from == 63) r &= ~0x4u; // h8 -> k
      if (from == 56) r &= ~0x8u; // a8 -> q
    }
  }
}

static inline void clear_rights_after_capture(unsigned& r, Color mover, Piece captured, Square cap_sq){
  if (captured != Piece::Rook) return;
  Color victim = other(mover);
  if (victim == Color::White) {
    if (cap_sq == 7)  r &= ~0x1u; // captured h1 rook -> no white K
    if (cap_sq == 0)  r &= ~0x2u; // captured a1 rook -> no white Q
  } else {
    if (cap_sq == 63) r &= ~0x4u; // captured h8
    if (cap_sq == 56) r &= ~0x8u; // captured a8
  }
}

void do_move(Board& b, const Move& m, State& st) {
  st.us            = b.side_to_move();
  st.prev_ep       = b.ep_square();
  st.prev_halfmove = b.halfmove_clock();
  st.prev_fullmove = b.fullmove_number();
  st.prev_castling = b.castling().rights;

  // default EP cleared; set again only on double push
  b.set_ep_square(-1);

  // CASTLING special-case
  if (m.flags == MoveFlag::Castle) {
    st.moved = Piece::King;
    st.captured = Piece::None;
    st.captured_sq = -1;

    // move king
    b.remove_piece(st.us, Piece::King, m.from);
    b.set_piece(st.us, Piece::King, m.to);

    // move rook according to destination
    if (st.us == Color::White) {
      if (m.to == 6) { // O-O e1->g1, rook h1->f1
        b.remove_piece(Color::White, Piece::Rook, 7);
        b.set_piece(Color::White, Piece::Rook, 5);
      } else {         // O-O-O e1->c1, rook a1->d1
        b.remove_piece(Color::White, Piece::Rook, 0);
        b.set_piece(Color::White, Piece::Rook, 3);
      }
    } else {
      if (m.to == 62) { // e8->g8, rook h8->f8
        b.remove_piece(Color::Black, Piece::Rook, 63);
        b.set_piece(Color::Black, Piece::Rook, 61);
      } else {          // e8->c8, rook a8->d8
        b.remove_piece(Color::Black, Piece::Rook, 56);
        b.set_piece(Color::Black, Piece::Rook, 59);
      }
    }

    // halfmove (king move, no capture)
    b.set_halfmove_clock(st.prev_halfmove + 1);

    // fullmove after Black
    if (st.us == Color::Black) b.set_fullmove_number(st.prev_fullmove + 1);

    // clear castling rights for mover
    unsigned r = st.prev_castling;
    if (st.us == Color::White) r &= ~(0x1u | 0x2u); else r &= ~(0x4u | 0x8u);
    Castling cr{}; cr.rights = r; b.set_castling(cr);

    // flip side
    b.set_side_to_move(other(st.us));
    return;
  }

  // NORMAL move path
  Color sc; Piece srcP = b.piece_at(m.from, &sc);
  st.moved = srcP;

  // destination occupancy
  Color dc; Piece dstP = b.piece_at(m.to, &dc);

  // EP capture?
  bool is_ep = (srcP == Piece::Pawn && dstP == Piece::None && m.to == st.prev_ep);
  if (is_ep) {
    int dir = (st.us == Color::White ? -8 : +8);
    Square cap_sq = m.to + dir;
    st.captured    = Piece::Pawn;
    st.captured_sq = cap_sq;
    b.remove_piece(other(st.us), Piece::Pawn, cap_sq);
  } else if (dstP != Piece::None && dc != st.us) {
    st.captured    = dstP;
    st.captured_sq = m.to;
    b.remove_piece(dc, dstP, m.to);
  } else {
    st.captured    = Piece::None;
    st.captured_sq = -1;
  }

  // move (with promotion if any)
  b.remove_piece(st.us, srcP, m.from);
  b.set_piece(st.us, (m.promo != Piece::None ? m.promo : srcP), m.to);

  // halfmove clock
  if (st.captured != Piece::None || srcP == Piece::Pawn) b.set_halfmove_clock(0);
  else b.set_halfmove_clock(st.prev_halfmove + 1);

  // EP square after double push
  if (srcP == Piece::Pawn && std::abs(m.to - m.from) == 16 && file_of(m.to) == file_of(m.from)) {
    b.set_ep_square((m.from + m.to) / 2);
  }

  // fullmove after Black
  if (st.us == Color::Black) b.set_fullmove_number(st.prev_fullmove + 1);

  // update castling rights (mover + possibly captured rook)
  unsigned r = st.prev_castling;
  clear_rights_after_move(r, st.us, srcP, m.from);
  clear_rights_after_capture(r, st.us, st.captured, st.captured_sq);
  Castling cr{}; cr.rights = r; b.set_castling(cr);

  // swap side
  b.set_side_to_move(other(st.us));
}

void undo_move(Board& b, const Move& m, const State& st) {
  // restore side
  b.set_side_to_move(st.us);

  // restore counters & rights
  b.set_ep_square(st.prev_ep);
  b.set_halfmove_clock(st.prev_halfmove);
  b.set_fullmove_number(st.prev_fullmove);
  Castling cr{}; cr.rights = st.prev_castling; b.set_castling(cr);

  if (m.flags == MoveFlag::Castle) {
    // move king back
    b.remove_piece(st.us, Piece::King, m.to);
    b.set_piece(st.us, Piece::King, m.from);
    // move rook back
    if (st.us == Color::White) {
      if (m.to == 6) { b.remove_piece(Color::White, Piece::Rook, 5); b.set_piece(Color::White, Piece::Rook, 7); }
      else            { b.remove_piece(Color::White, Piece::Rook, 3); b.set_piece(Color::White, Piece::Rook, 0); }
    } else {
      if (m.to == 62){ b.remove_piece(Color::Black, Piece::Rook, 61); b.set_piece(Color::Black, Piece::Rook, 63); }
      else           { b.remove_piece(Color::Black, Piece::Rook, 59); b.set_piece(Color::Black, Piece::Rook, 56); }
    }
    return;
  }

  // normal undo
  Color mc; Piece movedNow = b.piece_at(m.to, &mc);
  b.remove_piece(st.us, movedNow, m.to);
  Piece back = (m.promo != Piece::None ? Piece::Pawn : st.moved);
  b.set_piece(st.us, back, m.from);

  if (st.captured != Piece::None) {
    b.set_piece(other(st.us), st.captured, st.captured_sq);
  }
}

} // namespace euclid
