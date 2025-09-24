#pragma once
#include <cstdint>
#include "euclid/types.hpp"


namespace euclid {


enum class MoveFlag : uint8_t {
Quiet = 0,
Capture = 1 << 0,
DoublePush = 1 << 1,
EnPassant = 1 << 2,
Castle = 1 << 3,
PromoKnight = 1 << 4,
PromoBishop = 1 << 5,
PromoRook = 1 << 6,
PromoQueen = 1 << 7,
};


struct Move {
Square from{0};
Square to{0};
MoveFlag flags{MoveFlag::Quiet};
Piece promo{Piece::None};
};


} // namespace euclid