#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

#include "euclid/nn.hpp"
#include "euclid/nn_eval.hpp"

using namespace euclid;

int main() {
  // Build a tiny deterministic model: y = 5000*x0 + 0*x1 + 0 bias.
  MLPConfig cfg;
  cfg.sizes = {2, 1};
  cfg.hidden = Activation::ReLU;   // irrelevant here
  cfg.output = Activation::None;

  MLP mlp(cfg);
  auto& L = mlp.layers_mut().at(0);
  L.w[0] = 5000.0f; // w[0*in + 0]
  L.w[1] = 0.0f;    // w[0*in + 1]
  L.b[0] = 0.0f;

  // Save to a temp file.
  const auto tmp = std::filesystem::temp_directory_path() / "euclid_mlp_tmp.txt";
  {
    std::ofstream out(tmp);
    assert(out.good());
    mlp.save(out);
  }

  NeuralModel nm;
  assert(nm.load_from_file(tmp.string()) == true);
  assert(nm.loaded() == true);

  // x = [1, 0] => y = 5000 => clipped to +3000
  std::vector<float> x = {1.0f, 0.0f};
  const int cp = nm.eval_cp(x);
  assert(cp == NN_CP_CLIP);

  // Also validate rounding behavior.
  assert(nn_output_to_cp(12.6f) == 13);
  assert(nn_output_to_cp(-12.6f) == -13);

  std::error_code ec;
  std::filesystem::remove(tmp, ec);
  return 0;
}
