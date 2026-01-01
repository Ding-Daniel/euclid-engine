#include "euclid/selfplay.hpp"

#include <atomic>
#include <chrono>

#include "euclid/attack.hpp"
#include "euclid/draw.hpp"
#include "euclid/move_do.hpp"
#include "euclid/movegen.hpp"

namespace euclid {

static bool has_any_legal_move(Board& b) {
  MoveList ml;
  generate_pseudo_legal(b, ml);

  const Color us = b.side_to_move();
  for (const auto& m : ml) {
    State st{};
    do_move(b, m, st);
    const bool illegal = in_check(b, us);
    undo_move(b, m, st);
    if (!illegal) return true;
  }
  return false;
}

GameReport selfplay_game(Board start, const SearchLimits& limIn, int maxPlies) {
  using clock = std::chrono::steady_clock;

  GameReport rep;
  Board b = start;

  std::vector<U64> history;
  history.reserve(static_cast<std::size_t>(maxPlies) + 2);
  history.push_back(b.hash());

  std::uint64_t nodesTotal = 0;

  std::atomic<bool> stopFlag{false};
  SearchLimits lim = limIn;
  lim.stop = &stopFlag;

  const auto t0 = clock::now();

  for (int ply = 0; ply < maxPlies; ++ply) {
    // Rule draws first (50-move, repetition, insufficient material).
    if (is_rule_draw(b, history)) {
      rep.outcome = GameOutcome::Draw;
      rep.plies = ply;
      rep.reason = "rule draw (50-move, repetition, or insufficient material)";
      break;
    }

    // Terminal by no legal moves: mate or stalemate.
    if (!has_any_legal_move(b)) {
      const bool inCk = in_check(b, b.side_to_move());
      rep.plies = ply;
      if (inCk) {
        // Side to move is checkmated => other side wins.
        rep.outcome = (b.side_to_move() == Color::White)
          ? GameOutcome::BlackWin
          : GameOutcome::WhiteWin;
        rep.reason = "checkmate";
      } else {
        rep.outcome = GameOutcome::Draw;
        rep.reason = "stalemate";
      }
      break;
    }

    stopFlag.store(false, std::memory_order_relaxed);
    SearchResult r = search(b, lim);
    nodesTotal += r.nodes;

    // Defensive: ensure search returned something plausible.
    // If your Move type has a better “is null” check, use it instead.
    Move best = r.best;

    // Apply best move, verify legality the same way UCI does.
    const Color us = b.side_to_move();
    State st{};
    do_move(b, best, st);
    if (in_check(b, us)) {
      undo_move(b, best, st);
      rep.outcome = GameOutcome::Aborted;
      rep.plies = ply;
      rep.reason = "engine produced illegal move";
      break;
    }

    rep.moves.push_back(best);
    history.push_back(b.hash());

    // Continue loop
    rep.plies = ply + 1;
  }

  if (rep.reason.empty()) {
    rep.outcome = GameOutcome::Aborted;
    rep.reason = "max plies reached";
    rep.plies = maxPlies;
  }

  const auto t1 = clock::now();
  rep.seconds = std::chrono::duration<double>(t1 - t0).count();
  rep.nodes = nodesTotal;

  return rep;
}

SelfPlaySummary selfplay_many(const Board& start, const SelfPlayConfig& cfg) {
  SelfPlaySummary sum;
  sum.games = cfg.games;

  for (int i = 0; i < cfg.games; ++i) {
    GameReport rep = selfplay_game(start, cfg.limits, cfg.maxPlies);

    sum.nodes += rep.nodes;
    sum.seconds += rep.seconds;

    switch (rep.outcome) {
      case GameOutcome::WhiteWin: ++sum.whiteWins; break;
      case GameOutcome::BlackWin: ++sum.blackWins; break;
      case GameOutcome::Draw:     ++sum.draws;     break;
      case GameOutcome::Aborted:  ++sum.aborted;   break;
    }
  }

  return sum;
}

} // namespace euclid