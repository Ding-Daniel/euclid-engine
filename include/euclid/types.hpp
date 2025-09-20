#pragma once
#include <cstdint>


namespace euclid {


using U64 = std::uint64_t;
using Square = int; // 0..63


enum class Color : int { White = 0, Black = 1 };


enum class Piece : int { Pawn=0, Knight=1, Bishop=2, Rook=3, Queen=4, King=5, None=6 };


constexpr int COLOR_N = 2;
constexpr int PIECE_N = 6; // without None


inline constexpr int file_of(Square s) { return s & 7; }
inline constexpr int rank_of(Square s) { return s >> 3; }


} // namespace euclid