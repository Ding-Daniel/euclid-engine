#include "euclid/encode.hpp"

#include <cstddef>

#include "euclid/types.hpp"

namespace euclid {

std::vector<float> encode_12x64(const Board& b) {
  std::vector<float> x(static_cast<std::size_t>(EncodedInputSpec::kTotal), 0.0f);

  // 12x64 piece planes.
  for (Square s = 0; s < 64; ++s) {
    Color c = Color::White;
    const Piece p = b.piece_at(s, &c);
    if (p == Piece::None) continue;

    const int pieceIdx = static_cast<int>(p); // Pawn..King => 0..5 (per types.hpp)
    if (pieceIdx < 0 || pieceIdx >= 6) continue; // defensive

    const int colorOffset = (c == Color::White) ? 0 : 6;
    const int plane = colorOffset + pieceIdx;    // 0..11
    const int idx = plane * 64 + s;              // 0..767

    x[static_cast<std::size_t>(idx)] = 1.0f;
  }

  int off = EncodedInputSpec::kPieceFeatures; // 768

  // Side to move.
  x[static_cast<std::size_t>(off)] = (b.side_to_move() == Color::White) ? 1.0f : 0.0f;
  ++off;

  // Castling rights [K,Q,k,q] with the project's Castling bit convention: K=1,Q=2,k=4,q=8.
  const unsigned rights = b.castling().rights;
  x[static_cast<std::size_t>(off + 0)] = (rights & 1u) ? 1.0f : 0.0f;
  x[static_cast<std::size_t>(off + 1)] = (rights & 2u) ? 1.0f : 0.0f;
  x[static_cast<std::size_t>(off + 2)] = (rights & 4u) ? 1.0f : 0.0f;
  x[static_cast<std::size_t>(off + 3)] = (rights & 8u) ? 1.0f : 0.0f;
  off += 4;

  // En-passant file one-hot [a..h]. If no EP square, all zeros.
  const Square ep = b.ep_square();
  if (ep >= 0 && ep < 64) {
    const int f = file_of(ep);
    if (f >= 0 && f < 8) {
      x[static_cast<std::size_t>(off + f)] = 1.0f;
    }
  }

  return x;
}

} // namespace euclid
