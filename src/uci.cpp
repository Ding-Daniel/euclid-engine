#include "euclid/uci.hpp"
#include "euclid/types.hpp"
#include "euclid/board.hpp"
#include "euclid/movegen.hpp"
#include "euclid/move_do.hpp"
#include "euclid/attack.hpp"
#include "euclid/search.hpp"

#include <stdexcept>
#include <cctype>
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::atomic<bool> G_STOP{false};

namespace euclid {

// ------------ helpers ------------
static inline char file_char(int s) { return char('a' + file_of(s)); }
static inline char rank_char(int s) { return char('1' + rank_of(s)); }

static inline Piece promo_from_char(char c) {
  switch (std::tolower(static_cast<unsigned char>(c))) {
    case 'q': return Piece::Queen;
    case 'r': return Piece::Rook;
    case 'b': return Piece::Bishop;
    case 'n': return Piece::Knight;
    default:  return Piece::None;
  }
}

static inline char promo_to_char(Piece p) {
  switch (p) {
    case Piece::Queen:  return 'q';
    case Piece::Rook:   return 'r';
    case Piece::Bishop: return 'b';
    case Piece::Knight: return 'n';
    default:            return '\0';
  }
}

// ------------ UCI move conversion ------------
std::string move_to_uci(const Move& m) {
  std::string s;
  s.reserve(5);
  s.push_back(file_char(m.from));
  s.push_back(rank_char(m.from));
  s.push_back(file_char(m.to));
  s.push_back(rank_char(m.to));
  char pc = promo_to_char(m.promo); // C++14/17 friendly
  if (pc) s.push_back(pc);
  return s;
}

static inline int sq_from_uci(const std::string& u, int idxFile, int idxRank) {
  if (idxFile < 0 || idxRank < 0
      || idxFile >= static_cast<int>(u.size())
      || idxRank >= static_cast<int>(u.size()))
    throw std::invalid_argument("bad uci length");

  const char f = u[static_cast<size_t>(idxFile)];
  const char r = u[static_cast<size_t>(idxRank)];
  if (f < 'a' || f > 'h' || r < '1' || r > '8')
    throw std::invalid_argument("bad square");

  return (r - '1') * 8 + (f - 'a');
}

Move uci_to_move(const Board& b, const std::string& uci) {
  if (uci.size() != 4 && uci.size() != 5)
    throw std::invalid_argument("bad uci length");

  const int from = sq_from_uci(uci, 0, 1);
  const int to   = sq_from_uci(uci, 2, 3);
  const Piece wantPromo = (uci.size() == 5 ? promo_from_char(uci[4]) : Piece::None);

  MoveList ml;
  generate_pseudo_legal(b, ml);

  for (const auto& m : ml) { // MoveList supports range-for
    if (m.from == from && m.to == to) {
      if ((wantPromo == Piece::None && m.promo == Piece::None) ||
          (wantPromo != Piece::None && m.promo == wantPromo)) {
        return m;
      }
    }
  }
  throw std::invalid_argument("uci move not found among pseudo-legal moves");
}

// ------------ tokenization ------------
static std::vector<std::string> split_ws(const std::string& line) {
  std::istringstream iss(line);
  std::vector<std::string> out;
  std::string tok;
  while (iss >> tok) out.push_back(tok);
  return out;
}

// apply a sequence of UCI moves to a board (checks legality via in_check)
static void apply_moves(Board& b, const std::vector<std::string>& toks, size_t startIdx) {
  for (size_t i = startIdx; i < toks.size(); ++i) {
    const std::string& u = toks[i];
    Move m = uci_to_move(b, u);

    const Color us = b.side_to_move(); // capture mover color before do_move flips

    State st{};
    do_move(b, m, st);

    if (in_check(b, us)) { // illegal if our king in check after move
      undo_move(b, m, st);
      break;
    }
  }
}

// ------------ minimal UCI loop ------------
void uci_loop(std::istream& in, std::ostream& out) {
  Board b; // startpos
  G_STOP.store(false, std::memory_order_relaxed);

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    auto tokens = split_ws(line);
    const std::string& cmd = tokens[0];

    if (cmd == "uci") {
      out << "id name Euclid\n";
      out << "id author You\n";
      out << "uciok\n";
      out.flush();
    }
    else if (cmd == "isready") {
      out << "readyok\n";
      out.flush();
    }
    else if (cmd == "ucinewgame") {
      G_STOP.store(false, std::memory_order_relaxed);
      // optional: clear transposition table here
      // e.g. TT::global().clear();
    }
    else if (cmd == "position") {
      // position startpos [moves ...]
      // position fen <FEN...> [moves ...]
      if (tokens.size() >= 2) {
        size_t i = 1;
        if (tokens[i] == "startpos") {
          b = Board{}; // reset to start
          ++i;
        } else if (tokens[i] == "fen") {
          std::string fen;
          ++i;
          while (i < tokens.size() && tokens[i] != "moves") {
            if (!fen.empty()) fen.push_back(' ');
            fen += tokens[i++];
          }
          // If you have a FEN loader, call it here.
          // set_from_fen(b, fen);
        }
        if (i < tokens.size() && tokens[i] == "moves") {
          apply_moves(b, tokens, i + 1);
        }
      }
    }
    else if (cmd == "stop") {
      G_STOP.store(true, std::memory_order_relaxed);
    }
    else if (cmd == "go") {
      SearchLimits lim;
      lim.stop = &G_STOP;
      // parse typical UCI time/depth options
      for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& t = tokens[i];
        auto rd = [&](int& dst){ if (i + 1 < tokens.size()) dst = std::stoi(tokens[++i]); };
        if      (t == "depth")      rd(lim.depth);
        else if (t == "movetime")   rd(lim.movetime_ms);
        else if (t == "wtime")      rd(lim.wtime_ms);
        else if (t == "btime")      rd(lim.btime_ms);
        else if (t == "winc")       rd(lim.winc_ms);
        else if (t == "binc")       rd(lim.binc_ms);
        else if (t == "movestogo")  rd(lim.movestogo);
        else if (t == "infinite") { lim.depth = 99; lim.movetime_ms = 0; }
      }
      G_STOP.store(false, std::memory_order_relaxed);
      auto res = search(b, lim);
      out << "bestmove " << move_to_uci(res.best) << "\n";
      out.flush();
    }
    else if (cmd == "quit") {
      break;
    }
    // else: ignore unknown per UCI spec
  }
}

void uci_loop() { uci_loop(std::cin, std::cout); }

} // namespace euclid
