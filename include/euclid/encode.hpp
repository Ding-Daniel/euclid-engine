#pragma once

#include <vector>

#include "euclid/board.hpp"

namespace euclid {

// Encoder layout for Phase 11 (roadmap):
// - 12 * 64 binary planes: [WP,WN,WB,WR,WQ,WK, BP,BN,BB,BR,BQ,BK] each plane is A1..H8 (Square 0..63)
// - 1 float: side to move (1.0 = White, 0.0 = Black)
// - 4 floats: castling rights [K,Q,k,q] (1.0 if available else 0.0)
// - 8 floats: en-passant file one-hot [a..h] (all zeros if no EP square)
//
// Total = 12*64 + 1 + 4 + 8 = 781 floats.
struct EncodedInputSpec {
  static constexpr int kSquares = 64;
  static constexpr int kPiecePlanes = 12;
  static constexpr int kPieceFeatures = kPiecePlanes * kSquares; // 768

  static constexpr int kSideToMove = 1;
  static constexpr int kCastling = 4;
  static constexpr int kEpFile = 8;

  static constexpr int kTotal = kPieceFeatures + kSideToMove + kCastling + kEpFile; // 781
};

// Returns a dense float vector of length EncodedInputSpec::kTotal following the layout above.
std::vector<float> encode_12x64(const Board& b);

} // namespace euclid
