#pragma once

#include <vector>

#include "euclid/board.hpp"
#include "euclid/types.hpp"

namespace euclid {

// 50-move rule (claim draw at 100 halfmoves)
inline bool is_fifty_move_draw(const Board& b) {
  return b.halfmove_clock() >= 100;
}

// Threefold repetition: history.back() is the current position key.
// Return true if this key occurs at least 3 times in the history stack.
inline bool is_threefold_repetition(const std::vector<U64>& history) {
  if (history.empty()) return false;
  const U64 key = history.back();
  int count = 0;
  for (U64 k : history) {
    if (k == key) ++count;
  }
  return count >= 3;
}

// “Insufficient material” (early, standard practical set):
// - K vs K
// - K+N vs K
// - K+B vs K
// - K+B vs K+B with bishops on same-colored squares
// - K+NN vs K
inline bool is_insufficient_material(const Board& b) {
  struct Counts {
    int pawns = 0;
    int knights = 0;
    int bishops = 0;
    int rooks = 0;
    int queens = 0;
    int bishop_sq[2] = {-1, -1}; // keep up to 2 bishop squares
    int bishop_seen = 0;
  };

  Counts cnt[2]; // [White, Black]

  for (Square s = 0; s < 64; ++s) {
    Color pc{};
    Piece p = b.piece_at(s, &pc);
    if (p == Piece::None) continue;
    if (p == Piece::King) continue;

    Counts& c = cnt[static_cast<int>(pc)];
    switch (p) {
      case Piece::Pawn:   ++c.pawns; break;
      case Piece::Knight: ++c.knights; break;
      case Piece::Bishop:
        ++c.bishops;
        if (c.bishop_seen < 2) c.bishop_sq[c.bishop_seen] = s;
        ++c.bishop_seen;
        break;
      case Piece::Rook:   ++c.rooks; break;
      case Piece::Queen:  ++c.queens; break;
      default: break;
    }
  }

  // Any pawns/rooks/queens => do not auto-draw by insufficient material.
  for (int side = 0; side < 2; ++side) {
    if (cnt[side].pawns != 0)  return false;
    if (cnt[side].rooks != 0)  return false;
    if (cnt[side].queens != 0) return false;
  }

  const int wMinor = cnt[0].knights + cnt[0].bishops;
  const int bMinor = cnt[1].knights + cnt[1].bishops;

  // K vs K
  if (wMinor == 0 && bMinor == 0) return true;

  // K + (B or N) vs K
  if (wMinor == 1 && bMinor == 0) return true;
  if (wMinor == 0 && bMinor == 1) return true;

  // K + NN vs K
  if (cnt[0].knights == 2 && cnt[0].bishops == 0 && bMinor == 0) return true;
  if (cnt[1].knights == 2 && cnt[1].bishops == 0 && wMinor == 0) return true;

  // K+B vs K+B (same-color bishops)
  if (cnt[0].bishops == 1 && cnt[0].knights == 0 &&
      cnt[1].bishops == 1 && cnt[1].knights == 0) {
    const int ws = cnt[0].bishop_sq[0];
    const int bs = cnt[1].bishop_sq[0];
    if (ws >= 0 && bs >= 0) {
      const int wColor = (file_of(ws) + rank_of(ws)) & 1;
      const int bColor = (file_of(bs) + rank_of(bs)) & 1;
      if (wColor == bColor) return true;
    }
  }

  return false;
}

inline bool is_rule_draw(const Board& b, const std::vector<U64>& history) {
  return is_fifty_move_draw(b) ||
         is_threefold_repetition(history) ||
         is_insufficient_material(b);
}

} // namespace euclid
