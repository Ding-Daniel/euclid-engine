#pragma once

#include <array>
#include <cstdint>

#include "euclid/types.hpp"
#include "euclid/zobrist.hpp"

namespace euclid {

struct Castling { // bit 0..3 = KQkq
  unsigned rights = 0; // K=1, Q=2, k=4, q=8
};

// Engine-wide Zobrist table accessor (defined in src/board.cpp).
const Zobrist& zobrist();

class Board {
public:
  Board();
  void clear();

  // piece ops
  void set_piece(Color c, Piece p, Square s);
  void remove_piece(Color c, Piece p, Square s);

  // side to move
  void set_side_to_move(Color c) {
    if (stm_ == c) return;
    stm_ = c;
    // toggle side-to-move key (convention: XOR when Black to move)
    hash_ ^= zobrist().side_to_move;
  }
  Color side_to_move() const { return stm_; }

  // castling rights
  void set_castling(Castling c) {
    const unsigned old = castling_.rights;
    const unsigned neu = c.rights;

    if (old == neu) {
      castling_ = c;
      return;
    }

    const unsigned delta = old ^ neu;
    const auto& Z = zobrist();
    if (delta & 0x1u) hash_ ^= Z.castling[0]; // K
    if (delta & 0x2u) hash_ ^= Z.castling[1]; // Q
    if (delta & 0x4u) hash_ ^= Z.castling[2]; // k
    if (delta & 0x8u) hash_ ^= Z.castling[3]; // q

    castling_ = c;
  }
  Castling castling() const { return castling_; }

  // ep square (-1 if none)
  void set_ep_square(Square s) {
    if (ep_square_ == s) return;

    const auto& Z = zobrist();

    if (ep_square_ != -1) {
      hash_ ^= Z.ep_file[static_cast<std::size_t>(file_of(ep_square_))];
    }

    ep_square_ = s;

    if (ep_square_ != -1) {
      hash_ ^= Z.ep_file[static_cast<std::size_t>(file_of(ep_square_))];
    }
  }
  Square ep_square() const { return ep_square_; }

  // zobrist hash
  U64 hash() const { return hash_; }

  // query
  Piece piece_at(Square s, Color* c_out = nullptr) const;

  // clocks
  void set_halfmove_clock(int h) { halfmove_clock_ = h; }
  int  halfmove_clock() const { return halfmove_clock_; }

  void set_fullmove_number(int n) { fullmove_number_ = n; }
  int  fullmove_number() const { return fullmove_number_; }

private:
  // bitboards[color][piece]
  std::array<std::array<U64, PIECE_N>, COLOR_N> bb_{};
  Color stm_ = Color::White;
  Castling castling_{};
  Square ep_square_ = -1; // -1 = none
  U64 hash_ = 0ULL;
  int halfmove_clock_ = 0;
  int fullmove_number_ = 1;

  void recompute_hash_();
};

} // namespace euclid