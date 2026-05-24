#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Zobrist Hashing
https://www.chessprogramming.org/Zobrist_Hashing

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// random piece keys (for hashing) [piece][square]
extern U64 pieceKeys[12][64];

// random enpassant keys (for hashing) [square]
extern U64 enpassantKeys[64];

// random castling keys (for hashing) (max 16 as 1111 is maximum in binary)
extern U64 castleKeys[16];

// random side key
extern U64 sideKey;

// initialise all random keys
void initRandomKeys();

// generate (almost) unique hash key
U64 generateHashKey();

#endif // ZOBRIST_H
