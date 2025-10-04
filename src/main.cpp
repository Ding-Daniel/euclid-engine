#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <numeric>

#include "euclid/types.hpp"
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"
#include "euclid/eval.hpp"
#include "euclid/uci.hpp"
#include "euclid/search.hpp"

using namespace euclid;

static void usage() {
  std::cout <<
    "Euclid CLI\n"
    "Usage:\n"
    "  euclid_cli perft <depth> [fen...]\n"
    "  euclid_cli divide <depth> [fen...]\n"
    "  euclid_cli eval [fen...]\n"
    "  euclid_cli search depth <N> [fen...]\n"
    "If FEN omitted, uses startpos.\n";
}

static std::string join_from(const std::vector<std::string>& a, size_t i) {
  if (i >= a.size()) return "";
  std::ostringstream oss;
  for (size_t k = i; k < a.size(); ++k) {
    if (k > i) oss << ' ';
    oss << a[k];
  }
  return oss.str();
}

static Board board_from_args(const std::vector<std::string>& a, size_t fenStart) {
  Board b;
  if (fenStart < a.size()) {
    const std::string fen = join_from(a, fenStart);
    set_from_fen(b, fen);
  } else {
    set_from_fen(b, STARTPOS_FEN);
  }
  return b;
}

static int to_int(const std::string& s) {
  return std::stoi(s);
}

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty()) { usage(); return 0; }

  const std::string cmd = args[0];

  // perft <depth> [fen...]
  if (cmd == "perft") {
    if (args.size() < 2) { usage(); return 1; }
    const int depth = to_int(args[1]);
    Board b = board_from_args(args, 2);
    const auto nodes = perft(b, depth);
    std::cout << nodes << "\n";
    return 0;
  }

  // divide <depth> [fen...]
  if (cmd == "divide") {
    if (args.size() < 2) { usage(); return 1; }
    const int depth = to_int(args[1]);
    Board b = board_from_args(args, 2);
    std::vector<std::pair<Move, std::uint64_t>> parts;
    perft_divide(b, depth, parts);
    std::uint64_t total = 0;
    for (auto& [m, n] : parts) {
      std::cout << move_to_uci(m) << " " << n << "\n";
      total += n;
    }
    std::cout << "total " << total << "\n";
    return 0;
  }

  // eval [fen...]
  if (cmd == "eval") {
    Board b = board_from_args(args, 1);
    const int score = evaluate(b);
    std::cout << "eval " << score
              << " (" << (b.side_to_move() == Color::White ? "white" : "black") << " to move)\n";
    return 0;
  }

  // search depth <N> [fen...]
  if (cmd == "search") {
    int depth = 2;
    size_t fenStart = args.size();
    for (size_t i = 1; i + 1 < args.size(); ++i) {
      if (args[i] == "depth") depth = to_int(args[i + 1]);
      if (args[i] == "fen") { fenStart = i + 1; break; } // everything after 'fen' is the FEN
    }
    Board b = board_from_args(args, fenStart);
    auto r = search(b, depth);
    std::cout << "best " << move_to_uci(r.best)
              << " score " << r.score
              << " nodes " << r.nodes
              << " pv ";
    for (auto& m : r.pv) std::cout << move_to_uci(m) << ' ';
    std::cout << "\n";
    return 0;
  }

  usage();
  return 0;
}
