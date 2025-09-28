#include "euclid/perft.hpp"
#include "euclid/movegen.hpp"
#include "euclid/attack.hpp"
#include "euclid/move_do.hpp"

namespace euclid {

static std::uint64_t perft_mut(Board& b, int depth) {
  if (depth == 0) return 1ULL;

  MoveList ml;
  generate_pseudo_legal(b, ml);

  const Color us = b.side_to_move();
  std::uint64_t nodes = 0ULL;

  for (const auto& m : ml) {
    State st{};
    do_move(b, m, st);
    if (!in_check(b, us)) {               // legal only if our king not in check after move
      nodes += perft_mut(b, depth - 1);
    }
    undo_move(b, m, st);
  }
  return nodes;
}

std::uint64_t perft(const Board& b, int depth) {
  Board copy = b;                          // copy once at root
  return perft_mut(copy, depth);
}

} // namespace euclid
