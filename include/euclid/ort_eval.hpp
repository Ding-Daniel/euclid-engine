#pragma once

#include <string>

#include "euclid/board.hpp"

namespace euclid {

// ORT NN output is clipped to this bound (centipawns), consistent with NN_CP_CLIP.
constexpr int ORT_CP_CLIP = 3000;

// Load an ONNX model for evaluation.
// Expected contract:
// - Input: float tensor containing the engine's EncodedInputSpec::kTotal floats (781).
// - Output: single float interpreted as centipawns from side-to-move POV.
// Returns true on success.
bool ort_eval_load_file(const std::string& path);

// Unload/disable ORT evaluation.
void ort_eval_clear();

// True only if ORT model is loaded AND it matches the engine’s current input contract.
bool ort_eval_enabled();

// Evaluate position using ORT.
// Returns "white-positive" centipawns, consistent with evaluate(const Board&).
// If not enabled, returns 0 (caller should fallback).
int ort_evaluate_white_pov(const Board& b);

// Returns true on macOS when the detected ONNX Runtime version is in the range
// known to crash at process exit with:
//   "std::__1::system_error: mutex lock failed: Invalid argument"
// Workaround for executables/tests: flush output and use std::quick_exit(code)
// instead of returning from main. See: microsoft/onnxruntime#24579, #25038.
bool ort_eval_macos_exit_workaround_needed();

} // namespace euclid
