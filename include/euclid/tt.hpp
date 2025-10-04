#pragma once
#include <cstdint>
#include <vector>
#include "euclid/types.hpp"      // U64, Piece, Color etc.
#include "euclid/movegen.hpp"    // <-- Move lives here

namespace euclid {

// Bound type for TT entries
enum class TTBound : std::int8_t {
  Exact = 0,   // exact score
  Lower = 1,   // beta cut (score is a lower bound)
  Upper = 2    // alpha cut (score is an upper bound)
};

struct TTEntry {
  U64  key = 0;
  Move best{};           // principal/cut move
  std::int16_t depth = -1;
  std::int16_t score = 0;
  TTBound bound = TTBound::Exact;
};

class TT {
public:
  TT();                       // default ~16 MB
  explicit TT(std::size_t bytes);
  void   resize(std::size_t bytes);
  void   clear();

  // Returns true if an entry with matching key exists (copied into out)
  bool   probe(U64 key, TTEntry& out) const;

  // Store or replace
  void   store(U64 key, const Move& best, std::int16_t depth,
               std::int16_t score, TTBound bound);

private:
  std::vector<TTEntry> entries_;
  std::size_t mask_ = 0;

  std::size_t index(U64 key) const { return static_cast<std::size_t>(key) & mask_; }
  void ensure_pow2(std::size_t& n);
};

} // namespace euclid
