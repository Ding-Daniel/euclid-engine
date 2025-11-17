#include "euclid/search.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/eval.hpp"
#include "euclid/tt.hpp"
#include "euclid/types.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <vector>

namespace euclid {

// Forward declaration of the internal driver so IDEs don’t complain.
static SearchResult search_with_limits(const Board& root,
                                       int maxDepth,
                                       std::atomic<bool>* stopFlag);

namespace {

// ------------------------------------------------------------------
// Tunables and shared state
// ------------------------------------------------------------------
constexpr int MAX_PLY = 128;

static Move killer1[MAX_PLY];
static Move killer2[MAX_PLY];
static int  historyH[2][64][64];
static int  g_rootDepth = 0;

static constexpr int INF  = 30000;
static constexpr int MATE = 29000;

// Piece values for ordering (None,P,N,B,R,Q,K)
static constexpr int PVAL[7] = { 0, 100, 320, 330, 500, 900, 20000 };

// Global TT
static TT GTT;

// ------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------
inline bool same_move(const Move& a, const Move& b) {
  return a.from == b.from && a.to == b.to && a.promo == b.promo;
}

inline int eval_side_to_move(const Board& b) {
  const int e = evaluate(b);
  return b.side_to_move() == Color::White ? e : -e;
}

inline bool square_has_opponent(const Board& b, Color us, int sq) {
  Color oc; Piece op = b.piece_at(sq, &oc);
  return op != Piece::None && oc != us;
}

inline bool is_quiet(const Board& b, Color us, const Move& m) {
  if (m.promo != Piece::None) return false;
  return !square_has_opponent(b, us, m.to);
}

inline void store_killer_history(Color us, const Board& b, const Move& m, int ply) {
  if (ply < 0 || ply >= MAX_PLY) return;
  if (is_quiet(b, us, m)) {
    if (!same_move(killer1[ply], m)) { killer2[ply] = killer1[ply]; killer1[ply] = m; }
    int& h = historyH[(int)us][m.from][m.to];
    h += (ply + 1) * (ply + 1);
    if (h > (1 << 20)) { // cheap decay to avoid overflow
      for (int c = 0; c < 2; ++c)
        for (int f = 0; f < 64; ++f)
          for (int t = 0; t < 64; ++t)
            historyH[c][f][t] >>= 1;
    }
  }
}

inline int mvv_lva(const Board& b, const Move& m) {
  Color oc; Piece victim = b.piece_at(m.to, &oc);
  if (victim == Piece::None) return 0;
  Color uc; Piece attacker = b.piece_at(m.from, &uc);
  static const int val[] = {0,100,320,330,500,900,20000}; // None,P,N,B,R,Q,K
  return val[(int)victim]*16 - val[(int)attacker];
}

// Killer/history aware sorting score (captures, promo, TT first; then killers; then history)
inline int order_score(const Board& b, Color us, const Move& m, int ply, const Move& ttMove) {
  if (same_move(m, ttMove)) return 3'000'000;
  if (m.promo != Piece::None) return 2'600'000;                 // non-capture promotions
  if (square_has_opponent(b, us, m.to)) return 2'000'000 + mvv_lva(b, m);
  if (same_move(m, killer1[ply])) return 1'500'000;
  if (same_move(m, killer2[ply])) return 1'400'000;
  return historyH[(int)us][m.from][m.to];                        // quiets
}

// --- Minimal helpers for QS ordering (kept conservative) ---
static inline bool is_en_passant(const Board& b, Color us, const Move& m) {
  Color fc; Piece fromP = b.piece_at(m.from, &fc);
  if (fromP != Piece::Pawn || fc != us) return false;
  if (file_of(m.from) == file_of(m.to)) return false;
  Color tc; Piece toP = b.piece_at(m.to, &tc);
  if (toP != Piece::None) return false;
  return b.ep_square() == m.to;
}

static int move_score_basic(const Board& b, Color us, const Move& m) {
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

} // unnamed namespace

// ---------------------------
// Quiescence Search (conservative)
// ---------------------------
static int qsearch(Board& b, int alpha, int beta, std::uint64_t& nodes) {
  nodes++;

  const Color us = b.side_to_move();

  // If in check, allow all legal evasions (ordered)
  if (in_check(b, us)) {
    MoveList ev;
    generate_pseudo_legal(b, ev);

    std::vector<Move> moves;
    moves.reserve(ev.sz);
    for (const auto& m : ev) moves.push_back(m);
    std::stable_sort(moves.begin(), moves.end(),
      [&](const Move& a, const Move& c){ return move_score_basic(b, us, a) > move_score_basic(b, us, c); });

    for (const auto& m : moves) {
      State st{};
      do_move(b, m, st);
      if (!in_check(b, us)) {
        int score = -qsearch(b, -beta, -alpha, nodes);
        undo_move(b, m, st);
        if (score >= beta) return score;
        if (score > alpha) alpha = score;
      } else {
        undo_move(b, m, st);
      }
    }
    return alpha;
  }

  // Stand-pat
  int stand = eval_side_to_move(b);
  if (stand >= beta) return stand;
  if (stand > alpha) alpha = stand;

  // Only tactical moves
  MoveList ml;
  generate_pseudo_legal(b, ml);

  std::vector<Move> moves;
  moves.reserve(ml.sz);
  for (const auto& m : ml) {
    Color tc; Piece toP = b.piece_at(m.to, &tc);
    bool isCap = (toP != Piece::None && tc != us);
    bool isEP  = is_en_passant(b, us, m);
    bool isPr  = (m.promo != Piece::None);
    if (isCap || isEP || isPr) moves.push_back(m);
  }

  std::stable_sort(moves.begin(), moves.end(),
    [&](const Move& a, const Move& c){ return move_score_basic(b, us, a) > move_score_basic(b, us, c); });

  for (const auto& m : moves) {
    State st{};
    do_move(b, m, st);
    if (!in_check(b, us)) {
      int score = -qsearch(b, -beta, -alpha, nodes);
      undo_move(b, m, st);
      if (score >= beta) return score;
      if (score > alpha) alpha = score;
    } else {
      undo_move(b, m, st);
    }
  }

  return alpha;
}

// ---------------------------
// Alpha-Beta + TT + killers/history (+ safe check extension)
// ---------------------------
static int negamax(Board& b,
                   int depth,
                   int alpha,
                   int beta,
                   std::uint64_t& nodes,
                   std::vector<Move>& pv,
                   std::atomic<bool>* stopFlag)
{
  nodes++;

  // Cooperative stop (from UCI "stop")
  if (stopFlag && stopFlag->load(std::memory_order_relaxed)) {
    pv.clear();
    return alpha; // neutral return
  }

  const int alphaOrig = alpha;
  const U64 key = b.hash();
  const Color us = b.side_to_move();
  const int ply = std::max(0, g_rootDepth - depth);

  // TT probe
  TTEntry hit{};
  Move ttMove{};
  if (GTT.probe(key, hit) && hit.depth >= depth) {
    ttMove = hit.best;
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
  } else if (GTT.probe(key, hit)) {
    ttMove = hit.best; // use for ordering even if depth is shallow
  }

  if (depth == 0) {
    pv.clear();
    // return qsearch(b, alpha, beta, nodes); // enable if desired
    return eval_side_to_move(b);
  }

  MoveList ml;
  generate_pseudo_legal(b, ml);

  // Order moves
  std::vector<Move> moves;
  moves.reserve(ml.sz);
  for (const auto& m : ml) moves.push_back(m);

  std::stable_sort(moves.begin(), moves.end(),
    [&](const Move& a, const Move& bmv){
      return order_score(b, us, a, ply, ttMove) >
             order_score(b, us, bmv, ply, ttMove);
    });

  bool anyLegal = false;
  Move bestMove{};
  std::vector<Move> bestChildPV;
  int bestScore = -INF;

  int moveIndex = 0;
  for (const auto& m : moves) {
    State st{};
    do_move(b, m, st);
    if (!in_check(b, us)) {
      anyLegal = true;
      std::vector<Move> childPV;

      // --- Safe check extension ---
      const Color them = (us == Color::White ? Color::Black : Color::White);
      int ext = (in_check(b, them) && depth >= 2) ? 1 : 0;

      int newDepth = depth - 1 + ext;
      if (newDepth >= depth) newDepth = depth - 1; // guarantee progress
      newDepth = std::max(1, newDepth);

      // --- Simple LMR for late quiets (conservative) ---
      bool isCapture = square_has_opponent(b, them, m.to);
      bool isPromo   = (m.promo != Piece::None);
      bool isTT      = same_move(m, ttMove);

      int R = 0;
      if (depth >= 3 && !isCapture && !isPromo && !isTT && moveIndex >= 4) {
        R = 1;
      }
      int searchDepth = std::max(1, newDepth - R);

      int score = -negamax(b, searchDepth, -beta, -alpha, nodes, childPV, stopFlag);

      if (score > bestScore) {
        bestScore   = score;
        bestMove    = m;
        bestChildPV = std::move(childPV);
      }

      undo_move(b, m, st);

      if (bestScore >= beta) {
        // record quiet cutoffs as killers/history
        store_killer_history(us, b, m, ply);

        // store cutoff in TT
        GTT.store(key, m, static_cast<std::int16_t>(depth),
                  static_cast<std::int16_t>(bestScore), TTBound::Lower);
        pv.clear();
        return bestScore;
      }

      if (bestScore > alpha) alpha = bestScore;
      ++moveIndex;

      // Check stop between moves too
      if (stopFlag && stopFlag->load(std::memory_order_relaxed)) {
        pv.clear();
        return alpha;
      }
    } else {
      undo_move(b, m, st);
    }
  }

  if (!anyLegal) {
    if (in_check(b, us)) return -MATE + 1; // checkmated
    return 0;                               // stalemate
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

// Clamp helper
static inline int clamp(int x, int lo, int hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

// Internal driver that supports an optional stop flag
static SearchResult search_with_limits(const Board& root,
                                       int maxDepth,
                                       std::atomic<bool>* stopFlag)
{
  SearchResult res;
  res.depth = 0;
  res.nodes = 0;
  res.score = 0;
  res.best  = Move{};
  res.pv.clear();

  Board b = root;

  int lastScore = 0; // previous iteration’s score, seed for aspiration
  for (int d = 1; d <= maxDepth; ++d) {
    if (stopFlag && stopFlag->load(std::memory_order_relaxed)) break;

    std::vector<Move> pv;

    // Optional: clear killers per iteration for determinism
    // std::fill(std::begin(killer1), std::end(killer1), Move{});
    // std::fill(std::begin(killer2), std::end(killer2), Move{});

    int alpha = -INF;
    int beta  = +INF;

    if (d > 1) {
      int asp = 50 + 10 * d; // aspiration half-window
      alpha = clamp(lastScore - asp, -MATE, +MATE);
      beta  = clamp(lastScore + asp, -MATE, +MATE);

      while (true) {
        if (stopFlag && stopFlag->load(std::memory_order_relaxed)) break;

        g_rootDepth = d;
        int score = negamax(b, d, alpha, beta, res.nodes, pv, stopFlag);

        if (score <= alpha) {
          int widen = (beta - alpha) * 2;
          alpha = clamp(score - widen, -INF, +INF);
          continue;
        } else if (score >= beta) {
          int widen = (beta - alpha) * 2;
          beta = clamp(score + widen, -INF, +INF);
          continue;
        } else {
          lastScore   = score;
          res.best    = pv.empty() ? Move{} : pv.front();
          res.pv      = std::move(pv);
          res.score   = score;
          res.depth   = d;
          break;
        }
      }
    } else {
      g_rootDepth = d;
      int score = negamax(b, d, alpha, beta, res.nodes, pv, stopFlag);
      lastScore   = score;
      res.best    = pv.empty() ? Move{} : pv.front();
      res.pv      = std::move(pv);
      res.score   = score;
      res.depth   = d;
    }
  }

  return res;
}

// ------------------------------------------------------------------
// Public entry points
// ------------------------------------------------------------------

SearchResult search(const Board& root, int maxDepth) {
  return search_with_limits(root, std::max(1, maxDepth), /*stopFlag=*/nullptr);
}

// Depth/time wrapper used by UCI. Currently depth-driven; time controls can be
// layered in later without changing the external signature.
SearchResult search(const Board& root, const SearchLimits& lim) {
  int d = lim.depth > 0 ? lim.depth : 6; // default thinking depth
  return search_with_limits(root, d, lim.stop);
}

} // namespace euclid
