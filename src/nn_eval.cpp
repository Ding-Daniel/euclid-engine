#include "euclid/nn_eval.hpp"

#include "euclid/encode.hpp"
#include "euclid/nn.hpp"
#include "euclid/types.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <limits>
#include <span>
#include <vector>

namespace euclid {
namespace {

static MLP  g_model{};
static bool g_loaded = false;

static int nn_output_to_cp(float y, int clip_cp) {
  // Round to nearest int (centipawns).
  const long v = std::lround(static_cast<double>(y));

  int cp = 0;
  if (v > static_cast<long>(std::numeric_limits<int>::max())) {
    cp = std::numeric_limits<int>::max();
  } else if (v < static_cast<long>(std::numeric_limits<int>::min())) {
    cp = std::numeric_limits<int>::min();
  } else {
    cp = static_cast<int>(v);
  }

  return std::clamp(cp, -clip_cp, +clip_cp);
}

static bool dims_ok() {
  if (!g_loaded) return false;
  if (g_model.output_dim() != 1) return false;
  if (g_model.input_dim() != EncodedInputSpec::kTotal) return false;
  return true;
}

} // namespace

bool neural_eval_load_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    g_loaded = false;
    g_model = MLP{};
    return false;
  }
  g_loaded = g_model.load(in);
  if (!g_loaded) g_model = MLP{};
  return g_loaded;
}

void neural_eval_clear() {
  g_loaded = false;
  g_model = MLP{};
}

bool neural_eval_enabled() {
  return dims_ok();
}

int neural_evaluate_white_pov(const Board& b) {
  if (!dims_ok()) return 0;

  const std::vector<float> feat = encode_12x64(b);
  assert(static_cast<int>(feat.size()) == g_model.input_dim());

  const float y_stm = g_model.forward_scalar(std::span<const float>(feat.data(), feat.size()));
  const int cp_stm = nn_output_to_cp(y_stm, NN_CP_CLIP);

  // Contract: NN output is POV = side-to-move.
  // evaluate(const Board&) is white-positive.
  if (b.side_to_move() == Color::White) return cp_stm;
  return -cp_stm;
}

} // namespace euclid
