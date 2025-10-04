#include "euclid/uci.hpp"
#include "euclid/types.hpp"
#include "euclid/movegen.hpp"
#include <stdexcept>
#include <cctype>

namespace euclid {

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

std::string move_to_uci(const Move& m) {
  std::string s;
  s.reserve(5);
  s.push_back(file_char(m.from));
  s.push_back(rank_char(m.from));
  s.push_back(file_char(m.to));
  s.push_back(rank_char(m.to));
  char pc = promo_to_char(m.promo);   // avoid C++17 if-init to please some linters
  if (pc) s.push_back(pc);
  return s;
}

static inline int sq_from_uci(const std::string& u, int idxFile, int idxRank) {
  if (idxFile < 0 || idxRank < 0 || idxFile >= (int)u.size() || idxRank >= (int)u.size())
    throw std::invalid_argument("bad uci length");
  const char f = u[idxFile], r = u[idxRank];
  if (f < 'a' || f > 'h' || r < '1' || r > '8') throw std::invalid_argument("bad square");
  return (r - '1') * 8 + (f - 'a');
}

Move uci_to_move(const Board& b, const std::string& uci) {
  if (uci.size() != 4 && uci.size() != 5) throw std::invalid_argument("bad uci length");
  const int from = sq_from_uci(uci, 0, 1);
  const int to   = sq_from_uci(uci, 2, 3);
  const Piece wantPromo = (uci.size() == 5 ? promo_from_char(uci[4]) : Piece::None);

  MoveList ml;
  generate_pseudo_legal(b, ml);

  for (const auto& m : ml) {   // use range-for; MoveList has begin()/end()
    if (m.from == from && m.to == to) {
      if ((wantPromo == Piece::None && m.promo == Piece::None) ||
          (wantPromo != Piece::None && m.promo == wantPromo)) {
        return m;
      }
    }
  }
  throw std::invalid_argument("uci move not found among pseudo-legal moves");
}

} // namespace euclid
