#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <numeric>
#include <cstdint>

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
    "  euclid_cli search [depth <N>] [nodes <N>] [movetime <ms>]\n"
    "                  [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]\n"
    "                  [fen <FEN...>]\n"
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

static std::uint64_t to_u64(const std::string& s) {
  return static_cast<std::uint64_t>(std::stoull(s));
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

  // search [depth <N>] [nodes <N>] [movetime <ms>] ... [fen <FEN...>]
  if (cmd == "search") {
    SearchLimits lim{};
    lim.depth = 2; // preserve previous CLI default

    size_t fenStart = args.size();

    for (size_t i = 1; i < args.size(); ++i) {
      const std::string& tok = args[i];

      if (tok == "fen") { fenStart = i + 1; break; }
      if (i + 1 >= args.size()) break;

      const std::string& val = args[i + 1];

      if (tok == "depth")      { lim.depth = to_int(val); ++i; continue; }
      if (tok == "nodes")      { lim.nodes = to_u64(val); ++i; continue; }
      if (tok == "movetime")   { lim.movetime_ms = to_int(val); ++i; continue; }
      if (tok == "wtime")      { lim.wtime_ms = to_int(val); ++i; continue; }
      if (tok == "btime")      { lim.btime_ms = to_int(val); ++i; continue; }
      if (tok == "winc")       { lim.winc_ms = to_int(val); ++i; continue; }
      if (tok == "binc")       { lim.binc_ms = to_int(val); ++i; continue; }
      if (tok == "movestogo")  { lim.movestogo = to_int(val); ++i; continue; }
    }

    Board b = board_from_args(args, fenStart);
    auto r = search(b, lim);

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
