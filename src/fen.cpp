#include "euclid/fen.hpp"
#include <cctype>
#include <sstream>
#include <string>   // ensure operator>> into std::string is visible

namespace euclid {

static inline bool is_digit(char c) { return c >= '0' && c <= '9'; }
static inline int to_int(const std::string& s) { return std::stoi(s); }

static inline Piece char_to_piece(char c, Color& out_color) {
  switch (c) {
    case 'P': out_color = Color::White; return Piece::Pawn;
    case 'N': out_color = Color::White; return Piece::Knight;
    case 'B': out_color = Color::White; return Piece::Bishop;
    case 'R': out_color = Color::White; return Piece::Rook;
    case 'Q': out_color = Color::White; return Piece::Queen;
    case 'K': out_color = Color::White; return Piece::King;
    case 'p': out_color = Color::Black; return Piece::Pawn;
    case 'n': out_color = Color::Black; return Piece::Knight;
    case 'b': out_color = Color::Black; return Piece::Bishop;
    case 'r': out_color = Color::Black; return Piece::Rook;
    case 'q': out_color = Color::Black; return Piece::Queen;
    case 'k': out_color = Color::Black; return Piece::King;
    default:  out_color = Color::White; return Piece::None;
  }
}

static inline char piece_to_char(Piece p, Color c) {
  const char* W = "PNBRQK";
  const char* B = "pnbrqk";
  if (p == Piece::None) return '.';
  int idx = static_cast<int>(p);
  return (c == Color::White ? W[idx] : B[idx]);
}

static inline int file_from_char(char f) { return f - 'a'; }
static inline int rank_from_char(char r) { return r - '1'; }

void set_from_fen(Board& b, std::string_view fen) {
  b.clear();

  std::string fen_str(fen);
  std::istringstream ss(fen_str);
  std::string placement, active, castling, ep, half, full;
  if (!(ss >> placement >> active >> castling >> ep >> half >> full))
    throw FenError("Malformed FEN: expected 6 fields");

  // 1) Piece placement
  int r = 7, f = 0;
  for (char ch : placement) {
    if (ch == '/') { --r; f = 0; continue; }
    if (is_digit(ch)) { f += ch - '0'; continue; }
    Color col; Piece p = char_to_piece(ch, col);
    if (p == Piece::None) throw FenError("Invalid piece character in FEN");
    if (f < 0 || f > 7 || r < 0 || r > 7) throw FenError("Square out of range while parsing FEN");
    Square s = r * 8 + f;
    b.set_piece(col, p, s);
    ++f;
  }

  // 2) Active color
  if (active == "w") b.set_side_to_move(Color::White);
  else if (active == "b") b.set_side_to_move(Color::Black);
  else throw FenError("Invalid active color in FEN");

  // 3) Castling rights
  Castling cr{};
  if (castling != "-") {
    for (char cch : castling) {
      if (cch == 'K') cr.rights |= 0x1;
      else if (cch == 'Q') cr.rights |= 0x2;
      else if (cch == 'k') cr.rights |= 0x4;
      else if (cch == 'q') cr.rights |= 0x8;
      else throw FenError("Invalid castling char in FEN");
    }
  }
  b.set_castling(cr);

  // 4) En-passant square
  if (ep == "-") {
    b.set_ep_square(-1);
  } else {
    if (ep.size() != 2) throw FenError("Invalid en-passant square in FEN");
    int file = file_from_char(ep[0]);
    int rank = rank_from_char(ep[1]);
    if (file < 0 || file > 7 || rank < 0 || rank > 7) throw FenError("EP square out of range");
    b.set_ep_square(rank * 8 + file);
  }

  // 5) Halfmove & 6) Fullmove clocks
  b.set_halfmove_clock(to_int(half));
  b.set_fullmove_number(to_int(full));
}

std::string to_fen(const Board& b) {
  std::string out;

  // 1) Piece placement
  for (int r = 7; r >= 0; --r) {
    int empties = 0;
    for (int f = 0; f < 8; ++f) {
      Square s = r * 8 + f;
      Color c;
      Piece p = b.piece_at(s, &c);
      if (p == Piece::None) {
        ++empties;
      } else {
        if (empties) { out += char('0' + empties); empties = 0; }
        out += piece_to_char(p, c);
      }
    }
    if (empties) out += char('0' + empties);
    if (r) out += '/';
  }
  out += ' ';

  // 2) Active color
  out += (b.side_to_move() == Color::White ? 'w' : 'b');
  out += ' ';

  // 3) Castling
  unsigned cr = b.castling().rights;
  if (cr == 0) out += '-';
  else {
    if (cr & 0x1) out += 'K';
    if (cr & 0x2) out += 'Q';
    if (cr & 0x4) out += 'k';
    if (cr & 0x8) out += 'q';
  }
  out += ' ';

  // 4) En-passant square
  int ep = b.ep_square();
  if (ep < 0) out += '-';
  else {
    int file = file_of(ep);
    int rank = rank_of(ep);
    out += char('a' + file);
    out += char('1' + rank);
  }
  out += ' ';

  // 5) Halfmove & 6) Fullmove
  out += std::to_string(b.halfmove_clock());
  out += ' ';
  out += std::to_string(b.fullmove_number());

  return out;
}

} // namespace euclid
