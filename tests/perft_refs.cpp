#include <cassert>
#include <cstdint>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"

using namespace euclid;

static void check(const char* fen, int d, std::uint64_t expect) {
  Board b; set_from_fen(b, fen);
  auto got = perft(b, d);
  assert(got == expect);
}

int main() {
  // Start position (use STARTPOS_FEN directly; no `.data()`).
  check(STARTPOS_FEN, 1, 20ULL);
  check(STARTPOS_FEN, 2, 400ULL);
  check(STARTPOS_FEN, 3, 8902ULL);
  check(STARTPOS_FEN, 4, 197281ULL);
  // Depth 5 is 4'865'609 (skipped to keep CI fast)

  // Kiwipete
  static const char KP[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  check(KP, 1, 48ULL);
  check(KP, 2, 2039ULL);
  check(KP, 3, 97862ULL);
  check(KP, 4, 4085603ULL);
  // Depth 5 is 193'690'690 (also skipped)

  return 0;
}
