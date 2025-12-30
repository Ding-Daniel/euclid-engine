{\rtf1\ansi\ansicpg1252\cocoartf2865
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fnil\fcharset0 Menlo-Regular;}
{\colortbl;\red255\green255\blue255;\red86\green117\blue195;}
{\*\expandedcolortbl;;\cssrgb\c40859\c54320\c80786;}
\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\deftab720
\pard\pardeftab720\partightenfactor0

\f0\fs24 \cf0 \expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 Update \'97 Decisions & Specs (2025-09-10)\
\
This section captures the latest project decisions provided by the project owner on September 10, 2025. It is intended to supplement the original roadmap with locked scope, targets, and implementation choices.\
\
Goals & scope\
\
Primary success metric: Feature completeness by Week 12 \'97 all required features working and tested (FEN I/O, legal move generation, make/unmake, Alpha-Beta+quiescence, neural eval integrated, CLI with eval output).\
\
Secondary A (quality): Elo proxy via self-play \'97 target ~1600\'961800 Elo on a standard engine tournament after internal tuning.\
\
Secondary B (performance): Nodes/sec target depends on language: C++/Rust \uc0\u8805 1M nps in typical positions at shallow depths; Python expected lower unless native move gen/JIT is used.\
\
UCI vs CLI: Primary deliverable is CLI-only. Strong recommendation to add a minimal UCI layer later for GUI/testing integration.\
\
Language, stack, deployment\
\
Engine core: C++ (or Rust). Training: Python + PyTorch.\
\
Inference integration: PyTorch \uc0\u8594  ONNX export; embed ONNX Runtime (CPU first). Consider TensorRT or vendor runtimes only after profiling.\
\
Data & labeling\
\
PGN sources: Prefer Lichess monthly exports (rated, classical/rapid retained as metadata).\
\
Labels: Centipawn evaluations from a strong external engine at consistent depth/time. Optional extra WDL head later.\
\
Dataset size & splits: 10M\'9630M unique positions (start with 1\'963M for iteration). Deduplicate by position hash/canonical FEN. Split 80/10/10 by game, stratify by rating/time-control.\
\
Board state & rules\
\
Representation: Bitboards.\
\
Rules completeness: Implement 50-move rule, threefold repetition, and insufficient material early.\
\
Zobrist: Implement from day one (repetition checks + TT + dataset dedup).\
\
Move generation & correctness\
\
Perft gates: Must match canonical perft counts for startpos + Kiwipete + two additional standard positions to depth 5 (exact equality).\
\
Tables: Use precomputed masks; start with simple sliders or magic bitboards (preferred) for rook/bishop.\
\
Search design\
\
V1 must-haves: Negamax Alpha\'96Beta + quiescence, Transposition Table, iterative deepening, basic move ordering (MVV/LVA for captures).\
\
Next wave: Killer/history, LMR, aspiration windows, null-move pruning with care.\
\
Time management: depth/nodes/time modes; iterative deepening with soft/hard cutoffs.\
\
Evaluation model\
\
Phase 1: Simple MLP over 12\'d764 planes; centipawn head (clip \'b13000cp). Color-flip augmentation only.\
\
Phase 2 option: Consider NNUE-style incrementally updatable network for speed after MVP.\
\
Training logistics\
\
Hardware: Dev on \uc0\u8805 12 GB GPU (fast iteration). Full scale prefers \u8805 24 GB or multi-GPU.\
\
Losses: Huber/MAE for centipawns; CE for WDL if multi-head; regularization if needed.\
\
Monitoring: Validation MAE (pawn units), sign accuracy; periodic short self-play matches; curated test suite.\
\
Integration & tooling\
\
Engine \uc0\u8596  model: Batch leaf evals; eval-cache keyed by Zobrist; scores are from side-to-move perspective (negamax handles sign).\
\
Telemetry/Elo: Lightweight self-play harness; track nps, eval magnitudes, W/D/L; regression detection.\
\
Deliverables & packaging\
\
Platforms: Linux/macOS first; Windows later.\
\
Packaging: Static native binaries; Python wheels for training/tooling; optional Homebrew formula.\
\
Licensing: MIT (tentative). Do not redistribute third-party PGNs; ship download scripts and verify source licenses.\
\
Week 1\'962 Execution Checklists (added)\
\
Week 1 \'97 Core skeleton & correctness harness\
\
Repo + CMake + CI (Linux/macOS); clang-tidy.\
\
FEN parse/print; Zobrist gen; position equality via hash + state.\
\
Make/unmake with castling/ep/50-move/hash.\
\
Pseudo-legal movegen; legality filtering via check detection.\
\
Perft harness + CLI.\
\
DoD: startpos perft depths 1\'965; Kiwipete 1\'964; two other positions 1\'964.\
\
Week 2 \'97 Search bring-up & perf baseline\
\
Negamax Alpha\'96Beta + quiescence; iterative deepening; TT (Zobrist keys).\
\
Basic move ordering (captures first, MVV/LVA).\
\
Limits: depth/nodes/time; CLI go command.\
\
DoD: stable best-move output; perft unchanged; baseline nps recorded.\
\
Building a Neural Chess Engine: Step-by-Step Guide\
\
This guide outlines the full process of building a neural-network-based\
chess engine from scratch. It is structured into phases by weeks to\
provide a practical schedule.\
\
Week 1 -- Data Collection and Preparation\
\
Phase 1: Gather and Prepare Training Data\
\
Download large chess game databases (e.g., Lichess monthly PGNs).\
\
Parse PGN files to extract games.\
\
Convert moves into board positions with results or evaluation scores.\
\
Clean the dataset by filtering legal moves and valid positions.\
\
Decide on training targets: game outcomes (win/draw/loss) or centipawn evaluations.\
\
Week 2 -- Board Representation and FEN Parsing\
\
Phase 2: Internal Representation\
\
Choose between bitboards or array-based representation.\
\
Define piece codes (e.g., integers for each piece type).\
\
Store side to move, castling rights, en passant target, halfmove/fullmove counters.\
\
Phase 3: FEN Parsing\
\
Implement parser for Forsyth--Edwards Notation (FEN).\
\
Parse ranks, pieces, side to move, castling rights, and en passant square.\
\
Build function to set up the internal board state from FEN.\
\
Week 3 -- Move Generation\
\
Phase 4: Generate Pseudo-Legal Moves\
\
Implement move generation for each piece type (pawn, knight, bishop, rook, queen, king).\
\
Handle special rules: pawn promotion, en passant, castling.\
\
Phase 5: Filter Legal Moves\
\
Test moves by making and unmaking them.\
\
Remove moves that leave the king in check.\
\
Week 4 -- Move Application\
\
Phase 6: Make and Unmake Moves\
\
Implement functions to update board state when a move is made.\
\
Store captured pieces, castling rights, and other state details on a stack.\
\
Implement an unmake function to restore the previous state.\
\
Week 5 -- Search Algorithm (Minimax and Alpha-Beta)\
\
Phase 7: Search Framework\
\
Implement negamax recursion for minimax search.\
\
Integrate alpha-beta pruning to eliminate unnecessary branches.\
\
Phase 8: Base Case (Evaluation)\
\
At depth 0, return evaluation function value.\
\
Week 6 -- Quiescence and Search Enhancements\
\
Phase 9: Quiescence Search\
\
Implement search extension for captures at leaf nodes to avoid horizon effect.\
\
Phase 10: Search Enhancements (Optional)\
\
Transposition tables.\
\
Iterative deepening.\
\
Move ordering heuristics.\
\
Aspiration windows.\
\
Week 7 -- Neural Network Evaluation Function\
\
Phase 11: Input Encoding\
\
Encode board as features (e.g., 12\'d764 binary inputs for pieces).\
\
Add features for side-to-move, castling rights, en passant.\
\
Phase 12: Neural Network Architecture\
\
Build a feedforward neural network with several hidden layers.\
\
Use ReLU or tanh for hidden activations, linear for output.\
\
Phase 13: Training Setup\
\
Train on extracted positions with labels (game results or centipawn scores).\
\
Use mean squared error or L1 loss.\
\
Train with GPU acceleration if available.\
\
Week 8 -- Training the Neural Network\
\
Phase 14: Dataset and Loader\
\
Implement efficient data pipeline for large-scale training.\
\
Phase 15: Training and Saving\
\
Train until convergence.\
\
Save model weights.\
\
Verify outputs on known test positions.\
\
Week 9 -- Integration with Search\
\
Phase 16: Neural Net Integration\
\
Load trained model in the engine.\
\
Replace evaluation function with neural network outputs.\
\
Ensure correct scaling (centipawns, pawn units).\
\
Phase 17: Negamax Adaptation\
\
Ensure scores are negated correctly when changing perspective.\
\
Week 10 -- Command-Line Interface\
\
Phase 18: CLI Development\
\
Accept FEN and depth as input arguments.\
\
Run search from given position.\
\
Output evaluation score, best move, and optional principal variation.\
\
Phase 19: Eval Bar Representation\
\
Print evaluation as centipawns or pawn units (e.g., +0.53 = half-pawn advantage for White).\
\
Week 11 -- Testing and Debugging\
\
Phase 20: Unit Tests\
\
Verify move generation with perft results for known positions.\
\
Test special cases (castling, en passant, promotions).\
\
Phase 21: Evaluation Sanity Checks\
\
Compare evaluations with known engines.\
\
Ensure winning/losing positions are scored correctly.\
\
Phase 22: Full Games\
\
Have engine play complete games against itself or other engines.\
\
Confirm correctness and stability.\
\
Week 12 -- Refinement\
\
Phase 23: Optimize\
\
Profile performance.\
\
Optimize move generation and evaluation calls.\
\
Experiment with deeper search and larger networks.\
\
Phase 24: Prepare for Distribution\
\
Package engine for desktop CLI use.\
\
Document usage instructions.\
\
Conclusion\
\
By following this 12-week phased roadmap, you will develop a complete\
neural-network-based chess engine with FEN support, alpha-beta search,\
quiescence, and an evaluation bar.\
}