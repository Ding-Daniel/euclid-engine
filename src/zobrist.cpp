#include "euclid/zobrist.hpp"
#include <random>


namespace euclid {


Zobrist& Zobrist::instance() {
static Zobrist z;
return z;
}


void Zobrist::init(std::uint64_t seed) {
// Deterministic but non-trivial distribution
std::uint64_t x = seed;
auto next = [&]() -> std::uint64_t { return splitmix64(x); };


for (int c = 0; c < COLOR_N; ++c)
for (int p = 0; p < PIECE_N; ++p)
for (int s = 0; s < 64; ++s)
piece_on[static_cast<std::size_t>(c)]
[static_cast<std::size_t>(p)]
[static_cast<std::size_t>(s)] = next();


for (auto& k : castling) k = next();
for (auto& k : ep_file) k = next();
side_to_move = next();
}


} // namespace euclid