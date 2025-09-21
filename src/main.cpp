#include <iostream>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"

int main() {
  using namespace euclid;
  Board b;
  set_from_fen(b, STARTPOS_FEN);
  std::cout << "Euclid Engine CLI (week1 step2)\n";
  std::cout << "FEN: " << to_fen(b) << "\n";
  std::cout << "hash: 0x" << std::hex << b.hash() << std::dec << "\n";
}