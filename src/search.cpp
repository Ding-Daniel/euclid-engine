#include "euclid/search.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/eval.hpp"
#include <algorithm>
#include <limits>
#include <vector>

namespace euclid {

static constexpr int INF  = 30000;
static constexpr int MATE = 29000;

static inline int eval_side_to_move(const Board& b) {
  const int e = evaluate(b);
  return b.side_to_move() == Color::White ? e : -e;
}

static inline bool is_capture(const Board& b, Color us, const Move& m) {
  Color oc; Piece op = b.piece_at(m.to, &oc);
  return (op != Piece::None) && (oc != us);
}

static int negamax(Board& b, int depth, int alpha, int beta,
                   std::uint64_t& nodes, std::vector<Move>& pv) {
  nodes++;

  if (depth == 0) {
    pv.clear();
    return eval_side_to_move(b);
  }

  MoveList ml;
  generate_pseudo_legal(b, ml);

  const Color us = b.side_to_move();
  bool anyLegal = false;
  Move bestMove{};
  std::vector<Move> bestChildPV;
  int bestScore = -INF;

  auto try_move = [&](const Move& m) {
    State st{};
    do_move(b, m, st);
    if (!in_check(b, us)) {
      anyLegal = true;
      std::vector<Move> childPV;
      int score = -negamax(b, depth - 1, -beta, -alpha, nodes, childPV);
      if (score > bestScore) {
        bestScore   = score;
        bestMove    = m;
        bestChildPV = std::move(childPV);
      }
      if (score > alpha) alpha = score;
      undo_move(b, m, st);
      if (alpha >= beta) return true; // cutoff
    } else {
      undo_move(b, m, st);
    }
    return false;
  };

  // Simple ordering: captures first, then quiets (doesn't rely on Move.flag)
  for (const auto& m : ml) {
    if (is_capture(b, us, m)) {
      if (try_move(m)) break;
    }
  }
  if (alpha < beta) {
    for (const auto& m : ml) {
      if (!is_capture(b, us, m)) {
        if (try_move(m)) break;
      }
    }
  }

  if (!anyLegal) {
    if (in_check(b, us)) return -MATE + 1; // checkmated
    return 0;                               // stalemate
  }

  pv.clear();
  pv.push_back(bestMove);
  pv.insert(pv.end(), bestChildPV.begin(), bestChildPV.end());
  return bestScore;
}

SearchResult search(const Board& root, int depth) {
  SearchResult res;
  res.depth = depth;

  Board b = root;
  std::vector<Move> pv;
  int alpha = -INF, beta = INF;
  res.score = negamax(b, depth, alpha, beta, res.nodes, pv);
  if (!pv.empty()) res.best = pv.front();
  res.pv = std::move(pv);
  return res;
}

} // namespace euclid
