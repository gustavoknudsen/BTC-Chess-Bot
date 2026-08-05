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

A fixed-node self-play run of a binary against an identical copy of itself must produce a perfect mirror: every pair scores exactly one win and one loss, the score is exactly 50.00%, and every pair lands in the middle pentanomial bucket. That is the end-to-end check that the harness is unbiased, since any error in colour handling, result attribution or pairing breaks the mirror. Run it after touching the harness.

It is also a check on the engine. The first version of this harness did not mirror, which turned out to be an engine bug rather than a harness one: the mobility areas and king-blocker sets were refreshed only by `makeMove`, so the root position of every search was evaluated against whatever the previous search's last node had left behind. `restart=on` gives each game a fresh process and papers over that class of problem, which is how the bug was first isolated; it is still worth keeping for engines whose `ucinewgame` leaves state behind.

## Losses on time

A run at a real time control will occasionally record a loss on time, and the `-log` file says which engine, at which move, and by how much. A handful of overruns of 100 to 200 ms in long endgames is the ordinary case on a busy laptop: the engine's own `Move Overhead` option, 50 ms by default in BetterThanCris, is what reserves time for communication latency, and 50 ms is thin when several games share the cores. Raising it for the run, `'option.Move Overhead=200'`, is the usual fix, and `-timemargin` widens the harness's own grace on top.

Watch the balance rather than the count. Overruns split evenly between the two engines are noise. Overruns concentrated on one engine are a result: that engine is slower per move and is flagging for it, which is a real weakness at that time control, not an artefact.

## What the arbiter decides

The engines are never asked whether a game is over. The harness detects checkmate, stalemate, the fifty move rule, threefold repetition and insufficient material itself, from its own legal move generation, which is validated against the canonical perft counts by `make sprt-test`.

An engine loses the game outright when it plays an illegal move, exceeds its clock beyond `-timemargin`, fails to answer, or exits. Each of those is recorded in the `-log` file with the position, the clock state and the engine's last output, and the engine is restarted before the next game. A pair that could not be completed is discarded whole rather than counted half, so the pentanomial totals always describe every game that was scored.

## Self-test

`probe.py` is a small diagnostic kept alongside the harness: it asks a fresh engine process and an engine process that has already played a game to search the same position, and prints the first `info` line on which they disagree. An engine that carries state across `ucinewgame` shows up immediately, and the depth at which the outputs first differ says whether the cause is in the evaluation or in a search table. It is what isolated the stale mobility area bug. Run it as `python harness/probe.py ./btc27.exe`.

`make sprt-test` checks the arbiter against six canonical perft positions, round trips every legal move through SAN and UCI notation over a three ply tree, verifies FEN writing and reading, checks the draw and mate detection, verifies the statistics against hand computed values and their boundary behaviour, and parses a small book. It runs in well under a second and should be run after any change to the harness.
