#include "euclid/move_do.hpp"
#include <cstdlib>

namespace euclid {

static inline Color other(Color c){ return c==Color::White?Color::Black:Color::White; }

void do_move(Board& b, const Move& m, State& st) {
  st.us            = b.side_to_move();
  st.prev_ep       = b.ep_square();
  st.prev_halfmove = b.halfmove_clock();
  st.prev_fullmove = b.fullmove_number();
  st.prev_castling = b.castling().rights; // not mutated yet, but saved for future

  // default EP cleared; set again only on double push
  b.set_ep_square(-1);

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

  // ep square after double push
  if (srcP == Piece::Pawn && std::abs(m.to - m.from) == 16 && file_of(m.to) == file_of(m.from)) {
    b.set_ep_square((m.from + m.to)/2);
  }

  // fullmove number after Black's move
  if (st.us == Color::Black) b.set_fullmove_number(st.prev_fullmove + 1);

  // swap side
  b.set_side_to_move(other(st.us));
}

void undo_move(Board& b, const Move& m, const State& st) {
  // restore side first
  b.set_side_to_move(st.us);

  // restore counters & rights
  b.set_ep_square(st.prev_ep);
  b.set_halfmove_clock(st.prev_halfmove);
  b.set_fullmove_number(st.prev_fullmove);
  Castling cr{}; cr.rights = st.prev_castling; b.set_castling(cr);

  // take moved piece back off 'to'
  Color mc; Piece movedNow = b.piece_at(m.to, &mc);
  (void)movedNow; (void)mc; // we trust our state

  b.remove_piece(st.us, movedNow, m.to);
  // if it was a promotion, we restore the pawn; else restore original
  Piece back = (m.promo != Piece::None ? Piece::Pawn : st.moved);
  b.set_piece(st.us, back, m.from);

  // restore captured (normal or EP)
  if (st.captured != Piece::None) {
    b.set_piece(other(st.us), st.captured, st.captured_sq);
  }
}

} // namespace euclid
