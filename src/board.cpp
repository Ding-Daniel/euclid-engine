#include "euclid/board.hpp"
#include <cassert>


namespace euclid {


Board::Board() { clear(); }


void Board::clear() {
for (auto& by_color : bb_) for (auto& b : by_color) b = 0ULL;
stm_ = Color::White;
castling_ = {};
ep_square_ = -1;
Zobrist::instance().init();
recompute_hash_();
}


void Board::set_piece(Color c, Piece p, Square s) {
bb_[static_cast<std::size_t>(c)][static_cast<std::size_t>(p)] |= (1ULL << s);
recompute_hash_();
}


void Board::remove_piece(Color c, Piece p, Square s) {
bb_[static_cast<std::size_t>(c)][static_cast<std::size_t>(p)] &= ~(1ULL << s);
recompute_hash_();
}


void Board::recompute_hash_() {
const auto& Z = Zobrist::instance();
U64 h = 0ULL;
for (int c = 0; c < COLOR_N; ++c)
for (int p = 0; p < PIECE_N; ++p) {
U64 bb = bb_[static_cast<std::size_t>(c)][static_cast<std::size_t>(p)];
while (bb) {
int s = __builtin_ctzll(bb);
h ^= Z.piece_on[static_cast<std::size_t>(c)]
[static_cast<std::size_t>(p)]
[static_cast<std::size_t>(s)];
bb &= bb - 1;
}
}
if (stm_ == Color::Black) h ^= Z.side_to_move;
if (castling_.rights & 0x1) h ^= Z.castling[0];
if (castling_.rights & 0x2) h ^= Z.castling[1];
if (castling_.rights & 0x4) h ^= Z.castling[2];
if (castling_.rights & 0x8) h ^= Z.castling[3];
if (ep_square_ >= 0) h ^= Z.ep_file[static_cast<std::size_t>(file_of(ep_square_))];
hash_ = h;
}
Piece Board::piece_at(Square s, Color* c_out) const {
  for (int c = 0; c < COLOR_N; ++c) {
    for (int p = 0; p < PIECE_N; ++p) {
      if ((bb_[static_cast<std::size_t>(c)][static_cast<std::size_t>(p)] >> s) & 1ULL) {
        if (c_out) *c_out = static_cast<Color>(c);
        return static_cast<Piece>(p);
      }
    }
  }
  return Piece::None;
}


} // namespace euclid