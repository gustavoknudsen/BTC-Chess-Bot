#include "random.h"

// initiliase pseudorandom number state
unsigned int randomState = 1804289383;

// Xorshift algorithm (exclusive or shift)
// https://en.wikipedia.org/wiki/Xorshift
// generate 32 bit legal numbers
unsigned int xorshift32()
{
    // get state
    unsigned int number = randomState;

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // update state
    randomState = number;

    // return state
    return number;
}

// (Tord Romstad's method https://www.chessprogramming.org/Looking_for_Magics)
// generate 64 bit legal numbers
U64 random64()
{
    // initialise 4 random numbers
    U64 n1, n2, n3, n4;
    n1 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB
    n2 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB
    n3 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB
    n4 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB

    // return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

// generate possible magic number
U64 genMagicNumber()
{
    return (random64() & random64() & random64());
}
