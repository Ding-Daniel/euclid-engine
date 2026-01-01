#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "euclid/board.hpp"
#include "euclid/move.hpp"
#include "euclid/search.hpp"

namespace euclid {

enum class GameOutcome { WhiteWin, BlackWin, Draw, Aborted };

struct GameResult {
  GameOutcome outcome = GameOutcome::Aborted;
  int plies = 0;                 // number of half-moves played
  std::vector<Move> moves;       // moves played from the starting position
  std::string reason;            // human-readable termination reason
};

// Deterministic self-play using your existing search().
// - start: initial position
// - maxPlies: hard cap (prevents infinite games)
// - baseLimits: typically set depth or nodes; stop ptr inside is ignored and replaced internally
GameResult selfplay(Board start, int maxPlies, SearchLimits baseLimits);

} // namespace euclid
