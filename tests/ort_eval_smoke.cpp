#include <cassert>
#include <iostream>

#include "euclid/eval.hpp"
#include "euclid/fen.hpp"
#include "euclid/ort_eval.hpp"

int main() {
  using namespace euclid;

  ort_eval_clear();
  assert(!ort_eval_enabled());

  // No model present: must fail safely (no throw).
  const bool ok = ort_eval_load_file("this_file_should_not_exist_12345.onnx");
  assert(ok == false);
  assert(!ort_eval_enabled());

  // Calls must be safe even when disabled.
  Board b;
  set_from_fen(b, STARTPOS_FEN);
  assert(ort_evaluate_white_pov(b) == 0);

  // And engine eval fallback should still work.
  assert(evaluate(b) == 0);

  std::cout << "ort_eval_smoke ok\n";
  return 0;
}
