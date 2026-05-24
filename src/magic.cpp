#include "magic.h"
#include "bitboard.h"

// function that finds correct magic number (relevant bits from the lookup table, bishop is a flag to see if bishop or rook)
U64 findMagic(int square, int relevantBits, int bishop)
{
    // initialise the occupancy
    U64 occupancy[4096]; // max occupancies for a rook

    // create attack tables
    U64 attacks[4096];

    // create used attacks
    U64 usedAttacks[4096];

    // create attack mask for current piece
    U64 attackMask = bishop ? maskBishopAttacks(square) : maskRookAttacks(square);

    // create occupancy indices
    int occupancyI = 1 << relevantBits;

    // loop over occupancyI
    for (int index = 0; index < occupancyI; index++)
    {
        // create occupancies for all indices and store them
        occupancy[index] = setOccupancy(index, relevantBits, attackMask);

        // create attacks
        attacks[index] = bishop ? bishopAttacksOTF(square, occupancy[index]) : rookAttacksOTF(square, occupancy[index]);

    }

    // test magic number generated
    for (int count = 0; count < 100000000; count++)
    {
        // get possible magic number
        U64 magicNumber = genMagicNumber();

        // if not a proper magic number, skip
        if (countBits((attackMask * magicNumber) & 0xFF00000000000000) < 6)
        {
            continue;
        }

        // initialise all elements of the array usedAttacks
        memset(usedAttacks, 0ULL, sizeof(usedAttacks));

        // create index and fail flag
        int index, fail;

        // test magic index
        for (index = 0, fail = 0; !fail && index < occupancyI; index++)
        {
            // create magic index
            int magicIndex = (int)((occupancy[index] * magicNumber) >> (64 - relevantBits));

            // if magic index works (good magic number)
            if (usedAttacks[magicIndex] == 0ULL)
            {
                // initialise usedAttacks
                usedAttacks[magicIndex] = attacks[index];
            }
            // else if magic index doesnt work
            else if (usedAttacks[magicIndex] != attacks[index])
            {
                // update fail flag
                fail = 1;
            }
        }
        // found a working magic number
        if (!fail)
        {
            // return the magic number
            return magicNumber;
        }
    }

    // didn't find a working magic number (pls never get to this)
    printf("magic number fail\n");
    return 0ULL;
}

// function that initialises magic numbers
void initMagicNumbers()
{
    // loop over all squares (rook)
    for (int square = 0; square < 64; square++)
    {
        // rook magic numbers
        rookMagicNumbers[square] = findMagic(square, rookBits[square], 0);
    }
    printf("\n\n");
    // loop over all squares (bishop)
    for (int square = 0; square < 64; square++)
    {
        // rook magic numbers
        bishopMagicNumbers[square] = findMagic(square, bishopBits[square], 1);
    }
}
