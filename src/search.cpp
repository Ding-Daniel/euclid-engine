#include "euclid/search.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/eval.hpp"
#include "euclid/tt.hpp"
#include "euclid/types.hpp"

#include <chrono>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace euclid {
namespace {

// tune as you like
constexpr int MAX_PLY = 128;

static Move killer1[MAX_PLY];
static Move killer2[MAX_PLY];
static int  historyH[2][64][64];
static int  g_rootDepth = 0;

inline bool same_move(const Move& a, const Move& b) {
  return a.from == b.from && a.to == b.to && a.promo == b.promo;
}

inline bool square_has_opponent(const Board& b, Color us, int sq) {
  Color oc; Piece op = b.piece_at(sq, &oc);
  return op != Piece::None && oc != us;
}

inline bool is_quiet(const Board& b, Color us, const Move& m) {
  if (m.promo != Piece::None) return false;
  return !square_has_opponent(b, us, m.to);
}

// --- draw detection helpers ---
static std::vector<U64> REP_PATH;  // hashes along current search line only

struct RepGuard {
  std::vector<U64>& v;
  RepGuard(std::vector<U64>& vv, U64 key) : v(vv) { v.push_back(key); }
  ~RepGuard() { v.pop_back(); }
};

static inline bool threefold_now() {
  // If current position (the last in REP_PATH) appears >= 3 times, it's a draw.
  if (REP_PATH.empty()) return false;
  const U64 cur = REP_PATH.back();
  int cnt = 0;
  for (U64 k : REP_PATH) if (k == cur) ++cnt;
  return cnt >= 3;
}

static bool insufficient_material(const Board& b) {
  int wp=0,bp=0,wn=0,bn=0,wb=0,bb=0,wr=0,br=0,wq=0,bq=0;

  for (int s = 0; s < 64; ++s) {
    Color c; Piece p = b.piece_at(s, &c);
    switch (p) {
      case Piece::Pawn:   (c==Color::White ? wp : bp)++; break;
      case Piece::Knight: (c==Color::White ? wn : bn)++; break;
      case Piece::Bishop: (c==Color::White ? wb : bb)++; break;
      case Piece::Rook:   (c==Color::White ? wr : br)++; break;
      case Piece::Queen:  (c==Color::White ? wq : bq)++; break;
      default: break;
    }
  }

  // Any pawns/rooks/queens -> not insufficient.
  if (wp+bp+wr+br+wq+bq > 0) return false;

  // Only kings
  if (wn+bn+wb+bb == 0) return true;

  // King+minor vs King
  if ((wn+wb) == 1 && (bn+bb) == 0) return true;
  if ((bn+bb) == 1 && (wn+wb) == 0) return true;

  // KB vs KB with bishops on same color squares
  if (wn+bn == 0 && wb == 1 && bb == 1) {
    int wcol = -1, bcol = -1;
    for (int s = 0; s < 64; ++s) {
      Color c; Piece p = b.piece_at(s, &c);
      if (p == Piece::Bishop) {
        int col = (file_of(s) + rank_of(s)) & 1; // 0=dark,1=light (or vice versa)
        if (c == Color::White) wcol = col; else bcol = col;
      }
    }
    if (wcol == bcol && wcol != -1) return true;
  }

  return false;
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

} // anon

static constexpr int INF  = 30000;
static constexpr int MATE = 29000;

// Piece values for ordering (None,P,N,B,R,Q,K)
static constexpr int PVAL[7] = { 0, 100, 320, 330, 500, 900, 20000 };

static inline int eval_side_to_move(const Board& b) {
  const int e = evaluate(b);
  return b.side_to_move() == Color::White ? e : -e;
}

static inline bool is_en_passant(const Board& b, Color us, const Move& m) {
  Color fc; Piece fromP = b.piece_at(m.from, &fc);
  if (fromP != Piece::Pawn || fc != us) return false;
  if (file_of(m.from) == file_of(m.to)) return false;
  Color tc; Piece toP = b.piece_at(m.to, &tc);
  if (toP != Piece::None) return false;
  return b.ep_square() == m.to;
}

// Basic MVV-LVA + promo bonus used by qsearch ordering
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

// Global-ish TT
static TT GTT;

// ---------------------------
// Quiescence Search (captures/promo/EP; evasions if in check)
// ---------------------------
static int qsearch(Board& b, int alpha, int beta, std::uint64_t& nodes) {
  nodes++;

  const U64 key = b.hash();
  RepGuard guard(REP_PATH, key);  // push current position on the path

  // Draw checks first
  if (b.halfmove_clock() >= 100) return 0;       // 50-move rule
  if (threefold_now()) return 0;                 // threefold on current line
  if (insufficient_material(b)) return 0;        // cannot mate

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
/* Alpha-Beta + TT + killers/history */
static int negamax(Board& b, int depth, int alpha, int beta,
                   std::uint64_t& nodes, std::vector<Move>& pv)
{
  nodes++;
  RepGuard rg(REP_PATH, b.hash());

  if (threefold_now() || insufficient_material(b) || b.halfmove_clock() >= 100) {
    pv.clear();
    return 0; // Draw
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
    if (hit.bound == TTBound::Exact) { pv.clear(); return hit.score; }
    if (hit.bound == TTBound::Lower && hit.score >= beta) { pv.clear(); return hit.score; }
    if (hit.bound == TTBound::Upper && hit.score <= alpha) { pv.clear(); return hit.score; }
  } else if (GTT.probe(key, hit)) {
    ttMove = hit.best; // ordering hint
  }

  if (depth == 0) {
    pv.clear();
    return qsearch(b, alpha, beta, nodes); // enable quiescence at leaves
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

  for (const auto& m : moves) {
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

SearchResult search(const Board& root, int maxDepth) {
  SearchResult res{};
  res.depth = 0;
  res.nodes = 0;
  res.score = 0;
  res.best  = Move{};
  res.pv.clear();

  Board b = root;
  REP_PATH.clear();

  auto clamp = [](int x, int lo, int hi){ return x < lo ? lo : (x > hi ? hi : x); };

  int lastScore = 0;  // previous iteration’s score, seed for aspiration
  for (int d = 1; d <= maxDepth; ++d) {
    std::vector<Move> pv;

    // Optional: clear killers per root iteration (deterministic results)
    // std::fill(std::begin(killer1), std::end(killer1), Move{});
    // std::fill(std::begin(killer2), std::end(killer2), Move{});

    int alpha = -INF;
    int beta  = +INF;

    if (d > 1) {
      int asp = 50 + 10 * d;   // ~0.5 pawn + 0.1 per ply
      alpha = clamp(lastScore - asp, -MATE, +MATE);
      beta  = clamp(lastScore + asp, -MATE, +MATE);

      while (true) {
        g_rootDepth = d;
        int score = negamax(b, d, alpha, beta, res.nodes, pv);
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
      int score = negamax(b, d, alpha, beta, res.nodes, pv);
      lastScore   = score;
      res.best    = pv.empty() ? Move{} : pv.front();
      res.pv      = std::move(pv);
      res.score   = score;
      res.depth   = d;
    }
  }
  return res;
}
// ---- limits-based search overload ----

SearchResult search(const Board& root, const SearchLimits& lim) {
  using clock = std::chrono::steady_clock;

  const int maxDepth = (lim.depth > 0 ? lim.depth : 64);
  const auto start = clock::now();
  const auto deadline = [&]{
    if (lim.movetime_ms > 0) return start + std::chrono::milliseconds(lim.movetime_ms);
    int mtg = lim.movestogo > 0 ? lim.movestogo : 30;
    int bank = (root.side_to_move() == Color::White)
                 ? (lim.wtime_ms + lim.winc_ms)
                 : (lim.btime_ms + lim.binc_ms);
    if (bank <= 0) return clock::time_point::max();
    int budget = std::max(1, bank / std::max(1, mtg) - 10);
    return start + std::chrono::milliseconds(budget);
  }();

  auto time_up = [&]{
    if (lim.stop && lim.stop->load(std::memory_order_relaxed)) return true;
    if (deadline == clock::time_point::max()) return false;
    return clock::now() >= deadline;
  };

  SearchResult res{};
  Board b = root;
  REP_PATH.clear();

  auto clamp = [](int x, int lo, int hi){ return x < lo ? lo : (x > hi ? hi : x); };
  int lastScore = 0;

  for (int d = 1; d <= maxDepth; ++d) {
    std::vector<Move> pv;
    int alpha = -INF, beta = +INF;

    if (d > 1) {
      int asp = 50 + 10 * d;
      alpha = clamp(lastScore - asp, -MATE, +MATE);
      beta  = clamp(lastScore + asp, -MATE, +MATE);

      while (true) {
        if (time_up()) break;
        g_rootDepth = d;
        int score = negamax(b, d, alpha, beta, res.nodes, pv);
        if (score <= alpha) {
          int widen = (beta - alpha) * 2;
          alpha = clamp(score - widen, -INF, +INF);
        } else if (score >= beta) {
          int widen = (beta - alpha) * 2;
          beta = clamp(score + widen, -INF, +INF);
        } else {
          lastScore = score;
          res.best  = pv.empty() ? Move{} : pv.front();
          res.pv    = std::move(pv);
          res.score = score;
          res.depth = d;
          break;
        }
      }
    } else {
      if (time_up()) break;
      g_rootDepth = d;
      int score = negamax(b, d, alpha, beta, res.nodes, pv);
      lastScore = score;
      res.best  = pv.empty() ? Move{} : pv.front();
      res.pv    = std::move(pv);
      res.score = score;
      res.depth = d;
    }

    if (time_up()) break;
  }

  return res;
}

} // namespace euclid
