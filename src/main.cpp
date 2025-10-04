#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/search.hpp"
#include "euclid/uci.hpp"


using namespace euclid;

static std::string sqstr(int s){
  std::string out; out += char('a' + file_of(s)); out += char('1' + rank_of(s)); return out;
}

static void usage() {
  std::cout <<
    "Euclid CLI\n"
    "Usage:\n"
    "  euclid_cli perft <depth> [fen...]\n"
    "  euclid_cli divide <depth> [fen...]\n"
    "If FEN omitted, uses startpos.\n";
}

int main(int argc, char** argv) {
  if (argc < 2) { usage(); return 0; }

  std::string cmd = argv[1];
  if (cmd != "perft" && cmd != "divide") { usage(); return 1; }
  if (argc < 3) { usage(); return 1; }

  int depth = std::stoi(argv[2]);

  std::string fen;
  if (argc >= 4) {
    std::ostringstream os;
    for (int i = 3; i < argc; ++i) {
      if (i > 3) os << ' ';
      os << argv[i];
    }
    fen = os.str();
  } else {
    fen = std::string(STARTPOS_FEN);
  }

  Board b; set_from_fen(b, fen);

  if (cmd == "perft") {
    std::uint64_t nodes = perft(b, depth);
    std::cout << "nodes " << nodes << "\n";
    return 0;
  }

  // divide
  MoveList ml; generate_pseudo_legal(b, ml);
  const Color us = b.side_to_move();

  std::uint64_t total = 0ULL;
  for (const auto& m : ml) {
    State st{};
    do_move(b, m, st);
    // legality filter (same rule as perft)
    //extern bool in_check(const Board&, Color); //we dont want a global extern 
    if (!in_check(b, us)) {
      std::uint64_t n = perft(b, depth - 1);
      std::cout << sqstr(m.from) << sqstr(m.to);
      if (m.promo != Piece::None) {
        const char promoChars[] = {' ', 'P','N','B','R','Q','K'};
        std::cout << promoChars[static_cast<int>(m.promo)];
      }
      std::cout << ": " << n << "\n";
      total += n;
    }
    undo_move(b, m, st);
  }
  std::cout << "total " << total << "\n";
  return 0;
}

static void cmd_search(const std::vector<std::string>& args) {
  // usage: search depth N [fen <FEN...>]
  int depth = 2;
  std::string fen = std::string(STARTPOS_FEN);
  for (size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == "depth") depth = std::stoi(args[i+1]);
    if (args[i] == "fen") {
      std::ostringstream oss;
      for (size_t j = i+1; j < args.size(); ++j) {
        if (j > i+1) oss << ' ';
        oss << args[j];
      }
      fen = oss.str();
      break;
    }
  }
  Board b; set_from_fen(b, fen);
  auto r = search(b, depth);
  std::cout << "best " << move_to_uci(r.best)
            << " score " << r.score
            << " nodes " << r.nodes
            << " pv ";
  for (auto& m : r.pv) std::cout << move_to_uci(m) << ' ';
  std::cout << "\n";
}