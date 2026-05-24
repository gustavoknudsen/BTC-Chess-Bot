#ifndef MAGIC_H
#define MAGIC_H

#include "defs.h"
#include "attacks.h"
#include "random.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Magic Numbers
            Tom Romstad's Proposal
https://www.chessprogramming.org/Looking_for_Magics
      method was used to generate magic numbers
    but does not run when engine is ran naturally
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// function that finds correct magic number (relevant bits from the lookup table, bishop is a flag to see if bishop or rook)
U64 findMagic(int square, int relevantBits, int bishop);

// function that initialises magic numbers
void initMagicNumbers();

#endif // MAGIC_H
