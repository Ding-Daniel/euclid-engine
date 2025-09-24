#pragma once
#include <array>
#include <cstddef>
#include "euclid/move.hpp"


namespace euclid {


struct MoveList {
static constexpr std::size_t CAP = 256;
std::array<Move, CAP> data{};
std::size_t sz = 0;


void push(const Move& m) { if (sz < CAP) data[sz++] = m; }
const Move* begin() const { return data.data(); }
const Move* end() const { return data.data() + sz; }
std::size_t size() const { return sz; }
bool empty() const { return sz == 0; }
};


} // namespace euclid