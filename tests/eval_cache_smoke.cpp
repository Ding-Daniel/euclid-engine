#include "euclid/board.hpp"
#include "euclid/search.hpp"
#include "euclid/types.hpp"

#include <cassert>
#include <iostream>

using namespace euclid;

static bool same_move(const Move& a, const Move& b) {
  return a.from == b.from && a.to == b.to && a.promo == b.promo;
}

int main() {
  // Deterministic legal position.
  // White: King e1, Queen d1
  // Black: King e8
  Board b;
  b.clear();
  b.set_piece(Color::White, Piece::King, 4);   // e1
  b.set_piece(Color::White, Piece::Queen, 3);  // d1
  b.set_piece(Color::Black, Piece::King, 60);  // e8
  b.set_side_to_move(Color::White);

  SearchLimits lim{};
  lim.depth = 4;

  const SearchResult r1 = search(b, lim);
  const SearchResult r2 = search(b, lim);

  // Cache must not change semantics.
  assert(r1.depth == r2.depth);
  assert(r1.score == r2.score);
  assert(same_move(r1.best, r2.best));

  std::cout << "eval_cache_smoke ok\n";
  return 0;
}
