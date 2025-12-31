#include <cassert>
#include <cmath>
#include <sstream>
#include <vector>

#include "euclid/nn.hpp"

using namespace euclid;

static bool feq(float a, float b, float eps = 1e-6f) {
  return std::fabs(a - b) <= eps;
}

int main() {
  // 3 -> 2 -> 1, ReLU hidden, linear output
  MLPConfig cfg;
  cfg.sizes = {3, 2, 1};
  cfg.hidden = Activation::ReLU;
  cfg.output = Activation::None;

  MLP net(cfg);

  // Layer 0: out2 x in3
  // neuron0 = 1*x0 + (-2)*x1 + 0*x2 + b0
  // neuron1 = 0*x0 +  1*x1 + 3*x2 + b1
  auto& L0 = net.layers_mut()[0];
  L0.w = {
    1.0f, -2.0f, 0.0f,
    0.0f,  1.0f, 3.0f
  };
  L0.b = {0.0f, 0.0f};

  // Layer 1: out1 x in2
  // y = 2*h0 + 3*h1 + b
  auto& L1 = net.layers_mut()[1];
  L1.w = {2.0f, 3.0f};
  L1.b = {1.0f};

  {
    // x = [1,2,3]
    // h0 = 1*1 + (-2)*2 + 0*3 = -3 => ReLU -> 0
    // h1 = 0*1 + 1*2 + 3*3 = 11 => ReLU -> 11
    // y = 2*0 + 3*11 + 1 = 34
    std::vector<float> x = {1.0f, 2.0f, 3.0f};
    float y = net.forward_scalar(x);
    assert(feq(y, 34.0f));
  }

  {
    // x = [5,1,0]
    // h0 = 1*5 + (-2)*1 = 3 => ReLU 3
    // h1 = 1*1 + 3*0 = 1 => ReLU 1
    // y = 2*3 + 3*1 + 1 = 10
    std::vector<float> x = {5.0f, 1.0f, 0.0f};
    float y = net.forward_scalar(x);
    assert(feq(y, 10.0f));
  }

  // Save/load roundtrip
  std::stringstream ss;
  net.save(ss);

  MLP net2;
  bool ok = net2.load(ss);
  assert(ok);

  {
    std::vector<float> x = {1.0f, 2.0f, 3.0f};
    float y1 = net.forward_scalar(x);
    float y2 = net2.forward_scalar(x);
    assert(feq(y1, y2));
  }

  return 0;
}