#include <cassert>
#include <cstdint>
#include <iostream>

#include "euclid/board.hpp"
#include "euclid/fen.hpp"

namespace euclid {
// Implemented in src/search.cpp; not part of the public header API.
std::uint64_t search_eval_cache_probes();
std::uint64_t search_eval_cache_hits();
void search_eval_cache_clear();
int search_debug_eval_stm(const Board& b);
}

int main() {
  using namespace euclid;

  Board b;
  set_from_fen(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

  search_eval_cache_clear();

  const std::uint64_t hits0 = search_eval_cache_hits();
  (void)search_debug_eval_stm(b); // should miss after clear
  const std::uint64_t hits1 = search_eval_cache_hits();
  assert(hits1 == hits0);

  (void)search_debug_eval_stm(b); // should hit
  const std::uint64_t hits2 = search_eval_cache_hits();
  assert(hits2 >= hits1 + 1);

  assert(search_eval_cache_probes() >= 2);

  std::cout << "eval_cache_smoke ok (hits " << hits2 << ", probes " << search_eval_cache_probes() << ")\n";
  return 0;
}
