#include "euclid/uci.hpp"
#include <iostream>

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  euclid::uci_loop(std::cin, std::cout);
  return 0;
}
