// src/dataset.cpp

#include "euclid/dataset.hpp"

#include "euclid/encode.hpp"
#include "euclid/game.hpp"
#include "euclid/move_do.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace euclid {
namespace {

static void set_err(std::string* outErr, const std::string& msg) {
  if (outErr) *outErr = msg;
}

static float wdl_label_stm_pov(GameOutcome out, Color stm) {
  // Dataset contract: y is side-to-move POV.
  // +1 = win, 0 = draw, -1 = loss
  switch (out) {
    case GameOutcome::Draw:
      return 0.0f;
    case GameOutcome::WhiteWin:
      return (stm == Color::White) ? 1.0f : -1.0f;
    case GameOutcome::BlackWin:
      return (stm == Color::Black) ? 1.0f : -1.0f;
    case GameOutcome::Aborted:
    default:
      // If we include aborted games, we still need a finite label.
      // Using 0 keeps the dataset numerically stable.
      return 0.0f;
  }
}

static bool write_u64(std::ofstream& out, std::uint64_t v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
  return static_cast<bool>(out);
}

static bool write_f32(std::ofstream& out, float v) {
  out.write(reinterpret_cast<const char*>(&v), sizeof(v));
  return static_cast<bool>(out);
}

static bool write_f32_vec(std::ofstream& out, const std::vector<float>& v) {
  if (v.empty()) return true;
  out.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(float)));
  return static_cast<bool>(out);
}

} // namespace

bool write_selfplay_dataset(
  const std::string& outPath,
  const Board& start,
  const DatasetGenConfig& cfg,
  SearchLimits lim,
  DatasetGenStats* outStats,
  std::string* outErr
) {
  if (outStats) *outStats = DatasetGenStats{};

  std::ofstream out(outPath, std::ios::binary);
  if (!out) {
    set_err(outErr, "could not open dataset for write: " + outPath);
    return false;
  }

  // Write header with placeholder record_count, then patch it at the end.
  DatasetHeader hdr{};
  std::memcpy(hdr.magic, DATASET_MAGIC, sizeof(hdr.magic));
  hdr.version = DATASET_VERSION;
  hdr.feature_dim = static_cast<std::uint32_t>(EncodedInputSpec::kTotal);
  hdr.record_count = 0;
  hdr.flags = DATASET_FLAG_LABEL_WDL;
  hdr.reserved = 0;

  out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
  if (!out) {
    set_err(outErr, "failed writing dataset header: " + outPath);
    return false;
  }

  DatasetGenStats st{};
  st.games = static_cast<std::uint64_t>(cfg.games);

  std::uint64_t record_count = 0;

  for (int g = 0; g < cfg.games; ++g) {
    // Deterministic selfplay returns moves + final outcome.
    const GameResult gr = selfplay(start, cfg.maxPlies, lim);

    // Stats by game outcome (count the attempt regardless of includeAborted).
    switch (gr.outcome) {
      case GameOutcome::WhiteWin: st.whiteWins++; break;
      case GameOutcome::BlackWin: st.blackWins++; break;
      case GameOutcome::Draw:     st.draws++;     break;
      case GameOutcome::Aborted:  st.aborted++;   break;
      default: break;
    }

    if (gr.outcome == GameOutcome::Aborted && !cfg.includeAborted) {
      continue;
    }

    // Reconstruct positions from the move list and emit one record per position.
    Board b = start;

    // Include the terminal position as well (N moves => N+1 positions).
    for (std::size_t ply = 0; ; ++ply) {
      const std::vector<float> feat = encode_12x64(b);
      if (static_cast<int>(feat.size()) != EncodedInputSpec::kTotal) {
        set_err(outErr, "encode_12x64 returned unexpected feature size");
        return false;
      }

      const std::uint64_t key = b.hash();
      const float y = wdl_label_stm_pov(gr.outcome, b.side_to_move());

      if (!write_u64(out, key) || !write_f32(out, y) || !write_f32_vec(out, feat)) {
        set_err(outErr, "write failed while writing dataset: " + outPath);
        return false;
      }

      ++record_count;
      ++st.records;

      if (ply >= gr.moves.size()) break;

      State s{};
      do_move(b, gr.moves[ply], s);
    }
  }

  // Patch header record_count
  hdr.record_count = record_count;
  out.seekp(0, std::ios::beg);
  out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
  if (!out) {
    set_err(outErr, "failed to patch dataset header: " + outPath);
    return false;
  }

  out.flush();
  if (!out) {
    set_err(outErr, "failed flushing dataset: " + outPath);
    return false;
  }

  if (outStats) *outStats = st;
  return true;
}

} // namespace euclid
