# BetterThanCris Chess Bot in C(++)
- Bot was written primarily in C but utilises a few C++ features. The source code is in `.cpp` files but follows C programming conventions and structure.

## Table of Contents
1. [Introduction](#introduction)
2. [Strength](#strength)
3. [Features](#features)
4. [Building & Running](#building--running)
5. [Project Layout](#project-layout)
6. [To Do](#to-do)
7. [Play BetterThanCris](#play-betterthancris)
8. [Releases](#releases)
9. [Credits](#credits)

## Introduction
- Aggressive & Entertaining Chess
- Created by Gustavo Knudsen
- UCI Protocol Chess Engine
- Current Version: 2.4
- Is, In Fact, Better Than Cris

## Strength
**Version 2.4**
- [CCRL](https://www.computerchess.org.uk/ccrl/404/) Blitz Rating [Estimate](https://docs.google.com/spreadsheets/d/1t2gDEfoMDtqAA5uL9U_GPA9CijjlMrVK4AR4DiAqqGU/edit?usp=sharing): 2071 ± 20
- [Lichess](https://lichess.org/@/BetterThanCris) (Playing Almost Exclusively Against Other Bots):
    - Bullet: 2206 Peak, Blitz: 2101 Peak
- Against Humans, Especially in Bullet or Blitz, Rating can be Expected to be Higher
- [Sample Games](https://www.chess.com/c/2j3KdGsGr)

## Features

**General Features:**
- UCI Protocol
- Bitboard Board Representation
- Pre-Calculated Attack Tables:
    - Pre-Calculated Attack Tables Generator (Off by Default)
- Magic Bitboards:
    - Magic Number Generator (Off by Default)
    - Sliding Pieces
- Simple Time Management
- Perft Test

**Search:**
- Negamax Search w/ Alpha Beta Pruning
- Quiescence Search
- Iterative Deepening
- Aspiration Windows
- Move Ordering
- Principle Variation Search (PVS)
- PV Node Pruning
- Null Move Pruning
- Reverse Futility Pruning (RFP) / Static Null Move Pruning
- Razoring
- Late Move Reductions (LMR)
- Mate Distance Pruning
- History Moves
- Killer Moves
- Transposition Table w/ Zobrist Hashing
- Hash Move Ordering
- Repetitions

**Evaluation:**
- Material Evaluation
- Piece-Square Tables
- Tapered Evaluation
- Pawn Structure:
    - Double Pawns, Isolated Pawns, Backward Pawns
    - Connectivity: Phalanx & Supported Pawn Chains
    - Passed Pawns
- Mobility w/ X-Rays
- Rook Open / Semi-Open Files
- King Safety:
    - Attackers & Attackers Value
    - King Ring / Shelter Strength / Unblocked Pawn Storm
- Tempo

## Building & Running

The project builds with `g++` (tested with MSYS2 g++ 15.1.0 on Windows 11) and GNU `make` (tested with GNU Make 4.4.1).

```
make             # builds engine.exe (-O2)
make build       # same as above
make debug       # builds engine_debug.exe (-O0 -g)
make perft       # builds test_perft.exe (perft driver)
make test        # runs perft on 4 positions and checks canonical node totals
make clean       # removes build artefacts
```

To run the engine over UCI:

```
./engine.exe
uci
isready
position startpos
go depth 8
```

Or drop `engine.exe` into any UCI-compatible GUI (Arena, Cute Chess, Lucas Chess, etc.).

## Project Layout

```
src/
    defs.h              shared macros, enums, FEN constants
    version.h           engine name, version, author (bump version here)
    bitboard.{h,cpp}    bit utilities (set/get/pop, count, shift, BSF/BSR)
    random.{h,cpp}      xorshift32 / random64 / magic-number RNG
    attacks.{h,cpp}     leaper masks, slider OTF attacks, magic tables
    magic.{h,cpp}       magic number generation (not run by default)
    position.{h,cpp}    board state globals, parseFEN / printBoard
    zobrist.{h,cpp}     piece/enpassant/castle/side keys + hash generator
    move.{h,cpp}        move encoding macros, move list, printMove
    copy_make.h         copyBoard / undoBoard macros
    movegen.{h,cpp}     generateMoves, isUnderAttack
    make_move.h         makeMove
    perft.{h,cpp}       perftDriver / perftTest
    eval_constants.{h,cpp}  all evaluation tables and tunable constants
    evaluation.{h,cpp}  per-piece evaluators, evaluate() entry point,
                        king safety, position cache
    tt.{h,cpp}          transposition table, probeHash / recordHash
    search.{h,cpp}      negamax, quiescence, move ordering,
                        iterative deepening
    timeman.{h,cpp}     time control globals, getTime / communicate
    uci.{h,cpp}         parseMove / parsePosition / parseGo / uciLoop
    init.{h,cpp}        initAll
    main.cpp            entry point

tests/
    test_perft.cpp      perft driver linked against src/
```

## To Do
- Improve King Safety Evaluation
    - Defending Pieces | Checks | Mobility
- Add Static Exchange Evaluation (SEE) for move ordering and quiescence pruning
- Add Syzygy EGTB
- Improve Endgames
    - Blockage | Draws | Known Endgames
- Improve General Evaluation
    - Outposts | Strong Squares | Piece Attacks | Passed Pawn Improvement | Trapped Pieces
- Stockfish-style Time Management (best-move stability, fail-low extension, soft / hard split)
- Better TT replacement scheme (depth-preferred / age-based)
- Aspiration window widening on fail
- Add Pattern Evaluations
- Improve Mobility
- Add Space Evaluation
- Add Pondering Option
- Add Parallel Search / Shared Hash Table

## Play BetterThanCris
- If online, can be played on [Lichess](https://lichess.org/@/BetterThanCris)
- Can also be downloaded and ran like a normal UCI engine locally on a GUI

## Releases

**Version 2.4 - 24/05/2026**
- Refactor: split single-file engine into modular `src/` layout
- Search bug fixes:
    - LMR formula now uses move index, not the game halfmove counter
    - TT no longer wiped between searches, so entries persist across moves
    - Mate distance pruning added
    - Root now probes TT for move ordering
    - Reverse Futility Pruning guard fixed (previously always true, allowed pruning in mate space)
    - Draw / 50-move check no longer can suppress bestmove at the root
    - Removed uninitialized variable feeding into recordHash
- Evaluation bug fixes:
    - `mobilityBonus` no longer reads out of bounds in middlegame
    - Bishop and knight evaluation use the correct piece-type indices
    - Black king pawn-storm penalty now reads white pawns (was reading own pawns)
    - Static-init bug fixed: `centerFiles`, `outpostRanksWhite`, `outpostRanksBlack` are now built after the file/rank masks
    - Removed eval scaling by fifty-move counter (caused TT / eval inconsistency)
    - Position-cache now keyed by full 64-bit hash, not a truncated int
- Time management:
    - Formula now uses `moveNumber` (game progress), not search ply
    - Hard cap at `maximumTime`, clamped low-clock emergency stop
- UCI / robustness:
    - `stop` now stops the search, not the engine
    - Input during search no longer aborts on non-stop / non-quit commands
    - `parsePosition` handles arbitrary whitespace between moves and checks `makeMove` legality
    - Defensive cleanups: `encodeMove` macro arg parens, zobrist re-seed, `pawnAttacks` bounds guards

**Version 2.3 - 25/09/2024**
- Added Mobility w/ X-Rays to Evaluation
- Improved King Evaluation by Adding:
    - King Ring / Shelter Strength / Unblocked Pawn Storm
- Added Tempo Bonus to Evaluation
- Slight Adjustment to LMR

**Version 2.2 - 23/07/2024**
- Improved LMR
- Added Attackers & Attackers Weight to King Evaluation
- Added Hash Move Ordering
- Added Pawn Structure Evaluation
- Increased Weight of Passed Pawns
- Improved Bitboard Macro Speeds
- Added RFP / Static Null Move Pruning
- Added Razoring
- Repetitions
- Fixed Bug in Time Management

**Version 2.1 - 03/07/2024**
- Added Tapered Evaluation
- Added Transposition Table
- Added Basic LMR
- Added History Moves
- Added Killer Moves
- Added PVS
- Added Null Move Pruning
- Added Aspiration Windows

**Version 2.0 - 23/06/2024**
- First Version of Bot in C
- Added Negamax Search w/ Alpha Beta Pruning
- Added Quiescence Search
- Added Iterative Deepening
- Added Simple Time Management
- Added Material Evaluation
- Added Piece-Square Tables
- Added Rook Open / Semi-Open Files
- Added Basic King Safety

**Version 1.0 - 01/05/2024**
- Bot Written in Python with Built-In GUI (Not UCI)
- Numpy Array Representation
- Negamax Search w/ Alpha Beta Pruning
- Null Move Pruning
- Fixed Depth Search
- Material Evaluation
- Piece-Square Tables
- Simple Opening Book

## Credits
I would have never been able to complete this project without the help of these resources:
- Code Monkey King's [Bitboard CHESS ENGINE in C](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) (Based Off Of)
- [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) (For Almost Everything)
- UCI Communication from Richard Allbert
- Stockfish 10 [SRC](https://github.com/mcostalba/Stockfish/tree/master) for Inspiration of Evaluation and Evaluation Values
- Strelka Chess Engine [SRC](https://github.com/FireFather/strelka) for Razoring
- PeSTO's Evaluation Function by [Ronald Friederich](https://www.chessprogramming.org/Ronald_Friederich)
- TSCP by [Tom Kerrigan](https://www.chessprogramming.org/Tom_Kerrigan) for Inspiration and Guidance
- Gaurav Pant's [Improving Search](https://www.youtube.com/watch?v=mVdh5z0jtAo&t=3993s&ab_channel=GauravPant) for Better Search
- Sebastian Lague's [Chess Coding Adventure](https://youtube.com/playlist?list=PLFt_AvWsXl0cvHyu32ajwh2qU1i6hl77c&si=CyULwCJeNlQHIrk6)
- Eddie Sharick's [Creating a Chess Engine in Python](https://youtube.com/playlist?list=PLBwF487qi8MGU81nDGaeNE1EnNEPYWKY_&si=q9vOJjGycpV9yHHK)
- Bluefever Software's OG [Chess Engine in C](https://youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg&si=KjBemyRplDQps77r)
- [Xorshift Algorithm](https://en.wikipedia.org/wiki/Xorshift)
