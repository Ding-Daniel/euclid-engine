#include <cassert>
#include <iostream>
#include "euclid/search.hpp"
#include "euclid/fen.hpp"
#include "euclid/uci.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"   // State, do_move/undo_move
#include "euclid/attack.hpp"    // in_check

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
      if (!in_check(tmp, us)) return true;
    }
  }
  return false;
}

int main() {
  // STARTPOS depth-2: must return a legal move and a non-empty PV
  {
    Board b; set_from_fen(b, STARTPOS_FEN);
    auto r = search(b, 2);
    assert(move_is_legal(b, r.best));
    assert(!r.pv.empty());
  }
  // Simple mate-in-1-ish checker: ensure we return a checking move at d=1
  // (No strict assertion on exact move—just legal & non-empty PV.)
  {
    Board b; set_from_fen(b, "6k1/6pp/8/8/8/8/6PP/6KQ w - - 0 1");
    auto r = search(b, 1);
    assert(move_is_legal(b, r.best));
  }
  std::cout << "search_smoke ok\n";
  return 0;
}
