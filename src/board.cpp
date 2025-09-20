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
bb_[(int)c][(int)p] |= (1ULL << s);
recompute_hash_();
}


void Board::remove_piece(Color c, Piece p, Square s) {
bb_[(int)c][(int)p] &= ~(1ULL << s);
recompute_hash_();
}


void Board::recompute_hash_() {
const auto& Z = Zobrist::instance();
U64 h = 0ULL;
for (int c = 0; c < COLOR_N; ++c)
for (int p = 0; p < PIECE_N; ++p) {
U64 bb = bb_[c][p];
while (bb) {
int s = __builtin_ctzll(bb);
h ^= Z.piece_on[c][p][s];
bb &= bb - 1;
}
}
if (stm_ == Color::Black) h ^= Z.side_to_move;
if (castling_.rights & 0x1) h ^= Z.castling[0];
if (castling_.rights & 0x2) h ^= Z.castling[1];
if (castling_.rights & 0x4) h ^= Z.castling[2];
if (castling_.rights & 0x8) h ^= Z.castling[3];
if (ep_square_ >= 0) h ^= Z.ep_file[file_of(ep_square_)];
hash_ = h;
}


} // namespace euclid