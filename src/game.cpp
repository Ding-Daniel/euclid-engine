#include "euclid/game.hpp"

#include "euclid/attack.hpp"
#include "euclid/draw.hpp"
#include "euclid/move_do.hpp"
#include "euclid/movegen.hpp"

#include <atomic>
#include <utility>
#include <vector>

namespace euclid {
namespace {

static inline bool is_null_move(const Move& m) {
  return m.from == 0 && m.to == 0 && m.promo == Piece::None;
}

static bool is_legal_move(Board& b, const Move& m) {
  const Color us = b.side_to_move();
  State st{};
  do_move(b, m, st);
  const bool ok = !in_check(b, us);
  undo_move(b, m, st);
  return ok;
}

static Move first_legal_move(Board& b) {
  MoveList ml;
  generate_pseudo_legal(b, ml);
  for (const auto& m : ml) {
    if (is_legal_move(b, m)) return m;
  }
  return Move{}; // "null"
}

static bool has_any_legal(Board& b) {
  MoveList ml;
  generate_pseudo_legal(b, ml);
  for (const auto& m : ml) {
    if (is_legal_move(b, m)) return true;
  }
  return false;
}

static inline GameOutcome winner_from_mated_side(Color stm) {
  // stm is side-to-move and has no legal moves while in check => stm is mated
  return (stm == Color::White) ? GameOutcome::BlackWin : GameOutcome::WhiteWin;
}

} // namespace

GameResult selfplay(Board start, int maxPlies, SearchLimits baseLimits) {
  GameResult out{};
  Board b = std::move(start);

  if (maxPlies < 0) maxPlies = 0;

  // History for threefold detection (draw.hpp expects history.back() = current key)
  std::vector<U64> hist;
  hist.reserve(static_cast<std::size_t>(maxPlies) + 1);
  hist.push_back(b.hash());

  // Ensure we have a sensible deterministic control knob.
  if (baseLimits.depth <= 0 && baseLimits.nodes == 0 && baseLimits.movetime_ms == 0 &&
      baseLimits.wtime_ms == 0 && baseLimits.btime_ms == 0) {
    baseLimits.depth = 2;
  }

  std::atomic<bool> stop{false};
  baseLimits.stop = &stop;

  for (int ply = 0; ply < maxPlies; ++ply) {
    // Rule draws (50-move, repetition, insufficient)
    if (is_rule_draw(b, hist)) {
      out.outcome = GameOutcome::Draw;
      out.reason = "rule draw (50-move, repetition, or insufficient material)";
      break;
    }

    // Terminal by no legal moves (mate/stalemate)
    if (!has_any_legal(b)) {
      if (in_check(b, b.side_to_move())) {
        out.outcome = winner_from_mated_side(b.side_to_move());
        out.reason = "checkmate";
      } else {
        out.outcome = GameOutcome::Draw;
        out.reason = "stalemate";
      }
      break;
    }

    stop.store(false, std::memory_order_relaxed);
    SearchResult r = search(b, baseLimits);

    Move m = r.best;

    // Robustness: if search returns a null/illegal move, fall back to first legal.
    if (is_null_move(m) || !is_legal_move(b, m)) {
      m = first_legal_move(b);
    }
    if (is_null_move(m) || !is_legal_move(b, m)) {
      out.outcome = GameOutcome::Aborted;
      out.reason = "no legal move selectable (internal error)";
      break;
    }

    // Play it
    const Color us = b.side_to_move();
    State st{};
    do_move(b, m, st);

    // Assert legality invariant in debug-ish terms (still safe in Release).
    if (in_check(b, us)) {
      undo_move(b, m, st);
      out.outcome = GameOutcome::Aborted;
      out.reason = "selected move was illegal (king left in check)";
      break;
    }

    out.moves.push_back(m);
    hist.push_back(b.hash());
  }

  out.plies = static_cast<int>(out.moves.size());

  if (out.outcome == GameOutcome::Aborted && out.reason.empty()) {
    out.reason = (out.plies >= maxPlies) ? "max plies reached" : "aborted";
  }
  if (out.outcome == GameOutcome::Aborted && out.plies >= maxPlies) {
    out.reason = "max plies reached";
  }
  if (out.outcome == GameOutcome::Aborted && out.plies < maxPlies && out.reason == "aborted") {
    // keep as-is
  }
  if (out.outcome == GameOutcome::Aborted && out.plies == maxPlies && out.reason.empty()) {
    out.reason = "max plies reached";
  }
  if (out.outcome == GameOutcome::Aborted && out.plies == maxPlies && out.reason == "aborted") {
    out.reason = "max plies reached";
  }

  // If we never set a terminal outcome but we hit the ply cap, classify it.
  if (out.reason.empty() && out.plies >= maxPlies) {
    out.outcome = GameOutcome::Aborted;
    out.reason = "max plies reached";
  }

  // Dataset-generation semantics:
  // If we stopped only because we hit the ply cap, treat it as a Draw (label 0),
  // not an Aborted game (which would otherwise produce 0 records when includeAborted=false).
  if (out.outcome == GameOutcome::Aborted && out.reason == "max plies reached") {
    out.outcome = GameOutcome::Draw;
  }

  return out;
}

} // namespace euclid
