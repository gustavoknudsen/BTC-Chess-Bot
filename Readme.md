
# BetterThanCris Chess Bot in C(++)
 - Bot was written primarily in C but utilises a few C++ features. The source code is in a `.cpp` file but follows C programming conventions and structure.
## Table of Contents 
 1. [Introduction](#introduction)
 2. [Strength](#strength)  
 3. [Features](#features)
 4. [To Do](#to-do)  
 5. [Play BetterThanCris](#play-betterthancris)
 6. [Releases](#releases) 
 7. [Credits](#credits)
## Introduction
 - Aggressive & Entertaining Chess
 - Created by Gustavo Knudsen
 - UCI Protocol Chess Engine
 - Current Version: 2.3
 - Is, In Fact, Better Than Cris
## Strength
**Version 2.3**
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
 - History Moves
 - Killer Moves
 - Transposition Table w/ Zobrist Hashing
 - Hash Move Ordering
 - Repetitions

**Evaluation**
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

## To Do
- Improve King Safety Evaluation
	- Defending Pieces | Checks | Mobility
- Add Syzygy EGTB
- Improve Endgames
	- Blockage | Draws | Known Endgames
- Improve General Evaluation
	- Outposts | Strong Squares | Piece Attacks | Passed Pawn Improvement | Trapped Pieces
- Improve Time Management
- Add Pattern Evaluations
- Improve Mobility
- Add Space Evaluation
- Add Pondering Option
- Add Parallel Search / Shared Hash Table

 
## Play BetterThanCris
 - If online, can be played on  [Lichess](https://lichess.org/@/BetterThanCris) 
 - Can also be downloaded and ran like a normal UCI engine locally on a GUI
## Releases
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
 - PeSTO's Evaluation Function by [Ronald Friederich](https://www.chessprogramming.org/Ronald_Friederich "Ronald Friederich")
 - TSCP by [Tom Kerrigan](https://www.chessprogramming.org/Tom_Kerrigan "Tom Kerrigan") for Inspiration and Guidance
 - Gaurav Pant's [Improving Search](https://www.youtube.com/watch?v=mVdh5z0jtAo&t=3993s&ab_channel=GauravPant) for Better Search
 - Sebastian Lague's [Chess Coding Adventure](https://youtube.com/playlist?list=PLFt_AvWsXl0cvHyu32ajwh2qU1i6hl77c&si=CyULwCJeNlQHIrk6)
 - Eddie Sharick's [Creating a Chess Engine in Python](https://youtube.com/playlist?list=PLBwF487qi8MGU81nDGaeNE1EnNEPYWKY_&si=q9vOJjGycpV9yHHK)
 - Bluefever Software's OG [Chess Engine in C](https://youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg&si=KjBemyRplDQps77r)
 - [Xorshift Algorithm](https://en.wikipedia.org/wiki/Xorshift)
