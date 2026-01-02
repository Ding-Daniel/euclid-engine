#include "euclid/board.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace euclid {
namespace {

// Deterministic SplitMix64 for Zobrist initialization.
static inline std::uint64_t splitmix64_next(std::uint64_t& x) {
  x += 0x9e3779b97f4a7c15ULL;
  std::uint64_t z = x;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

static Zobrist make_zobrist_table() {
  Zobrist Z{};
  std::uint64_t seed = 0x1234abcd5678ef01ULL; // fixed seed => deterministic hashes

  for (std::size_t c = 0; c < COLOR_N; ++c) {
    for (std::size_t p = 0; p < PIECE_N; ++p) {
      for (std::size_t s = 0; s < 64; ++s) {
        Z.piece_on[c][p][s] = static_cast<U64>(splitmix64_next(seed));
      }
    }
  }

  for (std::size_t i = 0; i < 4; ++i) {
    Z.castling[i] = static_cast<U64>(splitmix64_next(seed));
  }

  for (std::size_t f = 0; f < 8; ++f) {
    Z.ep_file[f] = static_cast<U64>(splitmix64_next(seed));
  }

  Z.side_to_move = static_cast<U64>(splitmix64_next(seed));
  return Z;
}

} // namespace

const Zobrist& zobrist() {
  static const Zobrist Z = make_zobrist_table();
  return Z;
}

Board::Board() {
  clear();
}

void Board::clear() {
  for (std::size_t c = 0; c < COLOR_N; ++c) {
    for (std::size_t p = 0; p < PIECE_N; ++p) {
      bb_[c][p] = 0ULL;
    }
  }

  stm_ = Color::White;
  castling_.rights = 0;
  ep_square_ = -1;
  halfmove_clock_ = 0;
  fullmove_number_ = 1;
  hash_ = 0ULL;

  recompute_hash_();
}

void Board::set_piece(Color c, Piece p, Square s) {
  const std::size_t ci = static_cast<std::size_t>(c);
  const std::size_t pi = static_cast<std::size_t>(p);
  const U64 mask = 1ULL << static_cast<unsigned>(s);

#ifndef NDEBUG
  assert((bb_[ci][pi] & mask) == 0ULL);
#endif

  bb_[ci][pi] |= mask;
  hash_ ^= zobrist().piece_on[ci][pi][static_cast<std::size_t>(s)];
}

void Board::remove_piece(Color c, Piece p, Square s) {
  const std::size_t ci = static_cast<std::size_t>(c);
  const std::size_t pi = static_cast<std::size_t>(p);
  const U64 mask = 1ULL << static_cast<unsigned>(s);

#ifndef NDEBUG
  assert((bb_[ci][pi] & mask) != 0ULL);
#endif

  bb_[ci][pi] &= ~mask;
  hash_ ^= zobrist().piece_on[ci][pi][static_cast<std::size_t>(s)];
}

Piece Board::piece_at(Square s, Color* c_out) const {
  const U64 mask = 1ULL << static_cast<unsigned>(s);

  for (std::size_t c = 0; c < COLOR_N; ++c) {
    for (std::size_t p = 0; p < PIECE_N; ++p) {
      if (bb_[c][p] & mask) {
        if (c_out) *c_out = static_cast<Color>(c);
        return static_cast<Piece>(p);
      }
    }
  }

  if (c_out) *c_out = Color::White;
  return Piece::None;
}

void Board::recompute_hash_() {
  const auto& Z = zobrist();
  U64 h = 0ULL;

  // pieces
  for (std::size_t c = 0; c < COLOR_N; ++c) {
    for (std::size_t p = 0; p < PIECE_N; ++p) {
      U64 bb = bb_[c][p];
      while (bb) {
        const unsigned lsb = static_cast<unsigned>(__builtin_ctzll(bb));
        bb &= (bb - 1ULL);
        h ^= Z.piece_on[c][p][static_cast<std::size_t>(lsb)];
      }
    }
  }

  // castling (KQkq => indices 0..3)
  const unsigned r = castling_.rights;
  if (r & 0x1u) h ^= Z.castling[0];
  if (r & 0x2u) h ^= Z.castling[1];
  if (r & 0x4u) h ^= Z.castling[2];
  if (r & 0x8u) h ^= Z.castling[3];

  // ep file
  if (ep_square_ != -1) {
    h ^= Z.ep_file[static_cast<std::size_t>(file_of(ep_square_))];
  }

  // side to move (convention: XOR when Black to move)
  if (stm_ == Color::Black) {
    h ^= Z.side_to_move;
  }

  hash_ = h;
}

} // namespace euclid