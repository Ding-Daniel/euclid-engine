#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/eval.hpp"
#include "euclid/nn_eval.hpp"

using namespace euclid;

int main() {
  namespace fs = std::filesystem;

  // CTest runs executables from the build directory in this project.
  assert(fs::exists("./euclid_cli") && "euclid_cli must exist in build dir");

  const std::string model = "euclid_const_123.txt";
  (void)fs::remove(model);

  // Generate a constant model file via CLI.
  const int rc = std::system(("./euclid_cli nn_make_const " + model + " 123").c_str());
  assert(rc == 0);
  assert(fs::exists(model));

  // Baseline eval (no NN): startpos is material-equal => 0.
  Board b;
  set_from_fen(b, STARTPOS_FEN);
  neural_eval_clear();
  assert(!neural_eval_enabled());
  const int base = evaluate(b);
  assert(base == 0);

  // Load the generated model file; evaluation should change.
  const bool ok = neural_eval_load_file(model);
  assert(ok);
  assert(neural_eval_enabled());

  const int e = evaluate(b);
  assert(e != 0);
  assert(std::abs(e) >= 50); // should be around 123 (allowing for POV/sign handling)

  // Cleanup
  neural_eval_clear();
  (void)fs::remove(model);
  return 0;
}
