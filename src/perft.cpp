#include "perft.h"

// perft test function
void perftTest(int depth)
{
    printf("\nPerformance Test\n\n");

    // create move list
    moves moveList[1];

    // gen moves
    generateMoves(moveList);

    // create time variable
    int start = getTime();

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

        // cummulative nodes
        long cumNodes = nodes;

        // call perft driver (recursive)
        perftDriver(depth - 1);

        // old nodes
        long oldNodes = nodes - cumNodes;

        // undo move and restore board variables
        undoBoard();

        // print move
        printf(" move: %s%s%c   nodes: %lld\n", squareNames[getSource(moveList->moves[moveCount])],
                                               squareNames[getTarget(moveList->moves[moveCount])],
                                               promotedPieces[getPromoted(moveList->moves[moveCount])],
                                               oldNodes);

    }

    double timeTaken = getTime() - start;

    // print summary
    printf("\n  Depth: %d\n", depth);
    printf("  Nodes: %lld\n", nodes);
    printf("   Time: %.2fms\n", timeTaken);
    printf("    N/s: %.2f\n", nodes / (timeTaken / 1000));


}
