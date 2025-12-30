#include <cassert>
#include <atomic>

#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/search.hpp"

using namespace euclid;

int main() {
  Board b;
  set_from_fen(b, STARTPOS_FEN);

  // Node budget: should stop deterministically and not exceed the limit.
  std::atomic<bool> stop{false};

  SearchLimits lim{};
  lim.depth = 8;
  lim.nodes = 500;
  lim.stop  = &stop;

  SearchResult r = search(b, lim);

  assert(stop.load(std::memory_order_relaxed) == true);
  assert(r.nodes <= lim.nodes);

  // Pre-stopped: should return immediately with 0 nodes and depth 0.
  stop.store(true, std::memory_order_relaxed);
  SearchLimits lim2{};
  lim2.depth = 8;
  lim2.stop  = &stop;

  SearchResult r2 = search(b, lim2);
  assert(r2.nodes == 0);
  assert(r2.depth == 0);

  return 0;
}
