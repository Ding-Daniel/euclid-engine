#include <cassert>
#include <iostream>

#include "euclid/attack.hpp"
#include "euclid/fen.hpp"
#include "euclid/game.hpp"
#include "euclid/move_do.hpp"

int main() {
  using namespace euclid;

  Board b;
  set_from_fen(b, STARTPOS_FEN);

  SearchLimits lim{};
  lim.depth = 2;       // deterministic and fast
  lim.nodes = 0;

  const int maxPlies = 30;

  GameResult g = selfplay(b, maxPlies, lim);

  // Must at least produce some moves and be internally consistent.
  assert(!g.moves.empty());
  assert(g.plies == static_cast<int>(g.moves.size()));
  assert(g.plies <= maxPlies);

  // Re-validate legality by replaying the game.
  Board r;
  set_from_fen(r, STARTPOS_FEN);

  for (const auto& m : g.moves) {
    const Color us = r.side_to_move();
    State st{};
    do_move(r, m, st);
    assert(!in_check(r, us));
  }

  std::cout << "selfplay_smoke ok plies " << g.plies << "\n";
  return 0;
}
