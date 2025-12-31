#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

#include "euclid/board.hpp"
#include "euclid/encode.hpp"
#include "euclid/fen.hpp"
#include "euclid/nn.hpp"
#include "euclid/nn_eval.hpp"
#include "euclid/search.hpp"

using namespace euclid;

int main() {
  // Build a deterministic model: y = bias only (constant), large enough to clip.
  // IMPORTANT: y is interpreted as "side-to-move POV" by neural_evaluate_white_pov().
  MLPConfig cfg;
  cfg.sizes  = {EncodedInputSpec::kTotal, 1};
  cfg.hidden = Activation::ReLU;
  cfg.output = Activation::None;

  MLP mlp(cfg);
  auto& L = mlp.layers_mut().at(0);
  // weights are already zero; set a big positive bias so we hit clipping.
  L.b[0] = 5000.0f; // => clip to +3000

  const std::string path = "euclid_nn_model_tmp_search_pov.txt";
  {
    std::ofstream out(path);
    assert(out.good());
    mlp.save(out);
  }

  neural_eval_clear();
  assert(neural_eval_enabled() == false);

  assert(neural_eval_load_file(path) == true);
  assert(neural_eval_enabled() == true);

  // White to move startpos:
  // NN outputs +5000 stm => clipped +3000 => search score must be +3000 (stm POV).
  {
    Board b;
    set_from_fen(b, STARTPOS_FEN);
    SearchResult r = search(b, /*maxDepth=*/2);
    assert(r.depth >= 1);
    assert(r.score == NN_CP_CLIP);
  }

  // Black to move startpos:
  // NN still outputs +5000 stm => clipped +3000 => search score must be +3000 (stm POV).
  // If sign handling is wrong, this typically becomes -3000.
  {
    Board b;
    set_from_fen(b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    SearchResult r = search(b, /*maxDepth=*/2);
    assert(r.depth >= 1);
    assert(r.score == NN_CP_CLIP);
  }

  neural_eval_clear();
  assert(neural_eval_enabled() == false);

  (void)std::remove(path.c_str());
  return 0;
}
