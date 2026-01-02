#include <cassert>
#include <iostream>

#include "euclid/board.hpp"
#include "euclid/eval.hpp"
#include "euclid/fen.hpp"

int main() {
  using namespace euclid;

  // 1) Startpos should be balanced.
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);
    int s = evaluate(b);
    assert(s == 0);
  }

  // 2) Side-to-move should not change evaluate() (side-to-move agnostic).
  {
    Board w, b;
    set_from_fen(w, "4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    set_from_fen(b, "4k3/8/8/8/8/8/8/4K3 b - - 0 1");
    assert(evaluate(w) == evaluate(b));
  }

  // 3) Simple material sanity: single pawn is +100 for White.
  {
    Board b;
    set_from_fen(b, "4k3/8/8/8/8/8/P7/4K3 w - - 0 1");
    int s = evaluate(b);
    assert(s == 100);
  }

  // 4) Extra White queen should be strongly positive.
  // (Illegal chess position is fine for eval sanity.)
  {
    Board b;
    set_from_fen(b, "rnbqkbnr/pppppppp/8/8/8/Q7/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int s = evaluate(b);
    assert(s >= 850);
  }

  // 5) Extra Black queen should be strongly negative.
  {
    Board b;
    set_from_fen(b, "rnbqkbnr/pppppppp/q7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int s = evaluate(b);
    assert(s <= -850);
  }

  std::cout << "eval_sanity_smoke ok\n";
  return 0;
}