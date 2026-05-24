#include "movegen.h"

// promoted pieces key (always lowercase regardless of colour according to uci protocol)
char promotedPieces[256];
// Function to initialize the promotedPieces map
void initPromotedPieces()
{
    promotedPieces[4] = 'q';
    promotedPieces[3] = 'r';
    promotedPieces[1] = 'n';
    promotedPieces[2] = 'b';
    promotedPieces[10] = 'q';
    promotedPieces[9] = 'r';
    promotedPieces[7] = 'n';
    promotedPieces[8] = 'b';
}

// print move (for UCI)
void printMove(int move)
{
    // If the promotion piece is 0 or not valid, default to no promotion
    int promotedPiece = getPromoted(move);
    if (promotedPiece == 0)
    {
        printf("%s%s", squareNames[getSource(move)], squareNames[getTarget(move)]);
    }
    else
    {
        printf("%s%s%c", squareNames[getSource(move)], squareNames[getTarget(move)], promotedPieces[promotedPiece]);
    }

}

// print move list
// print move list (for debugging)
void printMoveList(moves *moveList)
{
    // don't print if empty
    if (!moveList->count)
    {
        printf("\nNo moves in list\n\n");
        return;
    }

    printf("\n        move     piece    capture    double    enpas    castle\n\n");
    // loop over moves in the list
    for (int moveIndex = 0; moveIndex < moveList->count; moveIndex++)
    {
        // create move
        int move = moveList->moves[moveIndex];

        // copy board
        copyBoard();

        // print move
        printf("%d%s      %s%s%c      %c         %d         %d         %d        %d\n",
                                        moveIndex + 1,
                                        (moveIndex + 1 < 10) ? " " : "",
                                        squareNames[getSource(move)],
                                        squareNames[getTarget(move)],
                                        getPromoted(move) ? promotedPieces[getPromoted(move)] : ' ',
                                        ASCIIpieces[getPiece(move)],
                                        getCapture(move),
                                        getDouble(move),
                                        getEnpassant(move),
                                        getCastle(move));

        undoBoard();
    }
    printf("\n      Total Moves: %d\n\n", moveList->count);

}

// print attacked squares (for testing)
void printAttackedSquares(int sideAttacker)
{
    // loop over all squares
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            // get square value
            int square = rank * 8 + file;

            if (!file)
            {
                printf("   %d  ", 8 - rank);
            }

            // check if square is under attack
            printf("%d ", isUnderAttack(square, sideAttacker) ? 1 : 0);
        }
        // print new line
        printf("\n");

    }
    // print files
    printf("\n      a b c d e f g h\n");

}
