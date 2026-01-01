#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "euclid/board.hpp"
#include "euclid/move.hpp"
#include "euclid/search.hpp"

namespace euclid {

enum class GameOutcome {
  WhiteWin,
  BlackWin,
  Draw,
  Aborted
};

struct GameReport {
  GameOutcome outcome = GameOutcome::Aborted;
  int plies = 0;                       // number of half-moves played
  std::string reason;                  // human-readable termination reason
  std::vector<Move> moves;             // played moves
  std::uint64_t nodes = 0;             // total nodes searched across plies
  double seconds = 0.0;                // wall time spent in self-play loop
};

struct SelfPlayConfig {
  int games = 1;
  int maxPlies = 200;                  // stop early; you already prefer this
  SearchLimits limits{};               // depth/nodes/movetime/wtime/etc
  bool printPerGame = true;            // print one-line result per game
};

struct SelfPlaySummary {
  int games = 0;
  int whiteWins = 0;
  int blackWins = 0;
  int draws = 0;
  int aborted = 0;

  std::uint64_t nodes = 0;
  double seconds = 0.0;
};

GameReport selfplay_game(Board start, const SearchLimits& lim, int maxPlies);
SelfPlaySummary selfplay_many(const Board& start, const SelfPlayConfig& cfg);

} // namespace euclid