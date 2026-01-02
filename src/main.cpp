#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <numeric>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <algorithm>

#include "euclid/types.hpp"
#include "euclid/board.hpp"
#include "euclid/fen.hpp"
#include "euclid/perft.hpp"
#include "euclid/eval.hpp"
#include "euclid/uci.hpp"
#include "euclid/search.hpp"
#include "euclid/nn_eval.hpp"
#include "euclid/nn.hpp"
#include "euclid/encode.hpp"

using namespace euclid;

static void usage() {
  std::cout <<
    "Euclid CLI\n"
    "Usage:\n"
    "  euclid_cli uci\n"
    "  euclid_cli perft <depth> [fen...]\n"
    "  euclid_cli divide <depth> [fen...]\n"
    "  euclid_cli eval [fen...]\n"
    "  euclid_cli eval nn <model_path> [fen...]\n"
    "  euclid_cli search [nn <model_path>] [depth <N>] [nodes <N>] [movetime <ms>]\n"
    "                  [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]\n"
    "                  [fen <FEN...>]\n"
    "  euclid_cli nn_make_const <out_path> <cp>\n"
    "\n"
    "Notes:\n"
    "  - Scores are printed in centipawns and pawn units.\n"
    "  - A simple eval bar is shown (clamped to +/-10.00 pawns).\n"
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

// ---- Phase 19: Eval formatting helpers ----
static std::string fmt_pawns(int cp) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::showpos << std::setprecision(2) << (static_cast<double>(cp) / 100.0);
  return oss.str();
}

// Simple ASCII eval bar: clamp to +/- 10.00 pawns (1000 cp).
// Example: [-====|=====      ]
static std::string eval_bar(int cp, int halfWidth = 12, int clipCp = 1000) {
  if (halfWidth < 4) halfWidth = 4;
  if (clipCp <= 0) clipCp = 1000;

  const int clamped = std::max(-clipCp, std::min(clipCp, cp));
  const double t = static_cast<double>(clamped) / static_cast<double>(clipCp);
  const int fill = static_cast<int>(std::lround(std::abs(t) * halfWidth));

  std::string left(halfWidth, ' ');
  std::string right(halfWidth, ' ');

  if (clamped > 0) {
    for (int i = 0; i < fill && i < halfWidth; ++i) right[i] = '=';
  } else if (clamped < 0) {
    for (int i = 0; i < fill && i < halfWidth; ++i) left[halfWidth - 1 - i] = '=';
  }

  return "[" + left + "|" + right + "]";
}

static void print_eval_line(const Board& b, int scoreCp, const char* prefix) {
  std::cout
    << prefix << " "
    << scoreCp << "cp"
    << " (" << fmt_pawns(scoreCp) << ") "
    << eval_bar(scoreCp)
    << " (" << (b.side_to_move() == Color::White ? "white" : "black") << " to move)\n";
}

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty()) { usage(); return 0; }

  const std::string cmd = args[0];

  // uci
  if (cmd == "uci") {
    uci_loop();
    return 0;
  }

  // nn_make_const <out_path> <cp>
  // Writes a compatible (781->1) model that outputs ~constant cp (bias-only).
  if (cmd == "nn_make_const") {
    if (args.size() < 3) { usage(); return 1; }
    const std::string outPath = args[1];
    const int cp = to_int(args[2]);

    MLPConfig cfg;
    cfg.sizes  = {EncodedInputSpec::kTotal, 1};
    cfg.hidden = Activation::ReLU;
    cfg.output = Activation::None;

    MLP mlp(cfg);
    // Bias-only constant model (weights default to zero in this project’s NN implementation).
    auto& L = mlp.layers_mut().at(0);
    if (!L.b.empty()) L.b[0] = static_cast<float>(cp);

    std::ofstream out(outPath);
    if (!out) {
      std::cerr << "error: could not open for write: " << outPath << "\n";
      return 2;
    }
    mlp.save(out);
    if (!out.good()) {
      std::cerr << "error: failed while writing model: " << outPath << "\n";
      return 2;
    }

    std::cout << "wrote " << outPath << " const_cp " << cp << "\n";
    return 0;
  }

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
  // eval nn <model_path> [fen...]
  if (cmd == "eval") {
    size_t fenStart = 1;

    if (args.size() >= 3 && args[1] == "nn") {
      const std::string modelPath = args[2];
      if (!neural_eval_load_file(modelPath) || !neural_eval_enabled()) {
        std::cerr << "error: failed to load EvalModel or model dims mismatch: " << modelPath << "\n";
        return 2;
      }
      fenStart = 3;
    }

    Board b = board_from_args(args, fenStart);
    const int score = evaluate(b);
    print_eval_line(b, score, "eval");
    return 0;
  }

  // search [nn <model_path>] [depth <N>] [nodes <N>] [movetime <ms>] ... [fen <FEN...>]
  if (cmd == "search") {
    SearchLimits lim{};
    lim.depth = 2; // preserve CLI default

    size_t fenStart = args.size();

    for (size_t i = 1; i < args.size(); ++i) {
      const std::string& tok = args[i];

      if (tok == "fen") { fenStart = i + 1; break; }

      if (tok == "nn") {
        if (i + 1 >= args.size()) {
          std::cerr << "error: search nn requires a model path\n";
          return 2;
        }
        const std::string modelPath = args[i + 1];
        if (!neural_eval_load_file(modelPath) || !neural_eval_enabled()) {
          std::cerr << "error: failed to load EvalModel or model dims mismatch: " << modelPath << "\n";
          return 2;
        }
        ++i;
        continue;
      }

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
              << " score " << r.score << "cp"
              << " (" << fmt_pawns(r.score) << ") "
              << eval_bar(r.score)
              << " nodes " << r.nodes
              << " pv ";
    for (auto& m : r.pv) std::cout << move_to_uci(m) << ' ';
    std::cout << "\n";
    return 0;
  }

  usage();
  return 0;
}