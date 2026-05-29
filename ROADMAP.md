# BetterThanCris Roadmap

Planned versions and the changes targeted in each. Estimates are rough self-play Elo over the prior shipped version; treat them as priority hints, not contracts. Anything that fails self-play gets reverted or deferred to a later version, as has already happened with TT bucketing, the Stockfish-style improving heuristic, and the bestmove-stability time shrink.

Stockfish is used throughout as a reference and source of inspiration for both classical evaluation and modern search techniques.

## Strategic context

The eval portion of the roadmap targets the classical-evaluation strength ceiling, roughly where the strongest hand-crafted engines peaked before neural-network evaluation took over. The engine sits well below that ceiling today and has substantial room for classical-eval improvement.

Past that ceiling, the path to top-engine strength runs through NNUE. v3.0 is the leap. The architectures used by top engines today are very large; v3.0 targets a scaled-down version (smaller feature transformer, no layer stacks, no PSQT buckets, no secondary feature sets) that a hobby project can train and ship, with room to grow in v3.x.

Many modern search techniques (correction history, ProbCut, multi-ply continuation history, pawn history) are independent of whether evaluation is classical or neural. They appear in the search section of the roadmap regardless of where NNUE lands.

Much of the v2.7-v2.9 eval work is **rewriting** existing BTC terms to match SF's canonical shape, not just adding new constants. Several current terms compile but use a simplified or wrong shape: the per-piece-type threat cascade inline in `evaluateWhite/BlackKnight` and `Bishop`, flag-style `Hanging`, the simplified `kingPenalty` sum, `getSimpleMobility`, the flat `KnightOnQueen`, the rank+file passed-pawn bonus, and the narrow `adjustEndgameEvaluation` (currently disabled). When this roadmap says "re-tune", "activate", or "detailed", the actual work is usually to delete the simplified version, port SF's term from `.stockfish_src_old/` verbatim, and then Texel-tune the constants.

## v2.6 (29/05/2026)

Merged into main:

- LMP + frontier futility pruning
- LMR with history-based reduction adjustment plus check-giver bonus
- Aspiration window widening on fail
- Insertion sort in move ordering
- UCI handshake fixes: Hash advertised, UCI_EngineAbout added, Move Overhead and Hash defaults aligned with source
- Binary renamed to btc<version>.exe (e.g. btc26.exe), derived from ENGINE_VERSION in src/version.h
- TT default raised 12MB to 128MB, max raised 128MB to 4096MB
- Singular extensions: TT-best singularity verification at reduced depth (depth >= 8, ttDepth >= depth - 3, singularBeta = ttScore - 2 * depth, verifier at (depth - 1) / 2, +1 ply extension on fail-low). +156 Elo over 45 self-play games.
- Combined search batch, shipped together after scoring 64% (35W 58D 7L over 100 games) vs the pre-batch 2.6 build:
	- Counter-move heuristic: `[prevPiece][prevTo]` table of single refutations, ordered between killers and history.
	- 2-ply continuation history: symmetric reads, 3/4 update bonus on the 2-ply table to match SF's 780/1040 weighting.
	- Internal iterative reductions: `depth--` at depth >= 6 with no TT move.
	- Improving heuristic: `eval[ply] > eval[ply-2]` gates the LMP threshold `(3 + d*d) / (2 - improving)`, the RFP margin, and LMR.
	- Best-move-stability time management: 70% soft-limit shrink at 5+ stable root best-moves, +30% extension on a >= 50cp score drop.
	- TT 4-entry buckets: depth-preferred + generation-aged replacement (`depth - 8 * (gen - age)`).

## v2.7 (king safety + threats)

Goal: the easiest, highest-confidence classical-eval gains. Both items have direct references in the classical eval the constants in `eval_constants.cpp` were originally lifted from.

1. King safety re-tune. Replace the current `kingPenalty^2 / 4096` quadratic. The squaring shape is fine, but the input `kingDanger` needs to be a sum of many small contributions rather than one big one: attacker count by piece-type weight, weak squares in the king ring, unsafe-check count, pinned-piece count, king-flank attack pressure, mobility differential, no-enemy-queen bonus, knight defense, pawn-shelter influence, plus a threshold so small attacks give zero penalty. A safe-check table by piece type with single/multiple distinction. The "single big penalty squared" shape is what produces the 300-500cp swings on 3 attackers today. Additional king-section terms that sit *alongside* the kingDanger quadratic in SF's `king()` should land in the same pass: `PawnlessFlank` (king's file has no pawns), `FlankAttacks * kingFlankAttack` (linear penalty for attacks in the king flank), and `+3 * kingFlankAttack² / 8 - 4 * kingFlankDefense` inside the kingDanger sum itself. Note: the mobility-differential contribution makes this partially blocked on v2.8 mobility re-work — land it first with a zeroed mobility term, top up after v2.8. Estimate: +50-100 Elo. Direct cause of the "engine sacrifices for nothing" symptom.
2. Rewrite and activate the threats scaffolding. Constants for `ThreatByMinor`, `ThreatByRook`, `ThreatByKing`, `ThreatByPawnPush`, `ThreatBySafePawn`, `Hanging`, `WeakQueen`, `RestrictedPiece` are defined in `eval_constants.cpp` but almost none are wired into `evaluate()`. The piece-section threat code currently inline in `evaluateWhite/BlackKnight` and `Bishop` (a cascade of per-piece-type `if`s plus a flag-style `Hanging`) should be **deleted and replaced** with SF's `threats()` shape: iterate the attacked weak set once, index `ThreatByMinor[type_of(piece)]` per attacker, use `popcount` for `Hanging` and `WeakQueenProtection` instead of flags. `KnightOnQueen` and `SliderOnQueen` slip to v2.8 since they need mobility-area info and `attackedBy2`. Estimate: +50-100 Elo.
3. Piece-on-king-ring bonuses and outpost rewrite. `BishopOnKingRing` (bishop X-raying through pawns onto the enemy king ring) and `RookOnKingRing` (rook sharing a file with the king ring) fire when the piece is *not* a direct attacker; they sit in the piece-section evaluators alongside the existing king-distance term. The current outpost detection (per-piece if-else in `evaluateWhite/BlackKnight` and `Bishop`) should be rewritten to SF's mask form: build `OutpostRanks & attackedBy[Us][PAWN] & ~pawn_attacks_span(Them)` once per side, then test piece occupancy and reachability against it. Cleaner and avoids the current outpost / uncontested-outpost / reachable-outpost double-counting risk. Estimate: +5-20 Elo combined.

## v2.8 (eval surgery and tuning infrastructure)

Goal: the largest classical eval improvement plus the multiplier that makes future tuning fast.

1. Complex mobility with pin exclusion. Re-attempt the existing `positionCache` scaffolding. The previous failure was performance; fix it by computing pin info once per node in `updatePositionCache` (not per piece). Switch `getSimpleMobility` callers to the complex `getMobility` path, with queen X-rays through own queens and a separate queen mobility table. The pin info computed here feeds two follow-on rewrites: (a) the pinned-queen `WeakQueen` penalty (BTC has the constant in `eval_constants.cpp` but unused; SF applies it when the queen sits on a slider-blocker line, distinct from `WeakQueenProtection` on weak pieces); (b) mobility-area-aware `KnightOnQueen` and a new `SliderOnQueen` term, replacing BTC's current flat `KnightOnQueen` bonus with SF's version that only counts safe squares the attacker can actually reach (and gates the slider variant on `attackedBy2`). Estimate: +50-100 Elo.
2. Use the existing `attackedBy2` table more aggressively. The table is already computed in `initAttacksTotal` via `firstPass`/`secondPass` accumulation but currently feeds only `stronglyProtected`/`defended`/`weak`. SF feeds `attackedBy2` into king-ring weak-square detection (consumed by v2.7 king safety), safe-pawn-push detection (threats), safe-check evaluation (king safety), and the `SliderOnQueen` gating above. The work here is wiring it into the v2.7 and item 1 rewrites, not computing the table. Estimate: +10-20 Elo as a building block.
3. Lazy eval thresholds. Skip expensive eval components (king safety, threats, passed, space) when the cheap eval portion is already far above the alpha-beta window. Pure speed gain that allows more detailed eval at no per-node cost. Estimate: +5-15 Elo.
4. Texel tuning pipeline. Standalone tuner binary linked against `evaluation.cpp`; a labeled dataset (Zurichess EPD, ~700K positions, free); gradient descent or local search on the constant tables in `eval_constants.cpp`. Run an initial Texel tune on the existing weights. Estimate: +30-80 Elo from the first tune alone, and a force multiplier for every later tuning decision.

After v2.8 the eval is substantially stronger and any future eval constant can be auto-tuned.

## v2.9 (fill in missing eval terms)

Goal: close more of the gap to the classical-eval ceiling.

1. Pawn hash table. Cache pawn-structure eval keyed by pawn-only zobrist. Mostly an NPS win, and lets pawn eval be much more detailed without per-node cost. King shelter and storm fit naturally in the same pawn entry. Once the cache lands, wire in the missing SF pawn terms: `BlockedPawn` at rank 5/6, `DoubledEarly` (early-file doubled pawn penalty), `KingOnFile` (penalty when the king sits on a semi-open or fully open file from its own side), `WeakLever` (constant + code already commented out in `evaluateWhite/BlackPawn`; uncomment), and the after-castle shelter probe (scaffolding for `c1/c8/g1/g8` shelter evaluation already drafted but commented out in `getKingSafety`; uncomment and verify the indices). Estimate: +20-40 Elo.
2. Detailed passed pawn eval. **Rewrite** the existing rank+file bonus in `evaluateWhite/BlackPawn` to match SF's `passed()`: (a) king-proximity scaling `(km_them * 19/4 - km_us * 2) * w` at relative rank >= 4 where `km = min(distance(king, blockSq), 5)` and `w = 5*r - 13`; (b) a `k` factor (0/7/17/30/36, +5 if defended) when the block square is empty, based on which subset of the path-to-queen is attacked; (c) "helpers" rule that revives blocked candidate passers if a supporting friendly pawn is shift-east/west away; (d) `PassedFile * edge_distance(file)` deduction. The current code contributes only the rank+file bonus and ignores everything about the king race or path safety. Estimate: +20-40 Elo.
3. Imbalance table. Material values that depend on counterpart pieces (bishop pair more valuable in open positions, knight more valuable with own pawns, etc.). Estimate: +10-30 Elo.
4. Space evaluation. Safe squares behind own pawns weighted by piece presence. Estimate: +10-30 Elo.
5. Initiative / complexity bonus from SF's `winnable()`. Distinct from scale factors and applied **before** mg/eg interpolation: a complexity score from `9 * passed_count + 12 * pawn_count + 9 * outflanking + 21 * pawnsOnBothFlanks + 24 * infiltration + 51 * !nonPawnMaterial - 43 * almostUnwinnable - 110` is added to whichever side is winning (sign of mg/eg), capped so it cannot flip the score sign. Scale factors *shrink* the eg of drawish endgames; the initiative term *amplifies* a balanced eval toward the side with a positional edge. Both terms live in the same SF function but they are independent and were missed when this section was first drafted. Estimate: +20-40 Elo.
6. Endgame scale factors. Opposite-color bishop drawish scaling, single-flank rook endgames, KBNK, queen-vs-no-queen reductions. Replace BTC's narrow `adjustEndgameEvaluation` (a KRPvKR / KNNK handler currently disabled in `evaluate()`) with SF's `Material::probe`-style dispatch: a small material-key cache returns a per-position scale factor in [0, 96] that scales the eg portion during interpolation. Estimate: +10-20 Elo.
7. Specialized endgame eval functions. KPK, KRK, KBNK, KBPK, KQKP — hand-coded eval functions that *replace* the general eval when the material configuration is one BTC consistently mis-scores. Different from scale factors, which scale a result; these return a known-correct number. SF keeps these in `endgame.cpp`; BTC has none. Estimate: +10-30 Elo.
8. Re-Texel-tune after each addition.

## v2.10 (search second pass with the stronger eval)

A stronger eval signal unlocks several modern search techniques that depend on a reliable static eval. Items 1-4 are new techniques; items 5-6 are tuning. The v2.6 experiments that originally lived here as revisits (2-ply continuation history, improving heuristic, best-move-stability time management, internal iterative reductions) shipped in 2.6 as a combined batch and are no longer pending.

1. Correction history. Five small tables (pawn structure, minor-piece config, non-pawn-white, non-pawn-black, continuation) that record the gap between static eval and observed search result, then bias the next static eval by the learned correction. Independent of whether eval is classical or neural. Estimate: +30-60 Elo.
2. ProbCut. At depth >= 5 with a capture/promotion move, do a reduced search at `beta + ~200` to find moves that almost surely exceed beta, then prune. Estimate: +20-40 Elo.
3. Pawn history. Separate history table keyed by pawn-structure hash, for quiet move ordering. Captures the intuition that good moves in one pawn structure are often good in similar structures. Estimate: +15-30 Elo.
4. Low-ply history. Separate history for near-root nodes, refreshed faster than the main table. Estimate: +5-15 Elo.
5. Razoring tuning. Texel-tune the razoring margins.
6. Aspiration window constants. Tune the delta progression.

## v2.x bonus (UCI and features, interleaved when convenient)

Not version-tied. Add as available.

- Pondering. Engine thinks on opponent's time. ~+50 Elo vs humans, ~0 in engine vs engine, but valuable for any GUI-mediated play against a human.
- Multi-PV. Show top N moves. Quality-of-life for analysis users.
- Syzygy EGTB. Endgame tablebase lookup. Estimate: +10-30 Elo in tournament play.
- Opening book support (polyglot). OwnBook UCI option. Estimate: +5-15 Elo in opening phase.
- Clear Hash button option. UCI button to flush TT mid-session.
- `TrappedRook` penalty. Rook with `mob <= 3` stuck on the same wing as a king that has lost castling rights. Detection code already drafted but commented out in `evaluateWhite/BlackRook`; needs only the `TrappedRook` constant and uncommenting. Trivial. Estimate: ~+5 Elo.
- Evaluation grain rounding. `v = (v / 16) * 16` at the end of `evaluate()`. Coarsens the eval so equivalent positions share TT entries more often, raising the cutoff rate. One-liner. Estimate: ~+5 Elo historically.

## v3.0 (NNUE)

The leap past the classical-eval ceiling. The end of v2 should reach that ceiling; this is the only way past it.

Architecture target (scaled-down):

- Features: HalfKP or HalfKAv2_hm. HalfKP is simpler and well-documented; HalfKAv2_hm is more powerful with horizontal mirror symmetry that halves the king-bucket count. Start with HalfKP unless that proves limiting.
- Network shape: feature transformer L1 around 256-512; a small hidden layer (~32); single output value.
- No layer stacks (no per-phase subnetworks) initially.
- No PSQT buckets initially.
- No secondary feature set initially.

Implementation steps:

1. Architecture choice and Python trainer setup. Use the public `nnue-pytorch` trainer; no need to write one from scratch.
2. Training data. 50-100M positions from self-play at depth 8-10, labelled by game outcome (WDL) plus late-iteration search score.
3. C++ inference. Add SIMD inference code (AVX2 baseline). Roughly 300-400 lines for a HalfKP/256 network.
4. Eval rewrite. Replace `evaluate()` with NNUE for non-leaf positions. Optionally keep classical eval as a regularisation target and as a fallback for very late endgames.
5. Incremental accumulator updates. Update the NNUE accumulator on each make/unmake so the whole feature transformer is not recomputed per call. Mandatory for usable NPS.
6. Tournament-test, iterate on data quality.

Estimate: +200-500 Elo.

## v3.x (beyond NNUE)

- Lazy SMP parallel search. Make the TT concurrency-safe (the 2.6 4-entry buckets are the structure this builds on). Estimate: +60-100 Elo per core doubling.
- Larger NNUE architectures. Scale up L1 toward 1024, add PSQT buckets, add layer stacks. As the network grows, inference needs better SIMD (AVX-512, NEON) and possibly quantisation.
- Secondary feature set (e.g. threat-based features) as a second input.
- More self-play loops and larger training datasets.
- Network architecture experimentation.

## Ongoing infrastructure (not version-tied)

These should land alongside the per-version work.

- Automated tournament harness (cute-chess-cli or fastchess) running the engine vs reference opponents on every commit, with SPRT testing for confident win/loss decisions.
- Standardised position test suites: WAC, ECM, Strategic Test Suite, for tactical regression detection beyond perft.
- Profiling and NPS tracking so performance regressions are caught before they ship.

## Estimated cumulative Elo trajectory

| Version | New work                                                | Estimated gain vs prior |
|---------|---------------------------------------------------------|-------------------------|
| 2.6     | LMP, futility, LMR, aspiration, sort, UCI, singular extensions, combined search batch (counter-move, 2-ply cont hist, IIR, improving, bm-stability time mgmt, TT 4-entry buckets) | +156 on singular extensions (45g) plus 64% (35W 58D 7L, ~+100 Elo) on the combined batch (100g) |
| 2.7     | king safety re-tune, threats activation                 | +100-200                |
| 2.8     | mobility w/ pin exclusion, attackedBy2, lazy thresholds, Texel pipeline | +100-220 |
| 2.9     | pawn hash, missing eval terms, retune                   | +60-160                 |
| 2.10    | correction history, ProbCut, pawn hist, low-ply, 2-ply cont hist revisit, improving revisit, bm-stability revisit, search retune | +80-170 |
| 3.0     | NNUE                                                    | +200-500                |
| 3.x     | Lazy SMP, network upgrades, secondary feature sets      | +100-300 per major step |

End of v2, assuming all of the above lands, puts the engine roughly in the 2500-2900 CCRL blitz range (solid strong-amateur). This is roughly where the strongest classical-eval engines historically peaked. v3.0 with a working NNUE pushes into the 2900-3400 range. Beyond depends on engineering time invested.
