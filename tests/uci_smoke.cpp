#include <cassert>
#include <vector>
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"
#include "euclid/uci.hpp"
#include "euclid/movegen.hpp"   // <-- brings in Move for the vector pair
#include "euclid/board.hpp"

using namespace euclid;

int main() {
  // Round-trip UCI on startpos
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);
    Move m = uci_to_move(b, "e2e4");
    std::string u = move_to_uci(m);
    assert(u == "e2e4");
  }
  // Promotion parse on a realistic one-move promotion
  {
    Board b;
    set_from_fen(b, "7k/P7/8/8/8/8/8/6K1 w - - 0 1"); // white pawn on a7
    Move m = uci_to_move(b, "a7a8q");
    (void)m;
  }
  // Divide has per-child totals that sum to perft
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);
    std::vector<std::pair<Move, std::uint64_t>> items;
    perft_divide(b, 2, items);
    std::uint64_t sum = 0;
    for (auto& kv : items) sum += kv.second;
    assert(sum == perft(b, 2));
  }
  return 0;
}
