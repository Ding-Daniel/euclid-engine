#include "euclid/search.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/eval.hpp"
#include "euclid/tt.hpp"
#include "euclid/types.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

namespace euclid {
namespace {

// -----------------------------------------------------------------------------
// Tunables / globals
// -----------------------------------------------------------------------------
constexpr int MAX_PLY = 128;

static Move killer1[MAX_PLY];
static Move killer2[MAX_PLY];
static int  historyH[2][64][64];
static int  g_rootDepth = 0;

// Time budget (single-threaded, so plain globals are fine)
static bool g_has_deadline = false;
static std::chrono::steady_clock::time_point g_deadline;

// -----------------------------------------------------------------------------
// Small utilities
// -----------------------------------------------------------------------------
inline bool same_move(const Move& a, const Move& b) {
  return a.from == b.from && a.to == b.to && a.promo == b.promo;
}

inline Color other_color(Color c) {
  return (c == Color::White) ? Color::Black : Color::White;
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
    if (h > (1 << 20)) { // cheap decay
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

// Capture-like test BEFORE making a move (needed for LMR gating)
inline bool is_en_passant_pre(const Board& b, Color us, const Move& m) {
  Color fc; Piece fromP = b.piece_at(m.from, &fc);
  if (fromP != Piece::Pawn || fc != us) return false;
  if (file_of(m.from) == file_of(m.to)) return false;
  return b.ep_square() == m.to;
}
inline bool is_capture_like_pre(const Board& b, Color us, const Move& m) {
  Color tc; Piece toP = b.piece_at(m.to, &tc);
  const bool isDirectCap = (toP != Piece::None && tc != us);
  return isDirectCap || is_en_passant_pre(b, us, m);
}

// Conservative gating for null-move pruning: avoid pawn/king-only endings (zugzwang risk)
inline bool has_non_pawn_material(const Board& b, Color c) {
  for (int sq = 0; sq < 64; ++sq) {
    Color pc;
    Piece p = b.piece_at(sq, &pc);
    if (p == Piece::None || pc != c) continue;
    if (p != Piece::Pawn && p != Piece::King) return true;
  }
  return false;
}

struct NullState {
  Color  prev_stm{};
  Square prev_ep{-1};
  int    prev_halfmove{0};
  int    prev_fullmove{1};
};

inline void do_null_move(Board& b, NullState& ns) {
  ns.prev_stm      = b.side_to_move();
  ns.prev_ep       = b.ep_square();
  ns.prev_halfmove = b.halfmove_clock();
  ns.prev_fullmove = b.fullmove_number();

  // A null move clears EP and toggles side-to-move.
  b.set_ep_square(-1);
  b.set_side_to_move(other_color(ns.prev_stm));

  // Clocks are not part of Zobrist hash in typical engines; preserve for correctness anyway.
  b.set_halfmove_clock(ns.prev_halfmove + 1);
  if (ns.prev_stm == Color::Black) b.set_fullmove_number(ns.prev_fullmove + 1);
}

inline void undo_null_move(Board& b, const NullState& ns) {
  // Restore clocks first (hash-independent), then hashed fields.
  b.set_halfmove_clock(ns.prev_halfmove);
  b.set_fullmove_number(ns.prev_fullmove);

  b.set_side_to_move(ns.prev_stm);
  b.set_ep_square(ns.prev_ep);
}

} // anon

// -----------------------------------------------------------------------------
// Eval helpers
// -----------------------------------------------------------------------------
static constexpr int INF  = 30000;
static constexpr int MATE = 29000;

// Mate-distance helpers for TT storage/retrieval (fail-soft)
static inline int to_tt_score(int score, int ply) {
  if (score > MATE - 1000) return score + ply;  // mate for us
  if (score < -MATE + 1000) return score - ply; // mate for them
  return score;
}
static inline int from_tt_score(int score, int ply) {
  if (score > MATE - 1000) return score - ply;
  if (score < -MATE + 1000) return score + ply;
  return score;
}

static inline int eval_side_to_move(const Board& b) {
  const int e = evaluate(b);
  return b.side_to_move() == Color::White ? e : -e;
}

// -----------------------------------------------------------------------------
// Global TT
// -----------------------------------------------------------------------------
static TT GTT;

// -----------------------------------------------------------------------------
// Quiescence (captures/promo/EP; full evasions if in check)
// -----------------------------------------------------------------------------
static int move_score_basic(const Board& b, Color us, const Move& m) {
  static const int PVAL[7] = { 0, 100, 320, 330, 500, 900, 20000 };
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
  if (is_en_passant_pre(b, us, m)) {
    int attacker = PVAL[static_cast<int>(fromP)];
    return 100000 + 10*PVAL[static_cast<int>(Piece::Pawn)] - attacker + promo_bonus;
  }
  return promo_bonus; // quiet
}

static int qsearch(Board& b, int alpha, int beta, std::uint64_t& nodes,
                   std::atomic<bool>* stopFlag)
{
  nodes++;
  if (stopFlag && (nodes & 0x3FFF) == 0) {
    if ((g_has_deadline && std::chrono::steady_clock::now() >= g_deadline) ||
        stopFlag->load(std::memory_order_relaxed)) {
      return alpha;
    }
  }

  const Color us = b.side_to_move();

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
        int score = -qsearch(b, -beta, -alpha, nodes, stopFlag);
        undo_move(b, m, st);
        if (score >= beta) return score;
        if (score > alpha) alpha = score;
      } else {
        undo_move(b, m, st);
      }
    }
    return alpha;
  }

  // Stand pat
  int stand = eval_side_to_move(b);
  if (stand >= beta) return stand;
  if (stand > alpha) alpha = stand;

  // Tactics only
  MoveList ml;
  generate_pseudo_legal(b, ml);

  std::vector<Move> moves;
  moves.reserve(ml.sz);
  for (const auto& m : ml) {
    Color tc; Piece toP = b.piece_at(m.to, &tc);
    bool isCap = (toP != Piece::None && tc != us);
    bool isEP  = is_en_passant_pre(b, us, m);
    bool isPr  = (m.promo != Piece::None);
    if (isCap || isEP || isPr) moves.push_back(m);
  }

  std::stable_sort(moves.begin(), moves.end(),
    [&](const Move& a, const Move& c){ return move_score_basic(b, us, a) > move_score_basic(b, us, c); });

  for (const auto& m : moves) {
    State st{};
    do_move(b, m, st);
    if (!in_check(b, us)) {
      int score = -qsearch(b, -beta, -alpha, nodes, stopFlag);
      undo_move(b, m, st);
      if (score >= beta) return score;
      if (score > alpha) alpha = score;
    } else {
      undo_move(b, m, st);
    }
  }

  return alpha;
}

// -----------------------------------------------------------------------------
// Negamax with TT + killers/history + LMR + check extension + null-move pruning
// -----------------------------------------------------------------------------
static int negamax(Board& b, int depth, int alpha, int beta,
                   std::uint64_t& nodes, std::vector<Move>& pv,
                   std::atomic<bool>* stopFlag)
{
  nodes++;
  if (stopFlag && (nodes & 0x3FFF) == 0) {
    if ((g_has_deadline && std::chrono::steady_clock::now() >= g_deadline) ||
        stopFlag->load(std::memory_order_relaxed)) {
      pv.clear();
      return alpha;
    }
  }

  const int alphaOrig = alpha;
  const U64 key = b.hash();
  const Color us = b.side_to_move();
  const int ply = std::max(0, g_rootDepth - depth);

  const bool usInCheck = in_check(b, us);
  int staticEval = 0;
  if (!usInCheck) {
    staticEval = eval_side_to_move(b);
  }

  // TT probe (apply mate-distance on load)
  TTEntry hit{};
  Move ttMove{};
  bool haveTT = GTT.probe(key, hit);
  if (haveTT && hit.depth >= depth) {
    ttMove = hit.best;
    int tts = from_tt_score(hit.score, ply);
    if (hit.bound == TTBound::Exact) { pv.clear(); return tts; }
    if (hit.bound == TTBound::Lower && tts >= beta) { pv.clear(); return tts; }
    if (hit.bound == TTBound::Upper && tts <= alpha) { pv.clear(); return tts; }
  } else if (haveTT) {
    ttMove = hit.best; // ordering hint
  }

  // Internal Iterative Deepening (seed a good ttMove on TT miss)
  bool hasTTMove = (ttMove.from | ttMove.to | static_cast<int>(ttMove.promo)) != 0;
  if (!hasTTMove && depth >= 3) {
    std::vector<Move> seedPV;
    (void)negamax(b, depth - 2, alpha, beta, nodes, seedPV, stopFlag);
    TTEntry rehit{};
    if (GTT.probe(key, rehit)) {
      ttMove = rehit.best;
    }
  }

  if (depth == 0) {
    pv.clear();
    return qsearch(b, alpha, beta, nodes, stopFlag);
  }

  // ---------------------------------------------------------------------------
  // Null-move pruning (conservative):
  // - only in non-PV nodes (beta-alpha <= 1)
  // - not in check
  // - depth >= 3
  // - require non-pawn material for both sides (reduce zugzwang risk)
  // ---------------------------------------------------------------------------
  const bool isPV = (beta - alpha) > 1;
  if (!isPV && depth >= 3 && !usInCheck) {
    const Color them = other_color(us);
    if (has_non_pawn_material(b, us) && has_non_pawn_material(b, them)) {
      NullState ns{};
      do_null_move(b, ns);

      std::vector<Move> nullPV;
      const int R = 2 + (depth / 4);
      const int nullDepth = std::max(0, depth - 1 - R);

      int score = -negamax(b, nullDepth, -beta, -beta + 1, nodes, nullPV, stopFlag);

      undo_null_move(b, ns);

      if (stopFlag && stopFlag->load(std::memory_order_relaxed)) {
        pv.clear();
        return alpha;
      }

      if (score >= beta) {
        // Store a lower bound; we do not have a real best move from null pruning.
        GTT.store(key, Move{}, static_cast<std::int16_t>(depth),
                  static_cast<std::int16_t>(to_tt_score(score, ply)), TTBound::Lower);
        pv.clear();
        return score;
      }
    }
  }

  // Generate and order
  MoveList ml;
  generate_pseudo_legal(b, ml);

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

  bool firstMove = true;
  int moveIndex = 0;
  for (const auto& m : moves) {

    const bool isCapLike = is_capture_like_pre(b, us, m);
    const bool isPromo   = (m.promo != Piece::None);
    const bool isTT      = same_move(m, ttMove);

    // Futility pruning at frontier: skip clearly hopeless quiets at depth==1
    if (depth == 1 && !usInCheck && !isCapLike && !isPromo) {
      const int FUT_MARGIN = 200; // small, safe margin
      if (staticEval + FUT_MARGIN <= alpha) { ++moveIndex; continue; }
    }

    State st{};
    do_move(b, m, st);

    if (!in_check(b, us)) {
      anyLegal = true;
      std::vector<Move> childPV;

      // Check extension (after the move)
      const Color them = other_color(us);
      const int ext    = (in_check(b, them) && depth >= 2) ? 1 : 0;

      int baseDepth = depth - 1 + ext;
      if (baseDepth >= depth) baseDepth = depth - 1;

      // LMR on late quiets (never reduce the first PV move)
      int R = 0;
      if (!firstMove && depth >= 3 && !isCapLike && !isPromo && !isTT && moveIndex >= 4) {
        R = 1;
      }
      int reducedDepth = std::max(0, baseDepth - R);
#ifndef NDEBUG
      if (reducedDepth >= depth) reducedDepth = depth - 1;
#endif

      int score;
      if (firstMove) {
        // Full window on the first legal move (PV move)
        score = -negamax(b, baseDepth, -beta, -alpha, nodes, childPV, stopFlag);
      } else {
        // PVS: try a zero-window search first
        score = -negamax(b, reducedDepth, -(alpha + 1), -alpha, nodes, childPV, stopFlag);
        // If it improves alpha, re-search with full window and full (unreduced) depth
        if (score > alpha) {
          childPV.clear();
          score = -negamax(b, baseDepth, -beta, -alpha, nodes, childPV, stopFlag);
        }
      }

      if (score > bestScore) {
        bestScore   = score;
        bestMove    = m;
        bestChildPV = std::move(childPV);
      }

      undo_move(b, m, st);

      if (bestScore >= beta) {
        // Quiet beta-cutoff contributes to killers/history
        if (!isCapLike && !isPromo) {
          store_killer_history(us, b, m, ply);
        }
        GTT.store(key, m, static_cast<std::int16_t>(depth),
                  static_cast<std::int16_t>(to_tt_score(bestScore, ply)), TTBound::Lower);
        pv.clear();
        return bestScore;
      }

      if (bestScore > alpha) alpha = bestScore;
      firstMove = false;
      ++moveIndex;

      if (stopFlag && stopFlag->load(std::memory_order_relaxed)) {
        pv.clear();
        return alpha;
      }
    } else {
      undo_move(b, m, st);
    }
  }

  if (!anyLegal) {
    if (in_check(b, us)) return -MATE + ply; // checkmated (prefer quicker mate)
    return 0;                                 // stalemate
  }

  // PV + TT store
  pv.clear();
  pv.push_back(bestMove);
  pv.insert(pv.end(), bestChildPV.begin(), bestChildPV.end());

  TTBound bound = TTBound::Exact;
  if (bestScore <= alphaOrig) bound = TTBound::Upper;
  else if (bestScore >= beta) bound = TTBound::Lower;

  GTT.store(key, bestMove, static_cast<std::int16_t>(depth),
            static_cast<std::int16_t>(to_tt_score(bestScore, ply)), bound);

  return bestScore;
}

// -----------------------------------------------------------------------------
// Time management helpers
// -----------------------------------------------------------------------------
static int compute_time_budget_ms(const Board& b, const SearchLimits& lim) {
  if (lim.movetime_ms > 0) return lim.movetime_ms;

  const Color us = b.side_to_move();
  const int myTime = (us == Color::White) ? lim.wtime_ms : lim.btime_ms;
  const int myInc  = (us == Color::White) ? lim.winc_ms  : lim.binc_ms;

  if (myTime <= 0) return 0; // no budget provided

  const int mtg = lim.movestogo > 0 ? lim.movestogo : 30;
  int slice = myTime / mtg;
  int budget = slice + (myInc * 3) / 4;

  // Clamp with small safety margins
  budget = std::max(20, std::min(budget, std::max(0, myTime - 30)));
  return budget;
}

// -----------------------------------------------------------------------------
// Core driver with limits (iterative deepening + aspiration windows)
// -----------------------------------------------------------------------------
static SearchResult search_with_limits(const Board& root, int maxDepth,
                                       std::atomic<bool>* stopFlag)
{
  SearchResult res{};
  res.depth = 0;
  res.nodes = 0;
  res.score = 0;
  res.best  = Move{};
  res.pv.clear();

  Board b = root;

  auto clamp = [](int x, int lo, int hi){ return x < lo ? lo : (x > hi ? hi : x); };

  int lastScore = 0;

  for (int d = 1; d <= maxDepth; ++d) {
    std::vector<Move> pv;
    int alpha = -INF, beta = +INF;

    if (d > 1) {
      int asp = 50 + 10 * d;   // aspiration window around last score
      alpha = clamp(lastScore - asp, -MATE, +MATE);
      beta  = clamp(lastScore + asp, -MATE, +MATE);

      while (true) {
        g_rootDepth = d;
        int score = negamax(b, d, alpha, beta, res.nodes, pv, stopFlag);

        if (stopFlag && stopFlag->load(std::memory_order_relaxed)) {
          // preserve the best we had from prior completed iteration
          return res.depth > 0 ? res : SearchResult{};
        }

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
      if (stopFlag && stopFlag->load(std::memory_order_relaxed)) {
        return res.depth > 0 ? res : SearchResult{};
      }
      lastScore   = score;
      res.best    = pv.empty() ? Move{} : pv.front();
      res.pv      = std::move(pv);
      res.score   = score;
      res.depth   = d;
    }
  }

  return res;
}

} // namespace (anon)
namespace euclid {

// ============================================================================
// Public entry points
// ============================================================================
SearchResult search(const Board& root, int maxDepth) {
  g_has_deadline = false; // depth-limited: no time budget
  return search_with_limits(root, std::max(1, maxDepth), /*stopFlag=*/nullptr);
}

SearchResult search(const Board& root, const SearchLimits& lim) {
  // Compute time budget (0 => no budget => depth only)
  const int d = lim.depth > 0 ? lim.depth : 6;
  int budget_ms = compute_time_budget_ms(root, lim);

  std::atomic<bool> dummyStop{false};
  std::atomic<bool>* stopPtr = lim.stop ? lim.stop : &dummyStop;

  if (budget_ms > 0) {
    g_has_deadline = true;
    g_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(budget_ms);
  } else {
    g_has_deadline = false;
  }

  return search_with_limits(root, d, stopPtr);
}

} // namespace euclid
