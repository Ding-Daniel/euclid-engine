#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"


int main() {
using namespace euclid;
Board b;
set_from_fen(b, STARTPOS_FEN);


// Depth 0 is always 1 by definition
assert(perft(b, 0) == 1ULL);


// Depth 1 is 0 for now because generator is a stub; this will change later.
assert(perft(b, 1) == 0ULL);
return 0;
}