#include <cassert>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

int main() {
  using namespace euclid;
  // White in check from Re2: legal escapes are Kxe2, Kd1, Kf1 => 3
  Board b;
  set_from_fen(b, "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1");
  assert(perft(b, 1) == 3ULL);
  return 0;
}
