#pragma once
#include <array>
#include <cstdint>

namespace euclid {

// Direction indices (clockwise)
enum : int { DIR_N=0, DIR_E=1, DIR_S=2, DIR_W=3, DIR_NE=4, DIR_SE=5, DIR_SW=6, DIR_NW=7 };

struct AttackTables {
  // Knight / King targets (variable-size lists)
  std::array<std::array<int,8>,64>   knight_to{};
  std::array<uint8_t,64>             knight_sz{};
  std::array<std::array<int,8>,64>   king_to{};
  std::array<uint8_t,64>             king_sz{};

  // Rays: for each direction, up to 7 squares to the edge
  std::array<std::array<std::array<int,7>,64>,8> rays{};
  std::array<std::array<uint8_t,64>,8>           ray_len{};
};

// Singleton accessor (built once, reused everywhere)
const AttackTables& ATT();

} // namespace euclid
