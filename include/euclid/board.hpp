#pragma once
#include <array>
#include <cstdint>
#include "euclid/types.hpp"
#include "euclid/zobrist.hpp"


namespace euclid {


struct Castling { // bit 0..3 = KQkq
unsigned rights = 0; // K=1, Q=2, k=4, q=8
};


class Board {
public:
Board();
void clear();


// very small API for now (we'll add FEN I/O next step)
void set_piece(Color c, Piece p, Square s);
void remove_piece(Color c, Piece p, Square s);


void set_side_to_move(Color c) { stm_ = c; recompute_hash_(); }
Color side_to_move() const { return stm_; }


void set_castling(Castling c) { castling_ = c; recompute_hash_(); }
Castling castling() const { return castling_; }


void set_ep_square(Square s) { ep_square_ = s; recompute_hash_(); }
Square ep_square() const { return ep_square_; }


U64 hash() const { return hash_; }


private:
// bitboards[color][piece]
std::array<std::array<U64, PIECE_N>, COLOR_N> bb_{};
Color stm_ = Color::White;
Castling castling_{};
Square ep_square_ = -1; // -1 = none
U64 hash_ = 0ULL;


void recompute_hash_();
};


} // namespace euclid