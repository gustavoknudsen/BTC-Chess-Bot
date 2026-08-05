# SPRT harness

A self-contained engine match runner and sequential test, written in the same C-style C++ as the engine. It plays two UCI engines against each other from an opening book, judges the games with its own arbiter, and applies a sequential probability ratio test to the results so a run stops as soon as the evidence is decisive.

The harness exists because a 50-300 game match cannot resolve a sub-30 Elo change, and past that point almost every remaining improvement to BetterThanCris is a sub-30 Elo change. It is the gate for the whole v2.8 tuning cycle, and the game generator v3.0 will train on.

## Build

```
make sprt        # builds sprt.exe
make sprt-test   # builds it and runs the internal checks
```

The harness links none of `src/`. That is deliberate: an arbiter that shared a move generator with the engine under test could not detect that engine's move generation bugs, and the engine's globals would not survive being driven from several threads at once.

## Quick start

```
./sprt.exe \
  -engine cmd=btc27.exe name=base \
  -engine cmd=btc28.exe name=dev \
  -each tc=10+0.1 option.Hash=64 \
  -openings file=books/UHO_Lichess_4852_v1.epd format=epd order=random \
  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
  -concurrency 8 \
  -pgnout games.pgn -log anomalies.log
```

Every opening is played twice with the colours reversed, and results are scored as pairs, so an odd number of games is never reported. Progress prints one line per pair, and the run stops when the test decides, when `-rounds` is exhausted, or on Ctrl+C (which finishes the games already in flight).

Exit codes: 0 when H1 was accepted or a plain match finished, 1 when H0 was accepted or the run was inconclusive, 2 on a fatal error such as an engine that could not be started.

## Options

Engine settings go inside `-engine`, or inside `-each` to apply to both. A setting given per engine wins over the same setting in `-each`.

| setting | meaning |
|---|---|
| `cmd=COMMAND` | command line that starts the engine |
| `name=NAME` | name used in the report and in PGN tags |
| `dir=PATH` | working directory for the engine |
| `tc=[MOVES/]TIME[+INC]` | clock in seconds: `10+0.1`, `40/60`, `2:30+2` |
| `st=SECONDS` | fixed time per move |
| `depth=N` | fixed depth per move |
| `nodes=N` | fixed nodes per move |
| `restart=on` | new process for every game |
| `option.NAME=VALUE` | sent as `setoption` before the first game |

Match settings:

| option | meaning |
|---|---|
| `-openings file=PATH [format=epd\|pgn] [plies=N] [order=sequential\|random] [start=N] [count=N]` | opening source. `plies` applies to PGN books, `count` caps how many openings are held in memory |
| `-rounds N` | opening pairs to play at most, 0 means until the test decides |
| `-concurrency N` | games in parallel |
| `-sprt elo0=A elo1=B alpha=X beta=Y [minpairs=N]` | enables the sequential test |
| `-resign movecount=N score=CP` | resign adjudication, both engines must agree for N moves |
| `-draw movenumber=N movecount=N score=CP` | draw adjudication |
| `-maxmoves N` | adjudicate a draw after N moves |
| `-timemargin MS` | grace before a clock overrun loses the game (default 100) |
| `-pgnout FILE` | append every game, with score and depth comments |
| `-log FILE` | append crashes, illegal moves and losses on time |
| `-trace FILE` | append every command and reply, for debugging a protocol problem |
| `-event NAME` | PGN event tag |
| `-seed N` | seed for the opening order |
| `-selftest` | run the internal checks and exit |

## Choosing the bounds

The two bounds are the hypotheses the test decides between, in Elo.

- **A change meant to gain**: `elo0=0 elo1=5`. H1 accepted means the change is worth more than 0 Elo; H0 accepted means it is not worth 5.
- **A simplification or refactor meant not to lose**: `elo0=-3 elo1=1`. This passes changes that are neutral and rejects real regressions.
- `alpha` and `beta` are the error rates you accept in each direction. `0.05` each is the usual choice and gives bounds of roughly plus and minus 2.94.

A test with these bounds typically needs a few hundred pairs for a clear gainer and several thousand for a marginal one. That is the point: the run length adapts to how strong the effect is, instead of being fixed in advance at a number too small to see it.

## How the statistics work

Results are collected **pentanomially**: the unit of observation is the pair of games sharing one opening, scored 0, 0.5, 1, 1.5 or 2 points for the first engine. Because both engines play both sides of the same opening, the opening's own difficulty largely cancels within the pair, so pair scores carry noticeably less variance than the same games counted one at a time. Less variance means fewer games to reach the same confidence.

The log likelihood ratio uses the normal approximation every practical engine testing tool uses. With `mean` the observed score per game and `sigma squared` the variance of that mean,

```
LLR = (s1 - s0) * (2 * mean - s0 - s1) / (2 * sigma squared)
```

where `s0` and `s1` are the scores corresponding to `elo0` and `elo1` under the logistic Elo model. H1 is accepted at `log((1 - beta) / alpha)`, H0 at `log(beta / (1 - alpha))`.

Two safeguards matter in practice. Empty outcome buckets get a prior of a thousandth of an observation, so a run in which no pair has yet been lost still has a defined variance. And no verdict is issued before `minpairs` pairs (16 by default), because the first few pairs of a match can easily all land in one bucket, which would make the estimated variance meaningless and let the ratio run off to a decision on a handful of games.

## Reproducibility

Fixed-node matches (`nodes=N`) are deterministic and immune to CPU load, so high concurrency does not distort them. Fixed-time matches are not: keep `-concurrency` at or below half the hardware threads, or the time control stops being honest.

`restart=on` gives each game a fresh engine process. With it, a fixed-node self-play run against an identical binary produces a perfect mirror: every pair scores exactly one win and one loss, the score is exactly 50.00%, and every pair lands in the middle bucket. That is worth running occasionally as an end-to-end check of the harness, and it is the setting to use when a run needs to be exactly reproducible.

Without `restart=on`, BetterThanCris does not reproduce a game exactly when it has already played one in the same process: `ucinewgame` clears the transposition table and every history table, yet the first game a process plays still diverges from later ones. This does not bias a match, since both engines are affected equally and the colours are swapped within each pair, but it does cost some of the variance reduction pairing is meant to buy. See HANDOVER.md for the investigation.

## What the arbiter decides

The engines are never asked whether a game is over. The harness detects checkmate, stalemate, the fifty move rule, threefold repetition and insufficient material itself, from its own legal move generation, which is validated against the canonical perft counts by `make sprt-test`.

An engine loses the game outright when it plays an illegal move, exceeds its clock beyond `-timemargin`, fails to answer, or exits. Each of those is recorded in the `-log` file with the position, the clock state and the engine's last output, and the engine is restarted before the next game. A pair that could not be completed is discarded whole rather than counted half, so the pentanomial totals always describe every game that was scored.

## Self-test

`make sprt-test` checks the arbiter against six canonical perft positions, round trips every legal move through SAN and UCI notation over a three ply tree, verifies FEN writing and reading, checks the draw and mate detection, verifies the statistics against hand computed values and their boundary behaviour, and parses a small book. It runs in well under a second and should be run after any change to the harness.
