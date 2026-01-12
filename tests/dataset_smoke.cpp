#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "euclid/dataset.hpp"
#include "euclid/fen.hpp"

using namespace euclid;

static std::uint32_t read_u32_le(std::istream& is) {
  unsigned char b[4]{0,0,0,0};
  is.read(reinterpret_cast<char*>(b), 4);
  return (std::uint32_t)b[0]
       | ((std::uint32_t)b[1] << 8)
       | ((std::uint32_t)b[2] << 16)
       | ((std::uint32_t)b[3] << 24);
}

static std::uint64_t read_u64_le(std::istream& is) {
  unsigned char b[8]{0,0,0,0,0,0,0,0};
  is.read(reinterpret_cast<char*>(b), 8);
  return (std::uint64_t)b[0]
       | ((std::uint64_t)b[1] << 8)
       | ((std::uint64_t)b[2] << 16)
       | ((std::uint64_t)b[3] << 24)
       | ((std::uint64_t)b[4] << 32)
       | ((std::uint64_t)b[5] << 40)
       | ((std::uint64_t)b[6] << 48)
       | ((std::uint64_t)b[7] << 56);
}

int main() {
  Board b;
  set_from_fen(b, STARTPOS_FEN);

  DatasetGenConfig cfg;
  cfg.games = 1;
  cfg.maxPlies = 1;          // deterministic: will always hit ply cap
  cfg.includeAborted = false;

  SearchLimits lim;
  lim.depth = 1;

  const std::filesystem::path tmp =
    std::filesystem::temp_directory_path() / "euclid_dataset_smoke.bin";

  std::string err;
  DatasetGenStats st;
  const bool ok = write_selfplay_dataset(tmp.string(), b, cfg, lim, &st, &err);
  assert(ok && "write_selfplay_dataset failed");

  // With the ply-cap treated as Draw, we must get records.
  assert(st.games == 1);
  assert(st.draws == 1);
  assert(st.aborted == 0);
  assert(st.records == 1);

  // Validate header + file size invariants.
  std::ifstream in(tmp, std::ios::binary);
  assert(in && "failed to open dataset file");

  char magic[8]{};
  in.read(magic, 8);
  assert(std::memcmp(magic, DATASET_MAGIC, 8) == 0);

  const std::uint32_t ver = read_u32_le(in);
  const std::uint32_t feat_dim = read_u32_le(in);
  const std::uint64_t rec_count = read_u64_le(in);
  const std::uint32_t flags = read_u32_le(in);
  (void)read_u32_le(in); // reserved

  assert(ver == DATASET_VERSION);
  assert(feat_dim == static_cast<std::uint32_t>(EncodedInputSpec::kTotal));
  assert((flags & DATASET_FLAG_LABEL_WDL) != 0);
  assert(rec_count == st.records);

  in.seekg(0, std::ios::end);
  const std::uint64_t file_size = static_cast<std::uint64_t>(in.tellg());

  constexpr std::uint64_t header_size = 32;
  const std::uint64_t record_size = 8 + 4 + (std::uint64_t)EncodedInputSpec::kTotal * 4;

  assert(file_size == header_size + rec_count * record_size);

  std::cout << "dataset_smoke ok records " << rec_count << " draws " << st.draws << "\n";
  return 0;
}
