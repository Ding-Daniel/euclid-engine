#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "euclid/attack.hpp"
#include "euclid/board.hpp"
#include "euclid/dataset.hpp"
#include "euclid/encode.hpp"
#include "euclid/eval.hpp"
#include "euclid/fen.hpp"
#include "euclid/game.hpp"     // GameResult + selfplay()
#include "euclid/nn.hpp"
#include "euclid/nn_eval.hpp"
#include "euclid/ort_eval.hpp"
#include "euclid/perft.hpp"
#include "euclid/search.hpp"
#include "euclid/types.hpp"
#include "euclid/uci.hpp"

using namespace euclid;

static void usage() {
  std::cout <<
    "Euclid CLI\n"
    "Usage:\n"
    "  euclid_cli uci\n"
    "\n"
    "  euclid_cli perft <depth> [fen...]\n"
    "  euclid_cli divide <depth> [fen...]\n"
    "\n"
    "  euclid_cli eval [fen...]\n"
    "  euclid_cli eval nn <model_path> [fen...]\n"
    "  euclid_cli eval ort <model.onnx> [fen...]\n"
    "\n"
    "  euclid_cli search [nn <model_path> | ort <model.onnx>] [depth <N>] [nodes <N>] [movetime <ms>]\n"
    "                  [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]\n"
    "                  [fen <FEN...>]\n"
    "\n"
    "  euclid_cli nn_make_const <out_path> <cp>\n"
    "\n"
    "  euclid_cli selfplay [nn <model_path> | ort <model.onnx>] [maxplies <N>] [depth <N>] [nodes <N>] [movetime <ms>]\n"
    "                     [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]\n"
    "                     [fen <FEN...>]\n"
    "\n"
    "  euclid_cli dataset selfplay <out_path> [games <N>] [maxplies <N>] [include_aborted <0|1>]\n"
    "                     [nn <model_path> | ort <model.onnx>] [depth <N>] [nodes <N>] [movetime <ms>]\n"
    "                     [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]\n"
    "                     [fen <FEN...>]\n"
    "\n"
    "  euclid_cli bench perft <depth> [iters <N>] [fen <FEN...>]\n"
    "  euclid_cli bench search [nn <model_path> | ort <model.onnx>] [iters <N>] [depth <N>] [nodes <N>] [movetime <ms>]\n"
    "                        [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]\n"
    "                        [fen <FEN...>]\n"
    "\n"
    "Notes:\n"
    "  - If FEN omitted, uses startpos.\n"
    "  - 'bench search' reports time + NPS based on SearchResult.nodes.\n";
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

// ---------------- Eval formatting (cp + pawn units + bar) ----------------

static std::string eval_bar(int cp) {
  // More sensitive bar than +/-1000cp so that small values show movement.
  // +/-600cp maps to full bar width.
  constexpr int kMaxCp = 600;
  constexpr int kSide = 12;
  constexpr int kTotal = kSide * 2 + 1;

  int x = std::clamp(cp, -kMaxCp, kMaxCp);
  int pos = kSide + (x * kSide) / kMaxCp; // integer step
  pos = std::clamp(pos, 0, kTotal - 1);

  std::string s(kTotal, ' ');
  s[kSide] = '|';
  if (pos != kSide) s[pos] = '=';
  return s;
}

static std::string fmt_cp(int cp) {
  std::ostringstream oss;
  const double pawns = static_cast<double>(cp) / 100.0;

  oss << cp << "cp ("
      << (pawns >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << pawns
      << ") [" << eval_bar(cp) << "]";
  return oss.str();
}

// ---------------- Search limits parsing ----------------

struct ParsedSearchArgs {
  SearchLimits lim{};
  size_t fenStart = 0;         // args index where fen begins (or args.size())
  int iters = 1;               // used by bench
};

static bool load_nn_or_die(const std::string& modelPath) {
  if (!neural_eval_load_file(modelPath) || !neural_eval_enabled()) {
    std::cerr << "error: failed to load EvalModel or model dims mismatch: " << modelPath << "\n";
    return false;
  }
  return true;
}

static bool load_ort_or_die(const std::string& modelPath) {
  ort_eval_clear();
  if (!ort_eval_load_file(modelPath) || !ort_eval_enabled()) {
#if defined(EUCLID_USE_ORT)
    std::cerr << "error: failed to load ONNX model (missing file or incompatible I/O): " << modelPath << "\n";
#else
    std::cerr << "error: ORT backend not available in this build. Rebuild with -DEUCLID_ENABLE_ORT=ON.\n";
#endif
    return false;
  }
  return true;
}

// Parses a “search-like” argument list that starts at args[startIdx] (exclusive of the command itself).
// Recognizes: nn <path>, ort <path>, depth, nodes, movetime, wtime/btime/winc/binc/movestogo, fen <FEN...>, iters <N> (optional).
static ParsedSearchArgs parse_search_like(const std::vector<std::string>& args, size_t startIdx) {
  ParsedSearchArgs out{};
  out.lim.depth = 2; // preserve prior default behavior

  out.fenStart = args.size();

  for (size_t i = startIdx; i < args.size(); ++i) {
    const std::string& tok = args[i];

    if (tok == "fen") { out.fenStart = i + 1; break; }

    if (tok == "nn") {
      if (i + 1 >= args.size()) {
        std::cerr << "error: nn requires a model path\n";
        out.fenStart = args.size();
        return out;
      }
      (void)load_nn_or_die(args[i + 1]);
      ++i;
      continue;
    }

    if (tok == "ort") {
      if (i + 1 >= args.size()) {
        std::cerr << "error: ort requires a model path\n";
        out.fenStart = args.size();
        return out;
      }
      (void)load_ort_or_die(args[i + 1]);
      ++i;
      continue;
    }

    if (tok == "iters") {
      if (i + 1 < args.size()) {
        out.iters = std::max(1, to_int(args[i + 1]));
        ++i;
      }
      continue;
    }

    if (i + 1 >= args.size()) break;
    const std::string& val = args[i + 1];

    if (tok == "depth")      { out.lim.depth = to_int(val); ++i; continue; }
    if (tok == "nodes")      { out.lim.nodes = to_u64(val); ++i; continue; }
    if (tok == "movetime")   { out.lim.movetime_ms = to_int(val); ++i; continue; }
    if (tok == "wtime")      { out.lim.wtime_ms = to_int(val); ++i; continue; }
    if (tok == "btime")      { out.lim.btime_ms = to_int(val); ++i; continue; }
    if (tok == "winc")       { out.lim.winc_ms = to_int(val); ++i; continue; }
    if (tok == "binc")       { out.lim.binc_ms = to_int(val); ++i; continue; }
    if (tok == "movestogo")  { out.lim.movestogo = to_int(val); ++i; continue; }
  }

  return out;
}

static const char* outcome_str(GameOutcome o) {
  switch (o) {
    case GameOutcome::WhiteWin: return "white_win";
    case GameOutcome::BlackWin: return "black_win";
    case GameOutcome::Draw:     return "draw";
    case GameOutcome::Aborted:  return "aborted";
    default:                    return "unknown";
  }
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
  if (cmd == "nn_make_const") {
    if (args.size() < 3) { usage(); return 1; }
    const std::string outPath = args[1];
    const int cp = to_int(args[2]);

    MLPConfig cfg;
    cfg.sizes  = {EncodedInputSpec::kTotal, 1};
    cfg.hidden = Activation::ReLU;
    cfg.output = Activation::None;

    MLP mlp(cfg);
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
  // eval ort <model.onnx> [fen...]
  if (cmd == "eval") {
    size_t fenStart = 1;

    if (args.size() >= 3 && args[1] == "nn") {
      const std::string modelPath = args[2];
      if (!load_nn_or_die(modelPath)) return 2;
      fenStart = 3;
    } else if (args.size() >= 3 && args[1] == "ort") {
      const std::string modelPath = args[2];
      if (!load_ort_or_die(modelPath)) return 2;
      fenStart = 3;
    }

    Board b = board_from_args(args, fenStart);
    const int score = evaluate(b);
    std::cout << "eval " << fmt_cp(score)
              << " (" << (b.side_to_move() == Color::White ? "white" : "black") << " to move)\n";
    return 0;
  }

  // search [nn <model_path> | ort <model.onnx>] [depth <N>] ... [fen <FEN...>]
  if (cmd == "search") {
    ParsedSearchArgs p = parse_search_like(args, 1);

    // Treat an explicit backend request as fatal if it didn't enable.
    for (size_t i = 1; i + 1 < args.size(); ++i) {
      if (args[i] == "nn") {
        if (!neural_eval_enabled()) return 2;
        break;
      }
      if (args[i] == "ort") {
        if (!ort_eval_enabled()) return 2;
        break;
      }
    }

    Board b = board_from_args(args, p.fenStart);
    auto r = search(b, p.lim);

    std::cout << "best " << move_to_uci(r.best)
              << " score " << fmt_cp(r.score)
              << " nodes " << r.nodes
              << " pv ";
    for (auto& m : r.pv) std::cout << move_to_uci(m) << ' ';
    std::cout << "\n";
    return 0;
  }

  // selfplay [nn <model_path> | ort <model.onnx>] [maxplies <N>] [depth <N>] ... [fen <FEN...>]
  if (cmd == "selfplay") {
    ParsedSearchArgs p = parse_search_like(args, 1);

    int maxPlies = 200; // default cap
    for (size_t i = 1; i + 1 < args.size(); ++i) {
      if (args[i] == "maxplies") {
        maxPlies = std::max(0, to_int(args[i + 1]));
        break;
      }
      if (args[i] == "fen") break;
    }

    for (size_t i = 1; i + 1 < args.size(); ++i) {
      if (args[i] == "nn") {
        if (!neural_eval_enabled()) return 2;
        break;
      }
      if (args[i] == "ort") {
        if (!ort_eval_enabled()) return 2;
        break;
      }
    }

    Board b = board_from_args(args, p.fenStart);
    GameResult g = selfplay(b, maxPlies, p.lim);

    std::cout << "outcome " << outcome_str(g.outcome)
              << " plies " << g.plies
              << " reason " << g.reason
              << " moves ";
    for (const auto& m : g.moves) std::cout << move_to_uci(m) << ' ';
    std::cout << "\n";
    return 0;
  }

  // dataset selfplay <out_path> ...
  if (cmd == "dataset") {
    if (args.size() < 2) { usage(); return 1; }
    const std::string sub = args[1];

    if (sub != "selfplay") {
      usage();
      return 1;
    }
    if (args.size() < 3) {
      usage();
      return 1;
    }

    const std::string outPath = args[2];

    DatasetGenConfig cfg;
    cfg.games = 1;
    cfg.maxPlies = 200;
    cfg.includeAborted = false;

    // Parse dataset-specific knobs from args[3..]
    for (size_t i = 3; i + 1 < args.size(); ++i) {
      if (args[i] == "games") {
        cfg.games = std::max(1, to_int(args[i + 1]));
        ++i;
        continue;
      }
      if (args[i] == "maxplies") {
        cfg.maxPlies = std::max(0, to_int(args[i + 1]));
        ++i;
        continue;
      }
      if (args[i] == "include_aborted") {
        cfg.includeAborted = (to_int(args[i + 1]) != 0);
        ++i;
        continue;
      }
      if (args[i] == "fen") break;
    }

    ParsedSearchArgs p = parse_search_like(args, 3);

    // Treat an explicit backend request as fatal if it didn't enable.
    for (size_t i = 3; i + 1 < args.size(); ++i) {
      if (args[i] == "nn") {
        if (!neural_eval_enabled()) return 2;
        break;
      }
      if (args[i] == "ort") {
        if (!ort_eval_enabled()) return 2;
        break;
      }
    }

    Board b = board_from_args(args, p.fenStart);

    DatasetGenStats st;
    std::string err;
    if (!write_selfplay_dataset(outPath, b, cfg, p.lim, &st, &err)) {
      std::cerr << "error: dataset generation failed: " << err << "\n";
      return 2;
    }

    std::cout
      << "dataset selfplay wrote " << st.records << " records"
      << " games " << st.games
      << " W " << st.whiteWins
      << " B " << st.blackWins
      << " D " << st.draws
      << " A " << st.aborted
      << " -> " << outPath << "\n";
    return 0;
  }

  // bench perft <depth> [iters <N>] [fen <FEN...>]
  // bench search [nn <model_path> | ort <model.onnx>] [iters <N>] [depth <N>] ... [fen <FEN...>]
  if (cmd == "bench") {
    if (args.size() < 2) { usage(); return 1; }
    const std::string sub = args[1];

    if (sub == "perft") {
      if (args.size() < 3) { usage(); return 1; }
      const int depth = to_int(args[2]);

      int iters = 1;
      size_t fenStart = 3;

      // Optional: iters <N> and/or fen <...>
      for (size_t i = 3; i < args.size(); ++i) {
        if (args[i] == "iters" && i + 1 < args.size()) {
          iters = std::max(1, to_int(args[i + 1]));
          ++i;
          fenStart = i + 1;
          continue;
        }
        if (args[i] == "fen") {
          fenStart = i + 1;
          break;
        }
      }

      Board b = board_from_args(args, fenStart);

      std::uint64_t lastNodes = 0;
      double totalSec = 0.0;

      for (int k = 0; k < iters; ++k) {
        auto t0 = std::chrono::steady_clock::now();
        lastNodes = perft(b, depth);
        auto t1 = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = t1 - t0;
        totalSec += dt.count();
      }

      const double avgSec = totalSec / static_cast<double>(iters);
      const double nps = (avgSec > 0.0) ? (static_cast<double>(lastNodes) / avgSec) : 0.0;

      std::cout << "bench perft depth " << depth
                << " iters " << iters
                << " nodes " << lastNodes
                << " avg_sec " << std::fixed << std::setprecision(6) << avgSec
                << " nps " << static_cast<std::uint64_t>(nps)
                << "\n";
      return 0;
    }

    if (sub == "search") {
      ParsedSearchArgs p = parse_search_like(args, 2);

      for (size_t i = 2; i + 1 < args.size(); ++i) {
        if (args[i] == "nn") {
          if (!neural_eval_enabled()) return 2;
          break;
        }
        if (args[i] == "ort") {
          if (!ort_eval_enabled()) return 2;
          break;
        }
      }

      Board b = board_from_args(args, p.fenStart);

      std::uint64_t lastNodes = 0;
      int lastScore = 0;
      Move lastBest{};
      std::vector<Move> lastPv;

      double totalSec = 0.0;

      for (int k = 0; k < p.iters; ++k) {
        auto t0 = std::chrono::steady_clock::now();
        SearchResult r = search(b, p.lim);
        auto t1 = std::chrono::steady_clock::now();

        std::chrono::duration<double> dt = t1 - t0;
        totalSec += dt.count();

        lastNodes = r.nodes;
        lastScore = r.score;
        lastBest = r.best;
        lastPv = r.pv;
      }

      const double avgSec = totalSec / static_cast<double>(p.iters);
      const double nps = (avgSec > 0.0) ? (static_cast<double>(lastNodes) / avgSec) : 0.0;

      std::cout << "bench search iters " << p.iters
                << " best " << move_to_uci(lastBest)
                << " score " << fmt_cp(lastScore)
                << " nodes " << lastNodes
                << " avg_sec " << std::fixed << std::setprecision(6) << avgSec
                << " nps " << static_cast<std::uint64_t>(nps)
                << " pv ";
      for (auto& m : lastPv) std::cout << move_to_uci(m) << ' ';
      std::cout << "\n";
      return 0;
    }

    usage();
    return 1;
  }

  usage();
  return 0;
}
