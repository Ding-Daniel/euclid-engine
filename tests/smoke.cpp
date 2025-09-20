#include <cassert>
#include "euclid/board.hpp"


int main() {
using namespace euclid;
Board a, b;
// Same init yields same hash
assert(a.hash() == b.hash());
a.set_piece(Color::White, Piece::Pawn, 12);
assert(a.hash() != b.hash());
return 0;
}