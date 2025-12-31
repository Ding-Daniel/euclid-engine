#pragma once

#include <cstdint>
#include <iosfwd>
#include <span>
#include <string>
#include <vector>

namespace euclid {

enum class Activation : std::uint8_t {
  None = 0,
  ReLU = 1,
  Tanh = 2,
};

struct MLPConfig {
  std::vector<int> sizes;          // includes input and output, e.g. {781, 256, 64, 1}
  Activation hidden = Activation::ReLU;
  Activation output = Activation::None;
};

class MLP {
public:
  struct Layer {
    int in = 0;
    int out = 0;
    // Row-major by output neuron: w[o*in + i]
    std::vector<float> w; // size = out * in
    std::vector<float> b; // size = out
  };

  MLP() = default;
  explicit MLP(const MLPConfig& cfg);

  int input_dim() const;
  int output_dim() const;

  const std::vector<Layer>& layers() const { return layers_; }
  std::vector<Layer>& layers_mut() { return layers_; }

  // Primary API: span-based.
  std::vector<float> forward(std::span<const float> x) const;
  float forward_scalar(std::span<const float> x) const; // requires output_dim() == 1

  // Convenience overloads for std::vector<float>.
  std::vector<float> forward(const std::vector<float>& x) const {
    return forward(std::span<const float>(x.data(), x.size()));
  }
  float forward_scalar(const std::vector<float>& x) const {
    return forward_scalar(std::span<const float>(x.data(), x.size()));
  }

  bool load(std::istream& in);
  void save(std::ostream& out) const;

  static constexpr int kDefaultInputDim = 781;

private:
  static float apply_act(float v, Activation a);
  static bool parse_activation(const std::string& s, Activation& out);

  std::vector<Layer> layers_;
  Activation hidden_act_ = Activation::ReLU;
  Activation output_act_ = Activation::None;
};

} // namespace euclid