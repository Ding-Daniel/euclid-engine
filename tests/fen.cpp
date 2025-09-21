#include <cassert>
#include <string>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"


int main() {
using namespace euclid;


// Round-trip startpos
Board b1;
set_from_fen(b1, STARTPOS_FEN);
assert(to_fen(b1) == STARTPOS_FEN);


// A position with castling rights only on white and an EP square
Board b2;
set_from_fen(b2, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w K - 0 1");
assert(to_fen(b2) == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w K - 0 1");


return 0;
}