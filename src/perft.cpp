#include "euclid/perft.hpp"
#include "euclid/movegen.hpp"


namespace euclid {


std::uint64_t perft(const Board& b, int depth) {
if (depth == 0) return 1ULL;
MoveList ml;
generate_pseudo_legal(b, ml);
std::uint64_t nodes = 0ULL;
for (const auto& m : ml) {
(void)m;
Board child = b; // TODO: apply move when movegen is implemented
nodes += perft(child, depth - 1);
}
return nodes;
}


} // namespace euclid