# BetterThanCris Roadmap

Planned versions and the changes targeted in each. Estimates are rough
self-play Elo over the prior shipped version; treat them as priority hints,
not contracts. Anything that fails self-play gets reverted or deferred to a
later version, as has already happened with TT bucketing, the Stockfish-style
improving heuristic, and the bestmove-stability time shrink.

Stockfish is used throughout as a reference and source of inspiration for
both classical evaluation and modern search techniques.

## Strategic context

The eval portion of the roadmap targets the classical-evaluation strength
ceiling, roughly where the strongest hand-crafted engines peaked before
neural-network evaluation took over. The engine sits well below that
ceiling today and has substantial room for classical-eval improvement.

Past that ceiling, the path to top-engine strength runs through NNUE.
v3.0 is the leap. The architectures used by top engines today are very
large; v3.0 targets a scaled-down version (smaller feature transformer,
no layer stacks, no PSQT buckets, no secondary feature sets) that a hobby
project can train and ship, with room to grow in v3.x.

Many modern search techniques (correction history, ProbCut, multi-ply
continuation history, pawn history) are independent of whether evaluation
is classical or neural. They appear in the search section of the roadmap
regardless of where NNUE lands.

## Current state (v2.6 in flight)

Already merged into main:

- LMP + frontier futility pruning
- LMR with history-based reduction adjustment plus check-giver bonus
- Aspiration window widening on fail
- Insertion sort in move ordering
- UCI handshake fixes: Hash advertised, UCI_EngineAbout added, Move
  Overhead and Hash defaults aligned with source
- Binary renamed to btc<version>.exe (e.g. btc26.exe), derived from
  ENGINE_VERSION in src/version.h
- TT default raised 12MB to 128MB, max raised 128MB to 4096MB

Tried and reverted in 2.6 (kept in memory for future re-attempts):

- 4-entry TT buckets with depth-preferred + age replacement: regressed
  startpos depth-10 nodes by 80%, cause unknown without instrumented
  cutoff logging
- Stockfish-style improving heuristic gating LMP/futility/RFP/LMR: lost
  45.5% in 3-way round-robin; LMP threshold halving on not-improving was
  too aggressive for the current eval signal
- Best-move stability time shrink + fail-low at root extension: lost
  51.0% in 3-way round-robin; 70% soft-limit shrink was too aggressive

## v2.6 final (this cycle)

Search-focused. Ship when each item proves +Elo in self-play.

1. Singular extensions. When the TT-best move scores significantly better
   than alternatives at reduced depth, extend it by 1 ply. Standard, low
   risk. Estimate: +20-40 Elo.
2. 2-ply continuation history. Extend the existing 1-ply table with a
   2-ply layer. Estimate: +10-20 Elo.
3. Counter-move heuristic. Per [prev_piece][prev_to] table of single
   counter moves. Estimate: +5-15 Elo.
4. Internal iterative reductions (IIR). When the node lacks a TT move at
   sufficient depth, reduce depth by 1 instead of doing internal
   iterative deepening. Cheap. Estimate: +10-20 Elo.
5. Re-attempt improving heuristic with milder constants. Drop the LMP
   threshold halving; only adjust margins by a smaller amount. Estimate:
   +10-25 Elo if it sticks, otherwise defer to v2.10.
6. Re-attempt TT bucketing with instrumented cutoff logging. Diagnose
   the bucket=4 regression. Ship only if positive. Estimate: +20-50 Elo
   if it works.

## v2.7 (king safety + threats, 1-2 weeks)

Goal: the easiest, highest-confidence classical-eval gains. Both items
have direct references in the classical eval the constants in
`eval_constants.cpp` were originally lifted from.

1. King safety re-tune. Replace the current `kingPenalty^2 / 4096`
   quadratic. The squaring shape is fine, but the input `kingDanger`
   needs to be a sum of many small contributions rather than one big
   one: attacker count by piece-type weight, weak squares in the king
   ring, unsafe-check count, pinned-piece count, king-flank attack
   pressure, mobility differential, no-enemy-queen bonus, knight
   defense, pawn-shelter influence, plus a threshold so small attacks
   give zero penalty. A safe-check table by piece type with
   single/multiple distinction. The "single big penalty squared" shape
   is what produces the 300-500cp swings on 3 attackers today. Estimate:
   +50-100 Elo. Direct cause of the "engine sacrifices for nothing"
   symptom.
2. Activate the threats scaffolding. Constants for `ThreatByMinor`,
   `ThreatByRook`, `ThreatByKing`, `ThreatByPawnPush`, `ThreatBySafePawn`,
   `Hanging`, `WeakQueen`, `RestrictedPiece` are defined in
   `eval_constants.cpp` but almost none are wired into `evaluate()`.
   Wire them up. Estimate: +50-100 Elo.

## v2.8 (eval surgery and tuning infrastructure, 2-4 weeks)

Goal: the largest classical eval improvement plus the multiplier that
makes future tuning fast.

1. Complex mobility with pin exclusion. Re-attempt the existing
   `positionCache` scaffolding. The previous failure was performance; fix
   it by computing pin info once per node in `updatePositionCache` (not
   per piece). Switch `getSimpleMobility` callers to the complex
   `getMobility` path, with queen X-rays through own queens and a
   separate queen mobility table. Estimate: +50-100 Elo.
2. `attackedBy2` table. Squares attacked twice by a side, computed by
   intersecting attack tables in `initAttacksTotal`. Used as a building
   block by many downstream eval terms (weak squares around the king,
   safe-pawn detection, defended-piece detection). Estimate: +10-20 Elo
   standalone, more as a building block.
3. Lazy eval thresholds. Skip expensive eval components (king safety,
   threats, passed, space) when the cheap eval portion is already far
   above the alpha-beta window. Pure speed gain that allows more
   detailed eval at no per-node cost. Estimate: +5-15 Elo.
4. Texel tuning pipeline. Standalone tuner binary linked against
   `evaluation.cpp`; a labeled dataset (Zurichess EPD, ~700K positions,
   free); gradient descent or local search on the constant tables in
   `eval_constants.cpp`. Run an initial Texel tune on the existing
   weights. Estimate: +30-80 Elo from the first tune alone, and a
   force multiplier for every later tuning decision.

After v2.8 the eval is substantially stronger and any future eval
constant can be auto-tuned.

## v2.9 (fill in missing eval terms, 2-3 weeks)

Goal: close more of the gap to the classical-eval ceiling.

1. Pawn hash table. Cache pawn-structure eval keyed by pawn-only zobrist.
   Mostly an NPS win, and lets pawn eval be much more detailed without
   per-node cost. King shelter and storm fit naturally in the same pawn
   entry. Estimate: +20-40 Elo.
2. Detailed passed pawn eval. King-distance scaling, blocker analysis,
   unstoppable-passer detection. Estimate: +20-40 Elo.
3. Imbalance table. Material values that depend on counterpart pieces
   (bishop pair more valuable in open positions, knight more valuable
   with own pawns, etc.). Estimate: +10-30 Elo.
4. Space evaluation. Safe squares behind own pawns weighted by piece
   presence. Estimate: +10-30 Elo.
5. Endgame scale factors. Opposite-color bishop drawish scaling, KBN
   endgames, etc. Estimate: +10-20 Elo.
6. Re-Texel-tune after each addition.

## v2.10 (search second pass with the stronger eval, 1-2 weeks)

A stronger eval signal makes the heuristics that failed in 2.6 work, and
unlocks several modern search techniques that depend on a reliable
static eval.

1. Correction history. Five small tables (pawn structure, minor-piece
   config, non-pawn-white, non-pawn-black, continuation) that record the
   gap between static eval and observed search result, then bias the
   next static eval by the learned correction. Independent of whether
   eval is classical or neural. Estimate: +30-60 Elo.
2. ProbCut. At depth >= 5 with a capture/promotion move, do a reduced
   search at `beta + ~200` to find moves that almost surely exceed beta,
   then prune. Estimate: +20-40 Elo.
3. Pawn history. Separate history table keyed by pawn-structure hash,
   for quiet move ordering. Captures the intuition that good moves in
   one pawn structure are often good in similar structures. Estimate:
   +15-30 Elo.
4. Low-ply history. Separate history for near-root nodes, refreshed
   faster than the main table. Estimate: +5-15 Elo.
5. Improving heuristic. Now reliable because eval is less noisy. Likely
   works with default constants this time.
6. Best-move stability and fail-low at root in time management. Same
   reasoning.
7. Razoring tuning. Texel-tune the razoring margins.
8. Aspiration window constants. Tune the delta progression.

## v2.x bonus (UCI and features, interleaved when time allows)

Not version-tied. Add as available.

- Pondering. Engine thinks on opponent's time. ~+50 Elo vs humans, ~0 in
  engine vs engine, but valuable for any GUI-mediated play against a
  human.
- Multi-PV. Show top N moves. Quality-of-life for analysis users.
- Syzygy EGTB. Endgame tablebase lookup. Estimate: +10-30 Elo in
  tournament play.
- Opening book support (polyglot). OwnBook UCI option. Estimate: +5-15
  Elo in opening phase.
- Clear Hash button option. UCI button to flush TT mid-session.

## v3.0 (NNUE, multi-month project)

The leap past the classical-eval ceiling. The end of v2 should reach
that ceiling; this is the only way past it.

Architecture target (scaled-down):

- Features: HalfKP or HalfKAv2_hm. HalfKP is simpler and well-documented;
  HalfKAv2_hm is more powerful with horizontal mirror symmetry that
  halves the king-bucket count. Start with HalfKP unless that proves
  limiting.
- Network shape: feature transformer L1 around 256-512; a small hidden
  layer (~32); single output value.
- No layer stacks (no per-phase subnetworks) initially.
- No PSQT buckets initially.
- No secondary feature set initially.

Implementation steps:

1. Architecture choice and Python trainer setup. Use the public
   `nnue-pytorch` trainer; no need to write one from scratch.
2. Training data. 50-100M positions from self-play at depth 8-10,
   labelled by game outcome (WDL) plus late-iteration search score.
3. C++ inference. Add SIMD inference code (AVX2 baseline). Roughly
   300-400 lines for a HalfKP/256 network.
4. Eval rewrite. Replace `evaluate()` with NNUE for non-leaf positions.
   Optionally keep classical eval as a regularisation target and as a
   fallback for very late endgames.
5. Incremental accumulator updates. Update the NNUE accumulator on each
   make/unmake so the whole feature transformer is not recomputed per
   call. Mandatory for usable NPS.
6. Tournament-test, iterate on data quality.

Estimate: +200-500 Elo.

## v3.x (beyond NNUE)

Each is a multi-week project.

- Lazy SMP parallel search. Concurrency-safe TT (the TT bucketing work
  finally pays off here). Estimate: +60-100 Elo per core doubling.
- Larger NNUE architectures. Scale up L1 toward 1024, add PSQT buckets,
  add layer stacks. As the network grows, inference needs better SIMD
  (AVX-512, NEON) and possibly quantisation.
- Secondary feature set (e.g. threat-based features) as a second input.
- More self-play loops and larger training datasets.
- Network architecture experimentation.

## Ongoing infrastructure (not version-tied)

These should land alongside the per-version work.

- Automated tournament harness (cute-chess-cli or fastchess) running the
  engine vs reference opponents on every commit, with SPRT testing for
  confident win/loss decisions.
- Standardised position test suites: WAC, ECM, Strategic Test Suite, for
  tactical regression detection beyond perft.
- Profiling and NPS tracking so performance regressions are caught
  before they ship.

## Estimated cumulative Elo trajectory

| Version | New work                                                | Estimated gain vs prior |
|---------|---------------------------------------------------------|-------------------------|
| 2.6     | search refinements + IIR + (maybe) ProbCut              | +60-130                 |
| 2.7     | king safety re-tune, threats activation                 | +100-200                |
| 2.8     | mobility w/ pin exclusion, attackedBy2, lazy thresholds, Texel pipeline | +100-220 |
| 2.9     | pawn hash, missing eval terms, retune                   | +60-160                 |
| 2.10    | correction history, ProbCut, pawn hist, search retune   | +80-170                 |
| 3.0     | NNUE                                                    | +200-500                |
| 3.x     | Lazy SMP, network upgrades, secondary feature sets      | +100-300 per major step |

End of v2, assuming all of the above lands, puts the engine roughly in
the 2500-2900 CCRL blitz range (solid strong-amateur). This is roughly
where the strongest classical-eval engines historically peaked. v3.0
with a working NNUE pushes into the 2900-3400 range. Beyond depends on
engineering time invested.
