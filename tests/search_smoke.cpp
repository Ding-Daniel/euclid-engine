#include <cassert>
#include <iostream>
#include <cstdint>
#include "euclid/search.hpp"
#include "euclid/fen.hpp"
#include "euclid/uci.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"   // State, do_move/undo_move
#include "euclid/attack.hpp"    // in_check

// Test-only hooks implemented in src/search.cpp (not part of the public header).
namespace euclid {
std::uint64_t search_eval_cache_hits();
std::uint64_t search_eval_cache_probes();
void search_eval_cache_clear();
int search_debug_eval_stm(const Board& b);
}

using namespace euclid;

static bool move_is_legal(const Board& b, const Move& m) {
  MoveList ml;
  generate_pseudo_legal(b, ml);
  const Color us = b.side_to_move();
  for (const auto& cand : ml) {
    if (cand.from == m.from && cand.to == m.to && cand.promo == m.promo) {
      State st{};
      Board tmp = b;
      do_move(tmp, cand, st);
      return !in_check(tmp, us);
    }
  }
  return false;
}

int main() {
  // Verify that the Zobrist-keyed eval cache is functional and stable.
  search_eval_cache_clear();
  {
    Board b; set_from_fen(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const std::uint64_t hits0 = search_eval_cache_hits();
    (void)search_debug_eval_stm(b);
    const std::uint64_t hits1 = search_eval_cache_hits();
    assert(hits1 == hits0); // first probe should be a miss after clear
    (void)search_debug_eval_stm(b);
    const std::uint64_t hits2 = search_eval_cache_hits();
    assert(hits2 >= hits1 + 1); // second probe should hit the cache
    assert(search_eval_cache_probes() >= 2);
  }

  Board b; set_from_fen(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

  SearchResult r = search(b, 4);
  assert((r.best.from | r.best.to | (int)r.best.promo) != 0);
  assert(move_is_legal(b, r.best));

  // Also run through UCI conversion to ensure it prints sanely
  std::cout << "best " << move_to_uci(r.best) << " score " << r.score << " depth " << r.depth
            << " nodes " << r.nodes << "\n";

  std::cout << "search_smoke ok\n";
  return 0;
}
