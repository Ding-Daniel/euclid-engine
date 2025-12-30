#include <cassert>
#include <iostream>
#include <vector>

#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/search.hpp"
#include "euclid/draw.hpp"

int main() {
  using namespace euclid;

  // 1) Insufficient material: K+B vs K should be treated as draw (score 0)
  {
    Board b;
    set_from_fen(b, "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1");
    assert(is_insufficient_material(b));

    SearchResult r = search(b, 4);
    assert(r.score == 0);
  }

  // 2) 50-move rule: halfmove clock >= 100 => draw (score 0), even if material advantage exists
  {
    Board b;
    set_from_fen(b, "7k/8/8/8/8/8/8/R3K3 w - - 100 1");
    assert(is_fifty_move_draw(b));

    SearchResult r = search(b, 4);
    assert(r.score == 0);
  }

  // 3) Threefold repetition utility: synthetic history
  {
    std::vector<U64> hist = {1, 2, 1, 2, 1};
    assert(is_threefold_repetition(hist));

    std::vector<U64> hist2 = {5, 6, 5, 6};
    assert(!is_threefold_repetition(hist2));
  }

  std::cout << "draw_rules_smoke ok\n";
  return 0;
}
