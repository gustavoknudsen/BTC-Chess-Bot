"""Cold vs warm probe.

Starts two processes of the same engine. One is fresh, the other has already
played a game and then been given ucinewgame. Both are asked to search the same
position and their info output is compared line by line. The first differing
line localises where a warm process stops behaving like a cold one.
"""
import subprocess, sys

ENGINE = sys.argv[1] if len(sys.argv) > 1 else './btc27.exe'
NODES = sys.argv[2] if len(sys.argv) > 2 else '8000'

FEN = "rnb1kb1r/pp2nppp/2pp1q2/4p3/4P3/2NP2N1/PPP1BPPP/R1BQK2R b KQkq - 2 6"
WARM = ["g7g6", "e1g1", "h7h5", "f2f4", "e5f4", "f1f4", "f6d4", "f4f2", "b8d7", "e2f1"]


def start():
    p = subprocess.Popen([ENGINE], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         text=True, bufsize=1)
    p.stdin.write("uci\n"); p.stdin.flush()
    while "uciok" not in p.stdout.readline(): pass
    p.stdin.write("setoption name Hash value 16\nisready\n"); p.stdin.flush()
    while "readyok" not in p.stdout.readline(): pass
    return p


def newgame(p):
    p.stdin.write("ucinewgame\nisready\n"); p.stdin.flush()
    while "readyok" not in p.stdout.readline(): pass


def go(p, moves):
    cmd = f"position fen {FEN}" + (" moves " + " ".join(moves) if moves else "")
    p.stdin.write(f"{cmd}\ngo nodes {NODES}\n"); p.stdin.flush()
    info = []
    while True:
        line = p.stdout.readline().strip()
        if line.startswith("info") and "string" not in line:
            info.append(line)
        if line.startswith("bestmove"):
            return info, line


cold = start(); newgame(cold)
warm = start(); newgame(warm)
for i in range(0, 10, 2):
    go(warm, WARM[:i])
newgame(warm)

coldInfo, coldBest = go(cold, [])
warmInfo, warmBest = go(warm, [])
for p in (cold, warm):
    p.stdin.write("quit\n"); p.stdin.flush()

for a, b in zip(coldInfo, warmInfo):
    if a != b:
        print("DIVERGES")
        print("  cold:", a)
        print("  warm:", b)
        break
else:
    if len(coldInfo) == len(warmInfo) and coldBest == warmBest:
        print("IDENTICAL   (%d info lines, %s)" % (len(coldInfo), coldBest))
    else:
        print("DIVERGES in line count:", len(coldInfo), len(warmInfo), coldBest, warmBest)
