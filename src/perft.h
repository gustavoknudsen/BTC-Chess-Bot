#ifndef PERFT_H
#define PERFT_H

#include "defs.h"
#include "move.h"
#include "movegen.h"
#include "make_move.h"
#include "copy_make.h"
#include "position.h"
#include "timeman.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Perft Function(s)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// perft driver (for testing)
static inline void perftDriver(int depth)
{
    // escape condition
    if (depth == 0)
    {
        // increment nodes count
        nodes++;
        return;
    }

    // create move list
    moves moveList[1];

    // gen moves
    generateMoves(moveList);

    for (int moveCount = 0; moveCount < moveList->count; moveCount++)
    {
        // copy the board and variables
        copyBoard();

        // make the move
        if (!makeMove(moveList->moves[moveCount], allMoves))
        {
            continue;
        }
        else
        {
            counter++;
        }

        // call perft driver (recursive)
        perftDriver(depth - 1);

        // undo move and restore board variables
        undoBoard();

        /* DEBUG HASH */
        /*
        U64 hashFromScratch = generateHashKey();
        if (hashKey != hashFromScratch)
        {
            printf("\nmake move\n");
            printf("move: "); printMove(moveList->moves[moveCount]);
            printBoard();
            printf("hash key should be %llx\n", hashFromScratch);
            getchar();
        }
        */
    }
}

// perft test function
void perftTest(int depth);

#endif // PERFT_H
