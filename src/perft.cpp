#include "euclid/perft.hpp"
#include "euclid/movegen.hpp"
#include <cassert>

namespace euclid {

static Board apply_unchecked(const Board& b, const Move& m) {
  Board nb = b;

  const Color us = b.side_to_move();

  // Handle capture (if any)
  Color dc;
  Piece dstP = nb.piece_at(m.to, &dc);
  if (dstP != Piece::None && dc != us) {
    nb.remove_piece(dc, dstP, m.to);
  }

  // Move the piece
  Color sc;
  Piece srcP = nb.piece_at(m.from, &sc);
  assert(srcP != Piece::None && sc == us);

  nb.remove_piece(us, srcP, m.from);
  nb.set_piece(us, (m.promo != Piece::None ? m.promo : srcP), m.to);

  // Non-pawn for this step; halfmove resets on capture, else ++
  if (dstP != Piece::None /*capture*/ || srcP == Piece::Pawn) nb.set_halfmove_clock(0);
  else nb.set_halfmove_clock(nb.halfmove_clock() + 1);

  // EP square cleared (no pawn logic yet)
  nb.set_ep_square(-1);

  // Fullmove number increments after Black's move
  if (us == Color::Black) nb.set_fullmove_number(nb.fullmove_number() + 1);

  // Flip side to move
  nb.set_side_to_move(us == Color::White ? Color::Black : Color::White);

  // TODO (later): update castling rights if king/rook moves; legality filter.
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
