#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "euclid/encode.hpp"
#include "euclid/eval.hpp"
#include "euclid/fen.hpp"
#include "euclid/nn.hpp"
#include "euclid/nn_eval.hpp"
#include "euclid/uci.hpp"

using namespace euclid;

int main() {
  // Create a deterministic constant-output model: y = +5000 (stm POV) => clipped to +3000.
  MLPConfig cfg;
  cfg.sizes  = {EncodedInputSpec::kTotal, 1};
  cfg.hidden = Activation::ReLU;
  cfg.output = Activation::None;

  MLP mlp(cfg);
  auto& L = mlp.layers_mut().at(0);
  L.b[0] = 5000.0f;

  const std::string path = "euclid_uci_evalmodel_tmp.txt";
  {
    std::ofstream out(path);
    assert(out.good());
    mlp.save(out);
  }

  // Ensure disabled initially.
  neural_eval_clear();
  assert(neural_eval_enabled() == false);

  // Drive UCI: setoption EvalModel value <path>
  {
    std::ostringstream cmd;
    cmd << "uci\n";
    cmd << "setoption name EvalModel value " << path << "\n";
    cmd << "quit\n";

    std::istringstream in(cmd.str());
    std::ostringstream out;
    uci_loop(in, out);

    assert(neural_eval_enabled() == true);
  }

  // Validate that evaluate() is now NN-driven (not material).
  // Use a position with nonzero material to distinguish from fallback material eval.
  {
    Board b;
    set_from_fen(b, "8/8/8/8/8/8/8/QKk5 w - - 0 1"); // white queen present => material would be +900
    const int e = evaluate(b);
    assert(e == NN_CP_CLIP); // +3000 due to clip, white to move
  }

  // Clear via UCI: setoption name EvalModel   (no value => clear)
  {
    std::ostringstream cmd;
    cmd << "setoption name EvalModel\n";
    cmd << "quit\n";
    std::istringstream in(cmd.str());
    std::ostringstream out;
    uci_loop(in, out);
    assert(neural_eval_enabled() == false);
  }

  (void)std::remove(path.c_str());
  return 0;
}
