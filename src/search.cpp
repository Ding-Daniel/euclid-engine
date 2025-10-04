#include "euclid/search.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/eval.hpp"
#include "euclid/tt.hpp"
#include "euclid/types.hpp"
#include <algorithm>
#include <limits>
#include <vector>

namespace euclid {

static constexpr int INF  = 30000;
static constexpr int MATE = 29000;

// Piece values for ordering (None,P,N,B,R,Q,K)
static constexpr int PVAL[7] = { 0, 100, 320, 330, 500, 900, 20000 };

static inline int eval_side_to_move(const Board& b) {
  const int e = evaluate(b);
  return b.side_to_move() == Color::White ? e : -e;
}

static inline bool move_eq(const Move& a, const Move& b) {
  return a.from == b.from && a.to == b.to && a.promo == b.promo;
}

static inline bool is_en_passant(const Board& b, Color us, const Move& m) {
  Color fc; Piece fromP = b.piece_at(m.from, &fc);
  if (fromP != Piece::Pawn || fc != us) return false;
  if (file_of(m.from) == file_of(m.to)) return false;
  Color tc; Piece toP = b.piece_at(m.to, &tc);
  if (toP != Piece::None) return false;
  return b.ep_square() == m.to;
}

// MVV-LVA + promo bonus
static int move_score(const Board& b, Color us, const Move& m) {
  Color fc; Piece fromP = b.piece_at(m.from, &fc);
  Color tc; Piece toP   = b.piece_at(m.to,   &tc);

  int promo_bonus = 0;
  if (m.promo != Piece::None)
    promo_bonus = PVAL[static_cast<int>(m.promo)] * 10;

  if (toP != Piece::None && tc != us) {
    int victim   = PVAL[static_cast<int>(toP)];
    int attacker = PVAL[static_cast<int>(fromP)];
    return 100000 + 10*victim - attacker + promo_bonus;
  }
  if (is_en_passant(b, us, m)) {
    int attacker = PVAL[static_cast<int>(fromP)];
    return 100000 + 10*PVAL[static_cast<int>(Piece::Pawn)] - attacker + promo_bonus;
  }
  return promo_bonus; // quiet
}

// Global-ish TT (simple and effective)
static TT GTT;

static int negamax(Board& b, int depth, int alpha, int beta,
                   std::uint64_t& nodes, std::vector<Move>& pv) {
  nodes++;

  const int alphaOrig = alpha;
  const U64 key = b.hash();

  // TT probe
  TTEntry hit{};
  if (GTT.probe(key, hit) && hit.depth >= depth) {
    if (hit.bound == TTBound::Exact) {
      pv.clear();
      return hit.score;
    } else if (hit.bound == TTBound::Lower && hit.score >= beta) {
      pv.clear();
      return hit.score;
    } else if (hit.bound == TTBound::Upper && hit.score <= alpha) {
      pv.clear();
      return hit.score;
    }
  }

  if (depth == 0) {
    pv.clear();
    return eval_side_to_move(b);
  }

  MoveList ml;
  generate_pseudo_legal(b, ml);

  const Color us = b.side_to_move();

  // Build & order move list
  std::vector<Move> moves;
  moves.reserve(ml.sz);
  for (const auto& m : ml) moves.push_back(m);

  // TT move bonus if we have one
  Move ttMove{};
  if (hit.key == key && hit.depth >= 0) ttMove = hit.best;

  std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& bmv) {
    int sa = move_score(b, us, a);
    int sb = move_score(b, us, bmv);
    if (move_eq(a, ttMove))  sa += 1'000'000;
    if (move_eq(bmv, ttMove)) sb += 1'000'000;
    return sa > sb;
  });

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

  for (const auto& m : moves) {
    if (try_move(m)) break;
  }

  if (!anyLegal) {
    if (in_check(b, us)) return -MATE + 1;
    return 0;
  }

  // Store PV
  pv.clear();
  pv.push_back(bestMove);
  pv.insert(pv.end(), bestChildPV.begin(), bestChildPV.end());

  // Store to TT
  TTBound bound = TTBound::Exact;
  if (bestScore <= alphaOrig) bound = TTBound::Upper;
  else if (bestScore >= beta) bound = TTBound::Lower;
  GTT.store(key, bestMove, static_cast<std::int16_t>(depth),
            static_cast<std::int16_t>(bestScore), bound);

  return bestScore;
}

SearchResult search(const Board& root, int maxDepth) {
  SearchResult res;
  res.depth = maxDepth;
  res.nodes = 0;

  Board b = root;
  int alpha = -INF, beta = INF;

  for (int d = 1; d <= maxDepth; ++d) {
    std::vector<Move> pv;
    int score = negamax(b, d, alpha, beta, res.nodes, pv);

    if (!pv.empty()) {
      res.best  = pv.front();
      res.pv    = std::move(pv);
      res.score = score;
      res.depth = d;
    } else {
      res.best = Move{};
      res.pv.clear();
      res.score = score;
      res.depth = d;
      break;
    }
  }
  return res;
}

} // namespace euclid
