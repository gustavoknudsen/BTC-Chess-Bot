#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Bitboard Macros
                   and Manip

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

extern uint8_t PopCnt16[1 << 16];

// set macro (sets bit to 1 in the square)
#define setBit(bitboard, square) ((bitboard) |= (1ULL << (square)))

// get macro (gets 1 or 0 from bitboard)
#define getBit(bitboard, square) ((bitboard) & (1ULL << (square) ))

// pop (remove) macro (unsets bit)
#define popBit(bitboard, square) \
    do { \
        if (getBit((bitboard), (square))) { \
            (bitboard) ^= (1ULL << (square)); \
        } \
    } while (0) // do { ... } while (0) common idiom in macros to make them behave like a single statement

// function that counts bits in bitboard (static inline int for performance)
static inline int countBits(U64 bitboard)
{
    // create counter
    int count = 0;

    // loop while bitboard > 0
    while (bitboard)
    {
        // increase counter
        count++;
        // remove a bit from bitboard
        bitboard &= bitboard - 1;
    }

    // return count
    return count;
}

// Function to get the index of the most significant set bit (MSB)
static inline int getLSFBIndex(U64 bitboard)
{
    if (bitboard)
    {
        unsigned long idx;
        _BitScanForward64(&idx, bitboard);
        return idx;
    }
    else
    {
        // no bits
        return -1;
    }
}

// Function to get the index of the most significant set bit (MSB)
static inline int getMSBIndex(U64 bitboard)
{
    if (bitboard)
    {
        unsigned long idx;
        _BitScanReverse64(&idx, bitboard);
        return idx;
    }
    else
    {
        // no bits
        return -1;
    }
}

// function that shifts bitboard to a direction
U64 shift(U64 bitboard, int direction);

// Shift a bitboard up one rank (toward white's side)
static inline U64 shiftUp(U64 bitboard) {
    return shift(bitboard, NORTH);
}

// Shift a bitboard down one rank (toward black's side)
static inline U64 shiftDown(U64 bitboard) {
    return shift(bitboard, SOUTH);
}

// print bitboard
void printBitBoard(U64 bitboard);

#endif // BITBOARD_H
