#ifndef ATTACKS_H
#define ATTACKS_H

#include "defs.h"
#include "bitboard.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Attack Tables

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

/*
    BitBoard Constants Visualisation

       Not A File              Not H File
  8  0 1 1 1 1 1 1 1       8  1 1 1 1 1 1 1 0
  7  0 1 1 1 1 1 1 1       7  1 1 1 1 1 1 1 0
  6  0 1 1 1 1 1 1 1       6  1 1 1 1 1 1 1 0
  5  0 1 1 1 1 1 1 1       5  1 1 1 1 1 1 1 0
  4  0 1 1 1 1 1 1 1       4  1 1 1 1 1 1 1 0
  3  0 1 1 1 1 1 1 1       3  1 1 1 1 1 1 1 0
  2  0 1 1 1 1 1 1 1       2  1 1 1 1 1 1 1 0
  1  0 1 1 1 1 1 1 1       1  1 1 1 1 1 1 1 0

     a b c d e f g h          a b c d e f g h

       Not AB File             Not GH File
  8  0 0 1 1 1 1 1 1       8  1 1 1 1 1 1 0 0
  7  0 0 1 1 1 1 1 1       7  1 1 1 1 1 1 0 0
  6  0 0 1 1 1 1 1 1       6  1 1 1 1 1 1 0 0
  5  0 0 1 1 1 1 1 1       5  1 1 1 1 1 1 0 0
  4  0 0 1 1 1 1 1 1       4  1 1 1 1 1 1 0 0
  3  0 0 1 1 1 1 1 1       3  1 1 1 1 1 1 0 0
  2  0 0 1 1 1 1 1 1       2  1 1 1 1 1 1 0 0
  1  0 0 1 1 1 1 1 1       1  1 1 1 1 1 1 0 0

     a b c d e f g h          a b c d e f g h


*/

// not A file global variable
extern const U64 notAFile;

// not H file constant
extern const U64 notHFile;

// not AB file constant
extern const U64 notABFile;

// not HG file constant
extern const U64 notGHFile;

// bishops occupancy bit count lookup table
extern const int bishopBits[64];

// rooks occupancy bit count lookup table
extern const int rookBits[64];

// rook magic numbers
extern U64 rookMagicNumbers[64];

// bishop magic numbers
extern U64 bishopMagicNumbers[64];

// bishop attack masks array
extern U64 bishopMasks[64];

// rook attack masks array
extern U64 rookMasks[64];

// bishop attacks table ([square][occupancies/block]) 512 possible occupancies for bishops
extern U64 bishopAttacks[64][512];

// rook attacks table ([square][occupancies/block])
extern U64 rookAttacks[64][4096];

// create pawn table (two dimensional array of [side to move][square])
extern U64 pawnAttacks[2][64];

// create knight attack table (one dimensional array of [square])
extern U64 knightAttacks[64];

// create king attack table (one dimensional array of [square])
extern U64 kingAttacks[64];

// get pawn attacks (mask)
U64 maskPawnAttacks(int side, int square);

// get knight attacks (mask)
U64 maskKnightAttacks(int square);

// get king attacks (mask)
U64 maskKingAttacks(int square);

// get bishop attacks (mask)
U64 maskBishopAttacks(int square);

// get rook attacks (mask)
U64 maskRookAttacks(int square);

// get bishop attacks on the fly (in the case of a piece 'blocking')
U64 bishopAttacksOTF(int square, U64 block);

// get bishop attacks on the fly (in the case of a piece 'blocking')
U64 rookAttacksOTF(int square, U64 block);

// initialise leapers attacks (pawns, knights, kings)
void initLeapersAttacks();

// function that sets occupancies
U64 setOccupancy(int index, int bitsInMask, U64 attackMask);

// initialise slider piece attack tables (bishop flag)
void initSlidersAttacks(int bishop);

// get bishop attacks (static inline as it is used in the move gen)
static inline U64 getBishopAttacks(int square, U64 occupancy)
{
    // get the bishop valid attacks
    occupancy &= bishopMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopBits[square];

    // return
    return bishopAttacks[square][occupancy];
}

// get rook attacks (static inline as it is used in the move gen)
static inline U64 getRookAttacks(int square, U64 occupancy)
{
    // get the rook valid attacks
    occupancy &= rookMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookBits[square];

    // return
    return rookAttacks[square][occupancy];

}

// get queen attacks (static inline as it is used in the move gen)
static inline U64 getQueenAttacks(int square, U64 occupancy)
{
    U64 final = 0ULL;

    // get bishop occupancies
    U64 bishopOccupancies = occupancy;

    // get rook occupancies
    U64 rookOccupancies = occupancy;

    // get the bishop valid attacks
    bishopOccupancies &= bishopMasks[square];
    bishopOccupancies *= bishopMagicNumbers[square];
    bishopOccupancies >>= 64 - bishopBits[square];

    // get bishop attacks
    final = bishopAttacks[square][bishopOccupancies];

    // get the rook valid attacks
    rookOccupancies &= rookMasks[square];
    rookOccupancies *= rookMagicNumbers[square];
    rookOccupancies >>= 64 - rookBits[square];

    // get bishop attacks
    final |= rookAttacks[square][rookOccupancies];

    // combine and return
    return final;
}

#endif // ATTACKS_H
