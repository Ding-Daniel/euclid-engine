# Euclid Engine

A modern C++ chess engine with a single command-line binary (`euclid_cli`) that supports:

* **UCI mode** (plug into chess GUIs)
* **Move generation + perft/divide**
* **Classical evaluation**
* **Neural evaluation** (native model or ONNX Runtime)
* **Alpha-beta search** with common time controls
* **Self-play** and **dataset generation**
* Built-in **bench** commands

> This repository is source-first: you build the engine locally and run `euclid_cli`.

---

## Contents 

* [Requirements](#requirements)
* [Download](#download)
* [Build](#build)
* [Test](#test)
* [Run](#run)
* [UCI mode (GUI integration)](#uci-mode-gui-integration)
* [Command reference](#command-reference)
* [Examples](#examples)
* [Neural evaluation](#neural-evaluation)
* [Datasets](#datasets)
* [Benchmarks](#benchmarks)
* [Stats for Nerds](#stats-for-nerds)
* [Troubleshooting](#troubleshooting)

---

## Requirements

### Build tooling 🛠️

* **CMake** (modern CMake recommended)
* A C++ compiler with **C++17** support (AppleClang/Clang/GCC)

### Optional (for ONNX Runtime evaluation)

If you plan to use the `ort` subcommands (e.g., `eval ort`, `search ort`, etc.), you need ONNX Runtime available to the build.

On macOS (Homebrew), a common approach is:

```bash
brew install onnxruntime
```

If your environment is different, install ONNX Runtime using your platform’s preferred package manager or from upstream binaries.

---

## Download ‼️

Clone the repository:

```bash
git clone <REPOSITORY_URL>
cd euclid-engine
```

Replace `<REPOSITORY_URL>` with your GitHub repository URL.

---

## Build 💻

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The main executable will be:

* `build/euclid_cli`

---

## Test ‼️

Run the full test suite:

```bash
ctest --test-dir build --output-on-failure
```

---

## Run ‼️

Print help:

```bash
./build/euclid_cli
```

---

## UCI mode (GUI integration)

Euclid runs as a UCI engine when invoked with the `uci` subcommand:

```bash
./build/euclid_cli uci
```

This command starts an **interactive loop** and will wait for UCI commands on stdin (it does not “finish” by itself).

### Exiting UCI mode (manual)

If you launched it in a terminal, you can type:

*quit*

### Using with chess GUIs

Most GUIs let you configure an engine command with arguments.

* **Engine command:** path to `euclid_cli`
* **Arguments:** `uci`

If your GUI does not support arguments, create a small wrapper script that runs `euclid_cli uci` and point the GUI to that script.

---

## Command reference

`euclid_cli` provides the following top-level commands:

```text
Euclid CLI
Usage:
  euclid_cli uci

  euclid_cli perft <depth> [fen...]
  euclid_cli divide <depth> [fen...]

  euclid_cli eval [fen...]
  euclid_cli eval nn <model_path> [fen...]
  euclid_cli eval ort <model.onnx> [fen...]

  euclid_cli search [nn <model_path> | ort <model.onnx>] [depth <N>] [nodes <N>] [movetime <ms>]
                  [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]
                  [fen <FEN...>]

  euclid_cli nn_make_const <out_path> <cp>

  euclid_cli selfplay [nn <model_path> | ort <model.onnx>] [maxplies <N>] [depth <N>] [nodes <N>] [movetime <ms>]
                     [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]
                     [fen <FEN...>]

  euclid_cli dataset selfplay <out_path> [games <N>] [maxplies <N>] [include_aborted <0|1>]
                     [nn <model_path> | ort <model.onnx>] [depth <N>] [nodes <N>] [movetime <ms>]
                     [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]
                     [fen <FEN...>]

  euclid_cli bench perft <depth> [iters <N>] [fen <FEN...>]
  euclid_cli bench search [nn <model_path> | ort <model.onnx>] [iters <N>] [depth <N>] [nodes <N>] [movetime <ms>]
                        [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <N>]
                        [fen <FEN...>]

Notes:
  - If FEN omitted, uses startpos.
  - 'bench search' reports time + NPS based on SearchResult.nodes.
```

---

## Examples

### Perft

Start position, depth 5:

```bash
./build/euclid_cli perft 5
```

Perft on a specific FEN:

```bash
./build/euclid_cli perft 5 "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
```

### Divide

```bash
./build/euclid_cli divide 4
```

### Classical evaluation

```bash
./build/euclid_cli eval
```

### Search

Search the start position for a fixed depth:

```bash
./build/euclid_cli search depth 8
```

Search with a move time budget (milliseconds):

```bash
./build/euclid_cli search movetime 1000
```

Search with time controls:

```bash
./build/euclid_cli search wtime 60000 btime 60000 winc 0 binc 0 movestogo 40
```

Search a custom FEN:

```bash
./build/euclid_cli search depth 10 fen "<FEN_STRING>"
```

### Self-play

Run a lightweight self-play game (depth-limited example):

```bash
./build/euclid_cli selfplay depth 2 maxplies 40
```

### Dataset generation (self-play)

Generate a small binary dataset:

```bash
./build/euclid_cli dataset selfplay /tmp/euclid_selfplay.bin games 2 maxplies 40 include_aborted 1 depth 2
```

---

## Neural evaluation

Euclid supports two neural evaluation backends:

1. **Native model** (`nn`)
2. **ONNX Runtime** (`ort`)

### Evaluate with a native model

```bash
./build/euclid_cli eval nn <model_path>
```

### Evaluate with an ONNX model

```bash
./build/euclid_cli eval ort <model.onnx>
```

### Search using a neural model

Native model:

```bash
./build/euclid_cli search nn <model_path> movetime 1000
```

ONNX Runtime:

```bash
./build/euclid_cli search ort <model.onnx> movetime 1000
```

### Self-play / dataset generation using a neural model

Native model:

```bash
./build/euclid_cli selfplay nn <model_path> movetime 50 maxplies 200
./build/euclid_cli dataset selfplay out.bin games 100 nn <model_path> movetime 50
```

ONNX Runtime:

```bash
./build/euclid_cli selfplay ort <model.onnx> movetime 50 maxplies 200
./build/euclid_cli dataset selfplay out.bin games 100 ort <model.onnx> movetime 50
```

---

## Datasets

`dataset selfplay` writes a **binary** dataset file.

* For the exact on-disk format and record layout, see the dataset definitions in the source tree (e.g., the dataset header/record structures in the `include/` headers).

Recommended workflow:

1. Generate data via self-play:

   * `euclid_cli dataset selfplay <out_path> ...`
2. Train a model externally.
3. Use the resulting model file with:

   * `eval nn <model_path>` / `search nn <model_path>`
   * or export to ONNX and use `eval ort <model.onnx>` / `search ort <model.onnx>`

---

## Benchmarks

### Bench perft

```bash
./build/euclid_cli bench perft 6 iters 10
```

### Bench search

```bash
./build/euclid_cli bench search iters 10 movetime 100
```

Neural search benchmark:

```bash
./build/euclid_cli bench search ort <model.onnx> iters 10 movetime 100
```

---

## Stats for Nerds 🤓

- **Core representation**: bitboards, precomputed attack tables for fast move generation.
- **Hashing**: Zobrist hashing for position keys.
- **Search framework**:
  - iterative deepening alpha–beta
  - transposition table (TT)
  - quiescence search (tactical stabilization)
  - time controls and hard limits (depth/nodes/movetime/time management)
  - null-move pruning
- **Move ordering (typical stack)**:
  - TT move / PV move preference
  - captures (with a static exchange/tactical bias)
  - history/killers style heuristics (where applicable)
- **Draw rules**: repetition and 50-move style rules.
- **Neural evaluation support**:
  - custom compact NN model format (`eval nn`)
  - constant-model generator for integration (`nn_make_const`)
  - optional ONNX Runtime path (`eval ort`)
- **Self-play tooling**: engine-driven self-play producing game trajectories.
- **Supervised dataset generator**:
  - writes WDL labels from side-to-move POV (`+1/0/-1`)
  - `encode_12x64` feature spec (12×64 piece planes + STM scalar)

---

## Troubleshooting

### `./build/euclid_cli uci` “hangs”

That is expected. UCI mode is an interactive protocol loop; it waits for commands from stdin/a GUI. Type `quit` to exit when running manually.

### Linker errors mentioning `onnxruntime` / missing `ort` symbols

If you enabled or built with ONNX Runtime support, ensure ONNX Runtime is installed and discoverable by your build system.

Common remediation steps:

* Install ONNX Runtime (e.g., `brew install onnxruntime` on macOS).
* Clean rebuild:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Clean rebuild

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

*Check out my repository for more projects! :)*