#include "euclid/nn.hpp"

#include <cassert>
#include <cmath>
#include <istream>
#include <ostream>
#include <string>

namespace euclid {

static bool all_positive(const std::vector<int>& v) {
  for (int x : v) if (x <= 0) return false;
  return true;
}

MLP::MLP(const MLPConfig& cfg)
  : hidden_act_(cfg.hidden), output_act_(cfg.output) {

  assert(cfg.sizes.size() >= 2);
  assert(all_positive(cfg.sizes));

  layers_.clear();
  layers_.reserve(cfg.sizes.size() - 1);

  for (size_t i = 0; i + 1 < cfg.sizes.size(); ++i) {
    Layer L;
    L.in = cfg.sizes[i];
    L.out = cfg.sizes[i + 1];
    L.w.assign(static_cast<size_t>(L.in) * static_cast<size_t>(L.out), 0.0f);
    L.b.assign(static_cast<size_t>(L.out), 0.0f);
    layers_.push_back(std::move(L));
  }
}

int MLP::input_dim() const {
  return layers_.empty() ? 0 : layers_.front().in;
}

int MLP::output_dim() const {
  return layers_.empty() ? 0 : layers_.back().out;
}

float MLP::apply_act(float v, Activation a) {
  switch (a) {
    case Activation::None: return v;
    case Activation::ReLU: return v > 0.0f ? v : 0.0f;
    case Activation::Tanh: return std::tanh(v);
  }
  return v;
}

bool MLP::parse_activation(const std::string& s, Activation& out) {
  if (s == "None" || s == "Linear") { out = Activation::None; return true; }
  if (s == "ReLU") { out = Activation::ReLU; return true; }
  if (s == "Tanh") { out = Activation::Tanh; return true; }
  return false;
}

std::vector<float> MLP::forward(std::span<const float> x) const {
  assert(!layers_.empty());
  assert(static_cast<int>(x.size()) == input_dim());

  std::vector<float> cur(x.begin(), x.end());
  std::vector<float> nxt;

  for (size_t li = 0; li < layers_.size(); ++li) {
    const Layer& L = layers_[li];
    nxt.assign(static_cast<size_t>(L.out), 0.0f);

    for (int o = 0; o < L.out; ++o) {
      float acc = L.b[static_cast<size_t>(o)];
      const size_t row = static_cast<size_t>(o) * static_cast<size_t>(L.in);
      for (int i = 0; i < L.in; ++i) {
        acc += L.w[row + static_cast<size_t>(i)] * cur[static_cast<size_t>(i)];
      }

      const bool is_last = (li + 1 == layers_.size());
      const Activation act = is_last ? output_act_ : hidden_act_;
      nxt[static_cast<size_t>(o)] = apply_act(acc, act);
    }

    cur.swap(nxt);
  }

  return cur;
}

float MLP::forward_scalar(std::span<const float> x) const {
  assert(output_dim() == 1);
  auto y = forward(x);
  return y[0];
}

void MLP::save(std::ostream& out) const {
  out << "EUCLID_MLP 1\n";
  out << "sizes " << (layers_.empty() ? 0 : (static_cast<int>(layers_.size()) + 1));
  if (!layers_.empty()) {
    out << " " << layers_.front().in;
    for (const auto& L : layers_) out << " " << L.out;
  }
  out << "\n";

  auto act_to_str = [](Activation a) -> const char* {
    switch (a) {
      case Activation::None: return "None";
      case Activation::ReLU: return "ReLU";
      case Activation::Tanh: return "Tanh";
    }
    return "None";
  };

  out << "hidden " << act_to_str(hidden_act_) << "\n";
  out << "output " << act_to_str(output_act_) << "\n";

  for (size_t li = 0; li < layers_.size(); ++li) {
    const Layer& L = layers_[li];
    out << "layer " << li << "\n";
    out << "W";
    for (float v : L.w) out << " " << v;
    out << "\nB";
    for (float v : L.b) out << " " << v;
    out << "\n";
  }
}

bool MLP::load(std::istream& in) {
  std::string magic;
  int version = 0;
  if (!(in >> magic >> version)) return false;
  if (magic != "EUCLID_MLP" || version != 1) return false;

  std::string kw;
  if (!(in >> kw) || kw != "sizes") return false;

  int n = 0;
  if (!(in >> n) || n < 2) return false;

  std::vector<int> sizes(static_cast<size_t>(n), 0);
  for (int i = 0; i < n; ++i) {
    if (!(in >> sizes[static_cast<size_t>(i)])) return false;
  }
  if (!all_positive(sizes)) return false;

  std::string hidden_kw, hidden_s;
  if (!(in >> hidden_kw >> hidden_s) || hidden_kw != "hidden") return false;

  std::string output_kw, output_s;
  if (!(in >> output_kw >> output_s) || output_kw != "output") return false;

  Activation hid = Activation::ReLU, outAct = Activation::None;
  if (!parse_activation(hidden_s, hid)) return false;
  if (!parse_activation(output_s, outAct)) return false;

  MLPConfig cfg;
  cfg.sizes = std::move(sizes);
  cfg.hidden = hid;
  cfg.output = outAct;
  *this = MLP(cfg);

  for (size_t li = 0; li < layers_.size(); ++li) {
    std::string layer_kw;
    size_t layer_idx = 0;
    if (!(in >> layer_kw >> layer_idx)) return false;
    if (layer_kw != "layer" || layer_idx != li) return false;

    std::string Wkw;
    if (!(in >> Wkw) || Wkw != "W") return false;

    Layer& L = layers_[li];
    const size_t wcount = static_cast<size_t>(L.in) * static_cast<size_t>(L.out);
    for (size_t k = 0; k < wcount; ++k) {
      if (!(in >> L.w[k])) return false;
    }

    std::string Bkw;
    if (!(in >> Bkw) || Bkw != "B") return false;

    for (size_t k = 0; k < static_cast<size_t>(L.out); ++k) {
      if (!(in >> L.b[k])) return false;
    }
  }

  return true;
}

} // namespace euclid