
<div align="center">
  <img src="logo.png" alt="BetterThanCris" width="200">
  <h1>BetterThanCris</h1>
  <p><b>A strong UCI chess engine written from scratch in performance-oriented C(++).</b></p>
  <p>
    <img src="https://img.shields.io/badge/Protocol-UCI-informational" alt="UCI">
    <img src="https://img.shields.io/badge/CCRL%20Blitz-~2933-orange" alt="Strength">
  </p>
  Last Release v2.7 - 9th June 2026
</div>

---

BetterThanCris is a UCI chess engine built from scratch in performance-oriented C(++). It uses alpha-beta search with the modern pruning and reduction techniques used by top engines with a full hand-crafted classical evaluation, reaching an estimated ~2930 CCRL Blitz strength (v2.7). Every change is validated by self-play A/B testing, and move generation is regression-tested against known perft node counts.

The codebase is deliberately procedural and data-oriented. There are no classes or STL containers on the hot path, cache-friendly bitboard representation, and no hot-path allocation. It compiles as C++ and uses a handful of its conveniences (`std::min`/`max`/`clamp`, references, structs), but was written originally as a C engine.

## Table of Contents 
 1. [Introduction](#introduction)
 2. [Why I Built This](#why-i-built-this)
 3. [Strength](#strength)  
 4. [Technical Highlights](#technical-highlights)
 5. [Features](#features)
 6. [To Do](#to-do)  
 7. [Play BetterThanCris](#play-betterthancris)
 8. [Releases](#releases) 
 9. [Credits](#credits)
## Introduction
 - UCI protocol chess engine, created by Gustavo Knudsen
 - Strong classical engine: estimated 2933 CCRL Blitz, every feature self-play validated
 - Magic-bitboard move generation, alpha-beta search, full hand-crafted evaluation
 - Plays online on [Lichess](https://lichess.org/@/BetterThanCris) - not always online
 - Current Version: 2.7
 - Aggressive, entertaining chess, and is, in fact, Better Than Cris

## Why I Built This

I'm a big chess fan, and was interested by chess engines and the AI behind them. So I began devloping my first chess engine a few months after learning programming.

I couldn't beat my friend Cris and instead of practising, I built an engine that could. It cleared that original goal a while ago. Many iterations later it now plays stronger than virtually every human on the planet, which feels a little surreal. It's my favourite project to work on, and there's always something new to improve.

## Strength
**Version 2.7 (Latest)**
- Estimated [CCRL](https://www.computerchess.org.uk/ccrl/404/) Blitz Rating ([Methodology / Data](https://docs.google.com/spreadsheets/d/1t2gDEfoMDtqAA5uL9U_GPA9CijjlMrVK4AR4DiAqqGU/edit?usp=sharing)): 2933 ± 33
	- Conditions: 2+1 Blitz (CCRL Blitz Time Control), Single Thread (Intel i7-12650H), Standard Opening Book to Move 6, 4-Man Endgame Tablebases
- A ~446+ gain over v2.6.  Due to a full evaluation overhaul and a search second pass (see [Releases](#releases))
- [Lichess](https://lichess.org/@/BetterThanCris) (Playing Almost Exclusively Against Other Bots):
	- Bullet: 2503 Peak, Blitz: 2506 Peak
- Against Humans, Especially in Bullet or Blitz, Rating can be Expected to be Higher
- [Sample Games](https://www.chess.com/analysis/collection/betterthancris-2-7-samples-games-2eHH42Fk2)

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
## Technical Highlights

Full breakdown in [Features](#features).

- **Bitboard engine with magic bitboards.** 64-bit board representation and perfect-hash (magic) lookups for sliding-piece attacks, with pre-computed attack tables.
- **Alpha-beta search:** Iterative deepening with aspiration windows, a Zobrist-hashed transposition table (bucketed, depth-and-age replacement), and a Static-Exchange-Evaluation-ordered quiescence search.
- **Modern search heuristics:** Late move reductions, null-move and reverse-futility pruning, late move and frontier-futility pruning, singular extensions, ProbCut, and correction history, plus five history tables (main, capture, continuation, pawn-structure, low-ply) for move ordering.
- **Full classical evaluation:** King safety, mobility with pin detection and queen x-rays, threats, detailed passed pawns, a material imbalance table, pawn-hash-cached structure, space, and specialised endgame knowledge including a generated KPK bitbase.
- **Purpose-built SPRT test harness:** A standalone match runner (`harness/`) that plays parallel engine-vs-engine matches, judges them with its own independent arbiter, and applies a pentanomial sequential probability ratio test, so changes worth a few Elo can be measured rather than guessed at.
- **Engineering Process:** Every feature is validated by self-play A/B testing with measured Elo improvement. Move generation is regression-tested against canonical perft node counts, and the harness arbiter against the same counts independently. Strength has climbed from ~2070 to an estimated ~2933 CCRL Blitz across recent versions.

## Features
 **General Features:**
 - UCI Protocol:
	- Fixed Depth, Movetime, and Fixed-Nodes (`go nodes`) Search Modes
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
- SPRT Testing Harness (`harness/`):
	- Parallel Engine-vs-Engine Matches, Own UCI Process and Arbiter Layer
	- Pentanomial (Game-Pair) Sequential Probability Ratio Test
	- EPD / PGN Opening Books, Adjudication, PGN Output, Anomaly Logging
	 
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
 - ProbCut (SEE-Filtered Capture, Qsearch-Verified Then Reduced Search Above a Raised Beta)
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
	- 1-Ply & 2-Ply Continuation History
	- Pawn-Structure History (Quiet Ordering Keyed by Pawn Structure)
	- Low-Ply History (Per-Search Near-Root Ordering Table)
	- Carryover Across Moves Within a Game
 - Correction History:
	- Pawn-Structure and Non-Pawn (Per-Colour) Static-Eval Correction Tables
	- Biases the Static Eval by the Learned Eval-vs-Search Gap
 - Killer Moves
 - Counter-Move Heuristic
 - Transposition Table w/ Zobrist Hashing:
	- 4-Entry Buckets w/ Depth-Preferred + Generation-Aged Replacement
 - Hash Move Ordering
 - Repetitions
 - Single-Legal-Move Fast Path

**Evaluation**
- Unified Classical Evaluation Scale (Single Consistent Material / Positional Scale)
- Material Evaluation + Piece-Square Tables, Tapered (Midgame / Endgame Interpolation)
- Material Imbalance Table (Piece-Pair Polynomial, Subsumes the Bishop Pair)
- Pawn Structure (Cached by a Pawn Hash):
	- Doubled, Isolated, Backward, Blocked, Weak-Lever Pawns
	- Connectivity: Phalanx & Supported Pawn Chains
	- King on Open / Semi-Open File
- Detailed Passed Pawns:
	- Rank / File Scaling, King-Race to the Stop Square, Path-to-Promotion Safety
- Mobility:
	- Pin-Excluded Mobility Area, X-Rays Through Queens
- Threats:
	- Threats by Minor / Rook / King / Pawn-Push, Hanging Pieces, Weak Queen
	- Knight-on-Queen and Slider-on-Queen Forks
- King Safety:
	- Sum-of-Contributions King Danger (Safe / Unsafe Checks, Weak King-Ring Squares, Slider Blockers)
	- King-Flank Attack / Defense, Pawnless Flank, No-Enemy-Queen
	- King Ring / Shelter Strength / Unblocked Pawn Storm
- Space Evaluation (Safe Central Squares Behind Friendly Pawns)
- Outposts, Rook Open / Semi-Open Files
- Endgame Knowledge:
	- Drawishness Scale Factors (Opposite-Colour Bishops, Single-Flank Rook Endings, Pawnless Edges)
	- Specialised Exact Evals (KXK, KBNK, KNNK, KR / KQ vs Lone Piece) + a KPK Bitbase
- Tempo

## To Do

**Next v2.9 (The Complete Classical Engine):**
- Lazy Evaluation / NPS:
	- Skip Expensive Eval Terms When the Cheap Eval Is Far Outside the Window
	- Cache More Eval (As With the Pawn Hash)
- Texel / SPSA Tuning of Eval Weights
	- Largest "Free Elo" Lever Without Eval Structure Changes
- Search-Constant Tuning (LMR / Null / Futility / Razoring / Aspiration / Singular)

**Evaluation (Lower Priority):**
- Initiative / Complexity Bonus (Needs an mg/eg Accumulator Refactor; Also Makes Endgame Scaling Exact)
- Piece-on-King-Ring Bonuses, Outpost Rewrite to Mask Form

**NNUE (v3.0 - Long-Term):**
- HalfKP / HalfKAv2 Feature Transformer + SIMD Inference
- Self-Play Training Data + Incremental Accumulator Updates

**UCI Completeness (v2.9):**
- Add Pondering Option
- Add Multi-PV Search
- Add Syzygy EGTB
- Add Opening Book Support

**Beyond NNUE (v3.x):**
- Add Parallel Search / Lazy SMP

 
## Play BetterThanCris
 - If online, can be played on  [Lichess](https://lichess.org/@/BetterThanCris) 
 - Can also be downloaded and ran like a normal UCI engine locally on a GUI
## Testing Methodology

Changes through v2.7 were validated with fixed-length self-play matches of 45-100 games at 2+1. The standard error on the score at those sample sizes is roughly ±5%, so results below about 55% are directional only; sample sizes are given per result below where they were recorded.

From v2.8 changes are accepted under SPRT, using a purpose-built harness (see [harness/](harness/)): elo0=0, elo1=5, α=β=0.05 for gains and elo0=-5, elo1=0 for simplifications, at 8+0.08 with a randomised UHO opening book, scored pentanomially over colour-reversed game pairs. The three marginal v2.7 search results were retested at elo0=0, elo1=15; verdicts are inline below.

## Releases
**Version 2.8 - Testing Harness**

Tooling rather than a strength release: the measurement capability the remaining classical work depends on, and the engine bugs building it exposed.

- Added SPRT Testing Harness (`harness/`), Written From Scratch:
	- Parallel Engine-vs-Engine Matches, Own UCI Process Layer and Independent Arbiter
	- Pentanomial (Game-Pair) Sequential Probability Ratio Test
	- EPD / PGN Opening Books, Adjudication, PGN Output, Anomaly Logging
- Added Fixed-Nodes Search (`go nodes`) for Deterministic Testing
- Fixed: `position` Command Overflowed a 2000-Byte Input Buffer Past ~Move 195, Desynchronising the Engine From the GUI
	- Effect: Long Games Were an Effective Forfeit (~0.9 Elo)
- Fixed: `Move Overhead` Option Was Silently Ignored
- Fixed: Engine Spun at Full CPU on Stdin EOF Instead of Exiting
- Fixed: Root Position of Each Search Was Evaluated With Stale Mobility Areas
- Retested Three Marginal v2.7 Results Under SPRT (Verdicts Inline Below); All Three Kept

**Version 2.7 - 08/06/2026**
- Major Evaluation Overhaul, Rebuilt on a Single Unified Classical Scale:
	- Replaced the Mixed Material / Piece-Square Scale With One Consistent Scale and Removed All Ad-Hoc Rescaling
		- Effect: Roughly 80% Score (35W 10D 5L Over 50 Games) vs the Pre-Overhaul Build
	- Rewrote King Safety to a Sum-of-Contributions King Danger:
		- Safe / Unsafe Checks, Weak King-Ring Squares, Slider Blockers, King-Flank Attack / Defense, Pawnless Flank, No-Enemy-Queen
	- Rewrote Threats: Threats by Minor / Rook / King / Pawn-Push, Hanging, Weak Queen, Knight-on-Queen and Slider-on-Queen Forks
	- Mobility With a Pin-Excluded Mobility Area and X-Rays Through Queens (53%, 27W 52D 21L Over 100 Games)
	- Detailed Passed Pawns With King-Race and Path-to-Promotion Safety (84%, 8D 1L)
	- Material Imbalance Table, Subsuming the Bishop Pair (58%, 26W 31D 15L Over 72 Games)
	- Pawn-Structure Rewrite + King-on-File + Pawn-Hash Cache (57%)
	- Space Evaluation and Knight / Slider-on-Queen Threats (65% as a Batch, 13W 9D 5L, Stopped Early)
	- Endgame Scale Factors for Drawish Endings (58%, 12W 34D 4L Over 50 Games, Played to Mate)
	- Specialised Endgames: Exact KXK / KBNK / KNNK / KR / KQ vs Lone Piece, Plus a KPK Bitbase
- Search Second Pass:
	- Correction History (Pawn and Non-Pawn Static-Eval Correction Tables): 56% (17W 22D 11L Over 50 Games, Originally Read as ~+42 Elo)
		- **SPRT Retest: Not Reproduced.** +4.8 ± 8.5 Elo Over 4,016 Games (Kept)
	- ProbCut (SEE-Filtered Capture, Qsearch-Verified Then Reduced Search Above a Raised Beta): 50% (22W 56D 22L Over 100 Games)
	- Pawn-Structure History for Quiet Move Ordering: 57% (18W 21D 11L Over 50 Games, Originally Read as ~+49 Elo)
		- **SPRT Retest: Not Reproduced.** -5.7 ± 16.0 Elo Over 1,042 Games
	- Low-Ply Near-Root History, Reset Per Search: 50.7% (36W 80D 34L Over 150 Games)
		- **SPRT Retest: Not Reproduced.** +0.3 ± 11.4 Elo Over 2,008 Games
- Every Change Individually A/B-Validated in Self-Play

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

BetterThanCris began as a tutorial implementation and has since grown into a substantially original engine, far beyond where it started. These resources were invaluable along the way:

**Foundations & learning:**
- Code Monkey King's [Bitboard Chess Engine in C](https://www.youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs): the tutorial the earliest version was built from
- [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page): reference for nearly every technique used here
- Bluefever Software's [Chess Engine in C](https://youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg), Sebastian Lague's [Chess Coding Adventure](https://youtube.com/playlist?list=PLFt_AvWsXl0cvHyu32ajwh2qU1i6hl77c), Eddie Sharick's [Chess Engine in Python](https://youtube.com/playlist?list=PLBwF487qi8MGU81nDGaeNE1EnNEPYWKY_), and Gaurav Pant's [Improving Search](https://www.youtube.com/watch?v=mVdh5z0jtAo)

**Engines & techniques referenced:**
- [Stockfish](https://github.com/official-stockfish/Stockfish): classical-era evaluation structure and values
- [Strelka](https://github.com/FireFather/strelka): razoring
- [PeSTO's Evaluation](https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function) by Ronald Friederich
- [TSCP](https://www.chessprogramming.org/TSCP) by Tom Kerrigan
- UCI protocol communication code by Richard Allbert
- [Xorshift](https://en.wikipedia.org/wiki/Xorshift) PRNG

## License

BetterThanCris is licensed under the GNU General Public License v3.0, see [LICENSE](LICENSE).

Copyright (C) 2024-2026 Gustavo Knudsen.
