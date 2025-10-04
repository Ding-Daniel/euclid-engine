#include "euclid/tt.hpp"
#include <algorithm>

namespace euclid {

static constexpr std::size_t DEFAULT_BYTES = 16ULL * 1024ULL * 1024ULL;

TT::TT() { resize(DEFAULT_BYTES); }
TT::TT(std::size_t bytes) { resize(bytes); }

void TT::ensure_pow2(std::size_t& n) {
  if (n == 0) { n = 1; return; }
  // round up to power of two
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFFu
  n |= n >> 32;
#endif
  n++;
}

void TT::resize(std::size_t bytes) {
  std::size_t n = bytes / sizeof(TTEntry);
  if (n < 1) n = 1;
  ensure_pow2(n);
  entries_.assign(n, TTEntry{});
  mask_ = n - 1;
}

void TT::clear() {
  std::fill(entries_.begin(), entries_.end(), TTEntry{});
}

bool TT::probe(U64 key, TTEntry& out) const {
  const TTEntry& e = entries_[index(key)];
  if (e.key == key && e.depth >= 0) { out = e; return true; }
  return false;
}

void TT::store(U64 key, const Move& best, std::int16_t depth,
               std::int16_t score, TTBound bound) {
  TTEntry& e = entries_[index(key)];
  // Replace if slot empty or new depth is better
  if (e.depth < depth || e.key != key) {
    e.key   = key;
    e.best  = best;
    e.depth = depth;
    e.score = score;
    e.bound = bound;
  }
}

} // namespace euclid
