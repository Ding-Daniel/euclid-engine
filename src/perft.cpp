#include "euclid/perft.hpp"
#include "euclid/movegen.hpp"
#include <cassert>

namespace euclid {

static Board apply_unchecked(const Board& b, const Move& m) {
  Board nb = b;

  const Color us = b.side_to_move();
  const Color op = (us == Color::White ? Color::Black : Color::White);

  // Clear EP by default; set it only on a double push
  nb.set_ep_square(-1);

  // Identify moving piece
  Color sc;
  Piece srcP = nb.piece_at(m.from, &sc);
  assert(srcP != Piece::None && sc == us);

  // Destination occupancy
  Color dc;
  Piece dstP = nb.piece_at(m.to, &dc);

  // En passant capture?
  bool is_ep = (srcP == Piece::Pawn && dstP == Piece::None && m.to == b.ep_square());
  if (is_ep) {
    int dir = (us == Color::White ? -8 : +8); // captured pawn sits behind target square
    Square cap_sq = m.to + dir;
    Color cc; Piece cp = nb.piece_at(cap_sq, &cc);
    assert(cp == Piece::Pawn && cc == op);
    nb.remove_piece(cc, cp, cap_sq);
  } else if (dstP != Piece::None && dc != us) {
    // Normal capture
    nb.remove_piece(dc, dstP, m.to);
  }

  // Move the piece (with promotion if any)
  nb.remove_piece(us, srcP, m.from);
  nb.set_piece(us, (m.promo != Piece::None ? m.promo : srcP), m.to);

  // Halfmove clock
  if (dstP != Piece::None || is_ep || srcP == Piece::Pawn) nb.set_halfmove_clock(0);
  else nb.set_halfmove_clock(nb.halfmove_clock() + 1);

  // EP square after a double pawn push
  if (srcP == Piece::Pawn && std::abs(m.to - m.from) == 16 && file_of(m.to) == file_of(m.from)) {
    nb.set_ep_square((m.from + m.to) / 2);
  }

  // Fullmove number increments after Black's move
  if (us == Color::Black) nb.set_fullmove_number(nb.fullmove_number() + 1);

  // Flip side to move
  nb.set_side_to_move(us == Color::White ? Color::Black : Color::White);

  // TODO: castling rights adjustments (later) + check-based legality filter (later)
  return nb;
}

std::uint64_t perft(const Board& b, int depth) {
  if (depth == 0) return 1ULL;

  MoveList ml;
  generate_pseudo_legal(b, ml);

  std::uint64_t nodes = 0ULL;
  for (const auto& m : ml) {
    Board child = apply_unchecked(b, m);
    nodes += perft(child, depth - 1);
  }
  return nodes;
}

} // namespace euclid
