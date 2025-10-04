#include "euclid/attacks_tbl.hpp"


namespace euclid {

static inline int file_of(int s){ return s & 7; }
static inline int rank_of(int s){ return s >> 3; }
static inline bool on_board(int s){ return s >= 0 && s < 64; }

static AttackTables build() {
  AttackTables T{};

  // Knight moves (relative)
  const int KN_OFF[8] = {+17,+15,+10,+6,-6,-10,-15,-17};
  for (int s = 0; s < 64; ++s) {
    int f0 = file_of(s), r0 = rank_of(s);
    int n = 0;
    for (int d : KN_OFF) {
      int t = s + d;
      if (!on_board(t)) continue;
      int df = std::abs(file_of(t) - f0);
      int dr = std::abs(rank_of(t) - r0);
      if (!((df == 1 && dr == 2) || (df == 2 && dr == 1))) continue;
      T.knight_to[s][n++] = t;
    }
    T.knight_sz[s] = static_cast<uint8_t>(n);
  }

  // King moves: 8 neighbors
  for (int s = 0; s < 64; ++s) {
    int f0 = file_of(s), r0 = rank_of(s);
    int n = 0;
    for (int df = -1; df <= 1; ++df) {
      for (int dr = -1; dr <= 1; ++dr) {
        if (!df && !dr) continue;
        int f = f0 + df, r = r0 + dr;
        if (f < 0 || f > 7 || r < 0 || r > 7) continue;
        T.king_to[s][n++] = r * 8 + f;
      }
    }
    T.king_sz[s] = static_cast<uint8_t>(n);
  }

  // Rays
  auto push_ray = [&](int s, int df, int dr, int dir_idx) {
    int f = file_of(s) + df, r = rank_of(s) + dr;
    int n = 0;
    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
      T.rays[dir_idx][s][n++] = r * 8 + f;
      f += df; r += dr;
    }
    T.ray_len[dir_idx][s] = static_cast<uint8_t>(n);
  };

  for (int s = 0; s < 64; ++s) {
    push_ray(s,  0,+1, DIR_N);
    push_ray(s, +1, 0, DIR_E);
    push_ray(s,  0,-1, DIR_S);
    push_ray(s, -1, 0, DIR_W);
    push_ray(s, +1,+1, DIR_NE);
    push_ray(s, +1,-1, DIR_SE);
    push_ray(s, -1,-1, DIR_SW);
    push_ray(s, -1,+1, DIR_NW);
  }

  return T;
}

const AttackTables& ATT() {
  static AttackTables T = build();
  return T;
}

} // namespace euclid
