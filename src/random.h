#ifndef RANDOM_H
#define RANDOM_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Random Functions

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// initiliase pseudorandom number state
extern unsigned int randomState;

// Xorshift algorithm (exclusive or shift)
// https://en.wikipedia.org/wiki/Xorshift
// generate 32 bit legal numbers
unsigned int xorshift32();

// (Tord Romstad's method https://www.chessprogramming.org/Looking_for_Magics)
// generate 64 bit legal numbers
U64 random64();

// generate possible magic number
U64 genMagicNumber();

#endif // RANDOM_H
