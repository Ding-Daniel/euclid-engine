#include "euclid/perft.hpp"
#include "euclid/movegen.hpp"
#include "euclid/attack.hpp"
#include "euclid/move_do.hpp"
#include <vector>

namespace euclid {

static std::uint64_t perft_mut(Board& b, int depth) {
  if (depth == 0) return 1ULL;

  MoveList ml;
  generate_pseudo_legal(b, ml);

  const Color us = b.side_to_move();
  std::uint64_t nodes = 0ULL;

  for (const auto& m : ml) {
    State st{};                // <-- match your existing type
    do_move(b, m, st);
    if (!in_check(b, us)) {    // legal only if our king not in check after move
      nodes += perft_mut(b, depth - 1);
    }
    undo_move(b, m, st);
  }
  return nodes;
}

std::uint64_t perft(const Board& b, int depth) {
  Board copy = b;              // copy once at root
  return perft_mut(copy, depth);
}

// New: per-move breakdown at root
void perft_divide(const Board& b, int depth,
                  std::vector<std::pair<Move, std::uint64_t>>& out) {
  out.clear();
  if (depth <= 0) return;

  Board root = b;
  MoveList ml;
  generate_pseudo_legal(root, ml);
  const Color us = root.side_to_move();

  for (const auto& m : ml) {   // <-- same pattern as above
    State st{};
    do_move(root, m, st);
    if (!in_check(root, us)) {
      std::uint64_t n = perft(root, depth - 1);
      out.emplace_back(m, n);
    }
    undo_move(root, m, st);
  }
}

} // namespace euclid
