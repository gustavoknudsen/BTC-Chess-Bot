#include "zobrist.h"
#include "bitboard.h"
#include "position.h"
#include "random.h"

// random piece keys (for hashing) [piece][square]
U64 pieceKeys[12][64];

// random enpassant keys (for hashing) [square]
U64 enpassantKeys[64];

// random castling keys (for hashing) (max 16 as 1111 is maximum in binary)
U64 castleKeys[16];

// random side key
U64 sideKey;

// initialise all random keys
void initRandomKeys()
{
    // re-seed the global pseudorandom state so the keys are always the same
    // across runs no matter what consumed the rng before this point
    randomState = 1804289383;

    // loop over all the pieces
    for (int piece = P; piece <= k; piece++)
    {
        // loop over all squares
        for (int square = 0; square < 64; square++)
        {
            // initialise piece key by giving a random U64 value
            pieceKeys[piece][square] = random64();
        }
    }

    // loop over board squares
    for (int square = 0; square < 64; square++)
    {
        // initialise enpassant key by giving a random U64 value
        enpassantKeys[square] = random64();
    }

    // loop over all possible castling keys
    for (int index = 0; index < 16; index++)
    {
        // initialise castle keys
        castleKeys[index] = random64();
    }

    // initialise sideKey
    sideKey = random64();

}

// generate (almost) unique hash key
U64 generateHashKey()
{
    // create final key variable and start at 0
    U64 finalKey = 0ULL;

    // piece bitboard copy (like in the makeMove function)
    U64 bitboard;

    // loop over piece bitboards (to add all pieces to hash key)
    for (int piece = P; piece <= k; piece++)
    {
        // get piece bitboard copy
        bitboard = bitboards[piece];

        // loop over all pieces in bitboard
        while (bitboard)
        {
            // get square value
            int square = getLSFBIndex(bitboard);

            // hash all the pieces
            finalKey ^= pieceKeys[piece][square];

            // pop current piece/bit
            popBit(bitboard, square);
        }
    }

    // if enpassant
    if (enpassant != noSq)
    {
        // hash the enpassant square
        finalKey ^= enpassantKeys[enpassant];
    }

    // add castling rights to hash
    finalKey ^= castleKeys[castle];

    // hash the side if black to move
    if (side == black) finalKey ^= sideKey;

    // return the hash key
    return finalKey;
}
