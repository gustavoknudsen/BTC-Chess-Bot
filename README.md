
<div align="center">
  <img src="logo.png" alt="BetterThanCris" width="200">
  <h3>BetterThanCris Chess Bot</h3>
	Last Release v2.6 - 29/05/2026
</div>

---

 Bot was written primarily in C but utilises a few C++ features. Most of the code is in `.cpp` files but follows C programming conventions and structure.
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
 - Current Version: 2.6
 - Is, In Fact, Better Than Cris
## Strength
**Version 2.6**
- [CCRL](https://www.computerchess.org.uk/ccrl/404/) Blitz Rating [Estimate](https://docs.google.com/spreadsheets/d/1t2gDEfoMDtqAA5uL9U_GPA9CijjlMrVK4AR4DiAqqGU/edit?usp=sharing): 2487 ± 36
- [Lichess](https://lichess.org/@/BetterThanCris) (Playing Almost Exclusively Against Other Bots):
	- Bullet: 2414 Peak, Blitz: 2267 Peak
- Against Humans, Especially in Bullet or Blitz, Rating can be Expected to be Higher
- [Sample Games](https://www.chess.com/analysis/collection/betterthancris-2-6-samples-games-3AxpF6QA2)

**Version 2.3**
- [CCRL](https://www.computerchess.org.uk/ccrl/404/) Blitz Rating [Estimate](https://docs.google.com/spreadsheets/d/1t2gDEfoMDtqAA5uL9U_GPA9CijjlMrVK4AR4DiAqqGU/edit?usp=sharing): 2071 ± 20
- [Lichess](https://lichess.org/@/BetterThanCris) (Playing Almost Exclusively Against Other Bots):
	- Bullet: 2301 Peak, Blitz: 2267 Peak
- Against Humans, Especially in Bullet or Blitz, Rating can be Expected to be Higher
- [Sample Games](https://www.chess.com/analysis/collection/betterthancris-2-6-samples-games-3AxpF6QA2)
## Features
 **General Features:**
 - UCI Protocol
 - Bitboard Board Representation
 - Pre-Calculated Attack Tables:
	 - Pre-Calculated Attack Tables Generator (Off by Default)
 - Magic Bitboards:
	 - Magic Number Generator (Off by Default)
	 - Sliding Pieces
- Time Management:
	- Soft Target / Hard Cap Split
	- Skip Iterations That Cannot Finish in Budget
	- Best-Move-Stability Shrink + Fail-Low Extension
- Perft Test
	 
 **Search:**
 - Negamax Search w/ Alpha Beta Pruning
 - Quiescence Search:
	- Captures-Only Move Generator
	- SEE Pruning of Losing Captures
 - Iterative Deepening
 - Aspiration Windows w/ Widening on Fail
 - Move Ordering:
	- Static Exchange Evaluation (SEE)
	- Good vs Bad Capture Classification
	- MVV/LVA + Capture History
	- Insertion Sort
 - Principle Variation Search (PVS)
 - PV Node Pruning
 - Null Move Pruning
 - Reverse Futility Pruning (RFP) / Static Null Move Pruning
 - Frontier Futility Pruning (Quiet Moves at Low Depth)
 - Late Move Pruning (LMP)
 - Razoring
 - Internal Iterative Reductions (IIR)
 - Improving Heuristic (Gates LMP / RFP / LMR by `eval[ply] > eval[ply-2]`)
 - Late Move Reductions (LMR):
	- Ethereal-Style Formula
	- History-Based Reduction Adjustment
	- Reduced Less for Check-Givers
 - Mate Distance Pruning
 - Singular Extensions:
	- TT-Best Move Singularity Verification at Reduced Depth
	- +1 Ply Extension if All Other Moves Fail Low vs `ttScore - 2 * depth`
 - Gravity History:
	- Main History (Quiet Moves)
	- Capture History (By Captured Piece)
	- 1-Ply Continuation History
	- 2-Ply Continuation History
	- Carryover Across Moves Within a Game
 - Killer Moves
 - Counter-Move Heuristic
 - Transposition Table w/ Zobrist Hashing:
	- 4-Entry Buckets w/ Depth-Preferred + Generation-Aged Replacement
 - Hash Move Ordering
 - Repetitions
 - Single-Legal-Move Fast Path

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

**Search (High Impact):**
- Correction History
- ProbCut
- Pawn History / Low-Ply History
- Razoring Tuning

**Evaluation (High Impact):**
- Texel / SPSA Tuning of Eval Weights
	- Largest "Free Elo" Lever Without Eval Structure Changes
- Improve King Safety Evaluation
	- Tone Down the kingPenalty^2 / 4096 Quadratic
	- Add Defending Pieces | Safe Checks | Mobility into Safety Calc
- Complex Mobility w/ Pin Exclusion
	- Re-attempt the Existing Scaffolding (Compute Pin Info Once Per Node)
- NNUE (Long-Term)
- Improve General Evaluation
	- Outposts | Strong Squares | Piece Attacks | Passed Pawn Improvement | Trapped Pieces
- Pattern Evaluations
- Threat Evaluation
- Space Evaluation
- Improve Endgames
	- Blockage | Draws | Known Endgames

**UCI & Infrastructure:**
- Add Pondering Option
- Add Multi-PV Search
- Add Syzygy EGTB
- Add Opening Book Support
- Announce `Hash` Option in `uci` Reply (Currently Handled but Not Advertised)
- Add Parallel Search / Lazy SMP

 
## Play BetterThanCris
 - If online, can be played on  [Lichess](https://lichess.org/@/BetterThanCris) 
 - Can also be downloaded and ran like a normal UCI engine locally on a GUI
## Releases
**Version 2.6 - 29/05/2026**
- Added Late Move Pruning (LMP):
	- Skip Quiet Moves Once `movesSearched >= 3 + depth*depth` at depth <= 8
	- Only at Non-PV, Non-Check, Non-Check-Giving Nodes With a Real Alpha Baseline
- Added Frontier Futility Pruning:
	- Skip Quiet Move if `eval + 120 * depth <= alpha` at depth <= 6
- Added History-Based LMR Adjustment:
	- Ethereal-Style Base Reduction Then +- 4 Ply Nudge From mainHistory + 1-Ply Continuation History
	- Quiet Check-Givers Get One Less Ply of Reduction
- Added Aspiration Window Widening on Fail:
	- Delta Starts at 50, Doubles on Each Fail, Same Bound Widened Around the Failed Score
	- Full Window Fallback Once Delta Exceeds 800
	- Iterations No Longer Skipped on Aspiration Fail (Previous Behaviour)
- Effect of the Above Four: Roughly 71.2% Score vs 2.5
- Replaced Bubble/Selection Sort with Insertion Sort in Move Ordering:
	- Stable Tie-Breaking and Lower Per-Sort Cost on Near-Sorted Lists
	- Effect: 60% Score Over the Pre-Sort 2.6 Build, Top of a 3-Way Round-Robin
- Added Singular Extensions:
	- TT-Best Move Singularity Verification at Reduced Depth, +1 Ply Extension on Fail-Low
	- Conditions: depth >= 8, ttDepth >= depth - 3, Beta/Exact Flag, Non-Mate ttScore
	- singularBeta = ttScore - 2 * depth, Verifier at (depth - 1) / 2 with All Pruning Disabled
	- Effect: 71.1% Score Over the Pre-Singular 2.6 Build (22W 20D 3L Over 45 Games)
- Added Counter-Move Heuristic, 2-Ply Continuation History, Internal Iterative Reductions, Improving Heuristic, Best-Move-Stability Time Management, and TT 4-Entry Buckets:
	- Counter-Move: `[prevPiece][prevTo]` Refutation Table, Ordered Between Killers and History
	- 2-Ply Continuation History: Symmetric Reads, 3/4 Update Bonus on the 2-Ply Table
	- Internal Iterative Reductions (IIR): `depth--` at depth >= 6 With No TT Move
	- Improving Heuristic: `eval[ply] > eval[ply-2]` Gates the LMP Threshold, RFP Margin, and LMR
	- Best-Move-Stability Time Management: 70% Soft-Limit Shrink at 5+ Stable Best-Moves, +30% Extension on a >= 50cp Drop
	- TT 4-Entry Buckets: Depth-Preferred + Generation-Aged Replacement (`depth - 8 * (gen - age)`)
	- Effect: 64% Score (35W 58D 7L Over 100 Games) vs the Pre-Batch 2.6 Build

**Version 2.5 - 24/05/2026**
- Added Static Exchange Evaluation (SEE):
	- Bad-Capture Pruning in Quiescence
	- Good vs Bad Capture Classification in Move Ordering
- Quiescence Overhaul:
	- Captures-Only Move Generator (No More Filtering All Moves Down to Captures)
	- SEE-Based Skip of Losing Captures
- Gravity History Overhaul:
	- Replaced Flat History With Bounded Gravity Updates
	- Added Capture History Indexed by Captured Piece
	- Added 1-Ply Continuation History
	- Bonus on Cutoff, Malus on Non-Best Moves at the Same Node
	- Histories Now Carry Across Moves (Reset Only on `ucinewgame`)
- Added Single-Legal-Move Fast Path (Skip Search When Forced)
- Time Management Rewrite:
	- Separated Soft Target (`softLimit`) and Hard Cap (`stoptime`)
	- Iterative Deepening Skips Iterations That Cannot Finish in Budget
	- No Aspiration Retry After a Stopped Search
	- Roughly 35% Less Overshoot at 2min+1s Time Control
- Briefly Added Then Removed Contempt-Aware Draw Scoring
	-Contempt gave ~70 Elo loss; Reverted to Plain Draw Return

**Version 2.4 - 22/05/2026**
- Refactored Engine Into Modular `src/` Layout
- Added Mate Distance Pruning
- Fixed Critical Search Bugs:
	- LMR Was Using Game Halfmove Counter Instead of Move Index
	- Transposition Table No Longer Wiped Between Searches
	- RFP Mate-Space Guard (Was Always True, Could Prune Mates)
	- Root Now Probes TT for Move Ordering
	- Draw Check Precedence at Root
- Fixed Critical Evaluation Bugs:
	- Mobility Bonus Out-of-Bounds Read in Middlegame
	- Bishop and Knight Attack Table Indexing for Black
	- Black King Pawn-Storm Penalty (Was Reading Own Pawns)
	- Static Initialisation of Center File and Outpost Rank Masks
	- Position Cache Hash Truncation
- Removed Fifty-Move Counter Eval Scaling (Caused TT Inconsistency)
- Improved Time Management:
	- Formulas Now Use Move Number Instead of Search Ply
	- Hard Cap at Maximum Time
	- Clamped Low-Clock Emergency Stop
- Fixed UCI Handling:
	- `stop` Command Now Halts Search Instead of Quitting Engine
	- `parsePosition` Handles Arbitrary Whitespace Between Moves
- Centralised Version Tracking in `version.h`

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
