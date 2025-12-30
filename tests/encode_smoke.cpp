#include <cassert>
#include <algorithm>

#include "euclid/board.hpp"
#include "euclid/encode.hpp"
#include "euclid/fen.hpp"

using namespace euclid;

static int sq(int file, int rank) { return rank * 8 + file; }

static int count_ones(const std::vector<float>& v, int start, int len) {
  int c = 0;
  for (int i = 0; i < len; ++i) {
    const float x = v[static_cast<std::size_t>(start + i)];
    if (x == 1.0f) ++c;
    else assert(x == 0.0f);
  }
  return c;
}

int main() {
  // --- Startpos checks ---
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);

    const auto x = encode_12x64(b);
    assert(static_cast<int>(x.size()) == EncodedInputSpec::kTotal);

    // Piece planes have exactly 32 ones at start position.
    const int pieces = count_ones(x, 0, EncodedInputSpec::kPieceFeatures);
    assert(pieces == 32);

    // White pawns plane = plane 0, rank 2 squares A2..H2 => 8..15
    int wp = 0;
    for (int f = 0; f < 8; ++f) {
      const int idx = 0 * 64 + sq(f, 1);
      assert(x[static_cast<std::size_t>(idx)] == 1.0f);
      ++wp;
    }
    assert(wp == 8);

    // Black pawns plane = plane 6, rank 7 squares A7..H7 => 48..55
    int bp = 0;
    for (int f = 0; f < 8; ++f) {
      const int idx = 6 * 64 + sq(f, 6);
      assert(x[static_cast<std::size_t>(idx)] == 1.0f);
      ++bp;
    }
    assert(bp == 8);

    // Side to move = White at startpos.
    const int stmOff = EncodedInputSpec::kPieceFeatures;
    assert(x[static_cast<std::size_t>(stmOff)] == 1.0f);

    // Castling rights KQkq all present in startpos.
    const int castOff = stmOff + 1;
    assert(x[static_cast<std::size_t>(castOff + 0)] == 1.0f); // K
    assert(x[static_cast<std::size_t>(castOff + 1)] == 1.0f); // Q
    assert(x[static_cast<std::size_t>(castOff + 2)] == 1.0f); // k
    assert(x[static_cast<std::size_t>(castOff + 3)] == 1.0f); // q

    // No en-passant target in startpos.
    const int epOff = castOff + 4;
    const int epOnes = count_ones(x, epOff, 8);
    assert(epOnes == 0);
  }

  // --- En-passant file one-hot + side-to-move ---
  {
    // After 1. e4, FEN typically has ep square e3 (file = 4) and black to move.
    Board b;
    set_from_fen(b, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

    const auto x = encode_12x64(b);

    const int stmOff = EncodedInputSpec::kPieceFeatures;
    assert(x[static_cast<std::size_t>(stmOff)] == 0.0f); // black to move => 0

    const int epOff = stmOff + 1 + 4;
    for (int i = 0; i < 8; ++i) {
      const float want = (i == 4) ? 1.0f : 0.0f; // e-file
      assert(x[static_cast<std::size_t>(epOff + i)] == want);
    }
  }

  // --- Castling rights bit check ---
  {
    Board b;
    set_from_fen(b, "4k3/8/8/8/8/8/8/4K2R w K - 0 1"); // only White king-side

    const auto x = encode_12x64(b);

    const int castOff = EncodedInputSpec::kPieceFeatures + 1;
    assert(x[static_cast<std::size_t>(castOff + 0)] == 1.0f); // K
    assert(x[static_cast<std::size_t>(castOff + 1)] == 0.0f); // Q
    assert(x[static_cast<std::size_t>(castOff + 2)] == 0.0f); // k
    assert(x[static_cast<std::size_t>(castOff + 3)] == 0.0f); // q
  }

  return 0;
}
