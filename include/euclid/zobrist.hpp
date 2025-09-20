#pragma once
#include <array>
#include <cstdint>
#include <random>
#include "euclid/types.hpp"


namespace euclid {


struct Zobrist {
// piece_on[color][piece][square]
std::array<std::array<std::array<U64, 64>, PIECE_N>, COLOR_N> piece_on{};
// castling rights: K, Q, k, q
std::array<U64, 4> castling{};
// en-passant file A..H (only meaningful when an EP square exists)
std::array<U64, 8> ep_file{};
// side to move
U64 side_to_move{};


static Zobrist& instance();
void init(std::uint64_t seed = 0x9E3779B97F4A7C15ULL);
};


// Mix helper for deterministic RNG seeding
inline std::uint64_t splitmix64(std::uint64_t& x) {
x += 0x9e3779b97f4a7c15ULL;
std::uint64_t z = x;
z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
return z ^ (z >> 31);
}


} // namespace euclid