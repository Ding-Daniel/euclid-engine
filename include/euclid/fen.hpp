#pragma once
#include <string>
#include <string_view>
#include <stdexcept>
#include "euclid/board.hpp"

namespace euclid {

struct FenError : std::runtime_error { using std::runtime_error::runtime_error; };

// Use a char[] here to avoid any toolchain hiccups with constexpr string_view
inline constexpr char STARTPOS_FEN[] =
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void set_from_fen(Board& b, std::string_view fen);
std::string to_fen(const Board& b);

} // namespace euclid
