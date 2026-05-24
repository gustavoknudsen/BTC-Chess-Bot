#ifndef DEFS_H
#define DEFS_H

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Shared Definitions

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// headers
#include <stdio.h>
#include <string.h>
#include <sysinfoapi.h>
#include <unistd.h>
#include <windows.h>
#include <cmath>
#include <stdint.h>
#include <stdlib.h>

// create the bitboard data type (64Bit unsigned integers)
#define U64 unsigned long long

// FEN dedug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

// notation of squares (a8 is assinged 0, b8 is assigned 1, etc.)
enum
{
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, noSq
};

// castling bits
/*
   0001    white king can castle to the king side
   0010    white king can castle to the queen side
   0100    black king can castle to the king side
   1000    black king can castle to the queen side
*/
enum
{
    wk = 1, wq = 2, bk = 4, bq = 8
};

// pieces (caps for white, lower for black)
enum
{
    P, N, B, R, Q, K, p, n, b, r, q, k
};

enum
{
    allPieces = 6
};

// colours (white=0, black=1, both=2)
enum
{
    white, black, both
};

// game phases
enum {
    opening,
    endgame,
    middlegame
};

// directions (currently no need for NESW)
enum {
    NORTH,
    NORTHEAST,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST,
    NORTHWEST,
};

// move type key (all moves = 0, captures = 1)
enum
{
    allMoves, capturesOnly
};

// highest possible value
#define infinity 50000
// mate value "higher bound" of score for a mate
#define mateValue 49000
// mate score "lower bound" of score for a mate
#define mateScore 48000

// max ply we can reach in a search
#define maxPly 64

// no hash entry
#define noHashEntry 100000 // to be outside of alpha-beta bounds

// transposition table flags
#define hashFlagExact 0
#define hashFlagAlpha 1
#define hashFlagBeta 2

#endif // DEFS_H
