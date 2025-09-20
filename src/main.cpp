#include <iostream>
#include "euclid/board.hpp"


int main() {
using namespace euclid;
Board b;
// Set up a couple of pieces to ensure hash changes
b.set_piece(Color::White, Piece::King, 4); // e1
b.set_piece(Color::Black, Piece::King, 60); // e8
b.set_castling({.rights = 0x1 | 0x4}); // K and k


std::cout << "Euclid Engine CLI (week1 skeleton)\n";
std::cout << "hash: 0x" << std::hex << b.hash() << std::dec << "\n";
return 0;
}