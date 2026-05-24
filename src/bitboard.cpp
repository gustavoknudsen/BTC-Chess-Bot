#include "bitboard.h"

uint8_t PopCnt16[1 << 16];

// function that shifts bitboard to a direction
U64 shift(U64 bitboard, int direction) {
    switch (direction) {
        case SOUTH:     return bitboard << 8;
        case SOUTHEAST: return (bitboard & 0x7F7F7F7F7F7F7F00) << 9;
        case EAST:      return (bitboard & 0x7F7F7F7F7F7F7F7F) << 1;
        case NORTHEAST: return (bitboard & 0x007F7F7F7F7F7F7F) >> 7;
        case NORTH:     return bitboard >> 8;
        case NORTHWEST: return (bitboard & 0x00FEFEFEFEFEFEFE) >> 9;
        case WEST:      return (bitboard & 0xFEFEFEFEFEFEFEFE) >> 1;
        case SOUTHWEST: return (bitboard & 0xFEFEFEFEFEFEFE00) << 7;
        default:        return 0;
    }
}

// print bitboard
void printBitBoard(U64 bitboard)
{
    // print new line in the beginning
    printf("\n");

    // loop through rows
    for (int row = 0; row < 8; row++)
    {
        // loop through cols
        for (int col = 0; col < 8; col++)
        {
            // create square index value for position
            int index = row * 8 + col;

            // print rows notation
            if (col == 0)
            {
                printf(" %d   ", 8 - row);
            }

            // if the bit at index is set, print 1, else print 0
            printf(" %d ", (getBit(bitboard, index)) ? 1 : 0);
        }
        // print new line for new row
        printf("\n");
    }
    // print row notation
    printf("\n      a  b  c  d  e  f  g  h\n\n");

    // print bitboard as value in decimal
    printf("Bitboard: %llu \n\n", bitboard);
}
