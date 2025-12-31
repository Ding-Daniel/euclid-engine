#include <cassert>
#include <fstream>
#include <string>
#include <vector>

#include "euclid/board.hpp"
#include "euclid/encode.hpp"   // <-- needed for EncodedInputSpec
#include "euclid/fen.hpp"
#include "euclid/nn.hpp"
#include "euclid/nn_eval.hpp"
#include "euclid/eval.hpp"

using namespace euclid;

int main() {
  // Deterministic model: y = bias (constant), large enough to clip.
  // Input dim must match EncodedInputSpec::kTotal (781).
  MLPConfig cfg;
  cfg.sizes = {EncodedInputSpec::kTotal, 1};
  cfg.hidden = Activation::ReLU;
  cfg.output = Activation::None;

  MLP mlp(cfg);
  auto& L = mlp.layers_mut().at(0);

  // Weights are already zero. Make bias huge so clipping is tested.
  L.b[0] = 5000.0f; // => should clip to +3000 in stm POV.

  const std::string path = "euclid_nn_model_tmp.txt";
  {
    std::ofstream out(path);
    assert(out.good());
    mlp.save(out);
  }

  neural_eval_clear();
  assert(neural_eval_enabled() == false);

  assert(neural_eval_load_file(path) == true);
  assert(neural_eval_enabled() == true);

  // White to move startpos: NN outputs +5000 stm => clip +3000 => evaluate() white-positive +3000.
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);
    const int e = evaluate(b);
    assert(e == NN_CP_CLIP);
  }

  // Black to move startpos: NN outputs +5000 stm => clip +3000, but evaluate() is white-positive => -3000.
  {
    Board b;
    set_from_fen(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    const int e = evaluate(b);
    assert(e == -NN_CP_CLIP);
  }

  neural_eval_clear();
  assert(neural_eval_enabled() == false);

  // Cleanup (best-effort)
  (void)std::remove(path.c_str());
  return 0;
}
