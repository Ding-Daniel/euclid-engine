#pragma once

#include <string>

#include "euclid/board.hpp"

namespace euclid {

// Roadmap spec: clip NN centipawn output to a bounded range.
constexpr int NN_CP_CLIP = 3000;

// Load a neural model from a text file saved by MLP::save().
// Returns true on success.
bool neural_eval_load_file(const std::string& path);

// Unload/disable neural evaluation.
void neural_eval_clear();

// True only if a model is loaded AND it matches the engine’s current input contract.
bool neural_eval_enabled();

// Evaluate position using the neural model.
// Returns "white-positive" centipawns, consistent with evaluate(const Board&).
// If not enabled, returns 0 (caller should fallback).
int neural_evaluate_white_pov(const Board& b);

} // namespace euclid
