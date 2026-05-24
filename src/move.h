#ifndef MOVE_H
#define MOVE_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                  Move Macros

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

/*
    Moves are represented by 24 bits and each bit/series of bits represent the following values:

    0000 0000 0000 0000 0011 1111       0x3F          source square
    0000 0000 0000 1111 1100 0000       0xFC0         target square
    0000 0000 1111 0000 0000 0000       0xF000        piece
    0000 1111 0000 0000 0000 0000       0xF0000       promoted piece
    0001 0000 0000 0000 0000 0000       0x100000      capture flag
    0010 0000 0000 0000 0000 0000       0x200000      double push flag
    0100 0000 0000 0000 0000 0000       0x400000      enpassant capture flag
    1000 0000 0000 0000 0000 0000       0x800000      castling flag
*/

// encode move
// each arg is wrapped in parens so callers can pass expressions safely
#define encodeMove(source, target, piece, promoted, capture, double, enpassant, castling) \
    ((source) | ((target) << 6) | ((piece) << 12) | ((promoted) << 16) | \
     ((capture) << 20) | ((double) << 21) | ((enpassant) << 22) | ((castling) << 23))

// get move source square
#define getSource(move) ((move) & 0x3F)

// get move target square
#define getTarget(move) (((move) & 0xFC0) >> 6)

// get move piece
#define getPiece(move) (((move) & 0xF000) >> 12)

// get promoted piece
#define getPromoted(move) (((move) & 0xF0000) >> 16)

// get capture flag
#define getCapture(move) (((move) & 0x100000) >> 20)

// get double push flag
#define getDouble(move) (((move) & 0x200000) >> 21)

// get enpassant flag
#define getEnpassant(move) (((move) & 0x400000) >> 22)

// get castling flag
#define getCastle(move) (((move) & 0x800000) >> 23)

// move list structure (kind of like a dict)
typedef struct {
    // create moves array
    int moves[256];

    // create move count variable (index of the move)
    int count;
} moves;

// add move to list function
static inline void addMove(moves *moveList, int move)
{
    // store the move into the list
    moveList->moves[moveList->count] = move;

    // increment count
    moveList->count++;
}

// promoted pieces key (always lowercase regardless of colour according to uci protocol)
extern char promotedPieces[256];
// Function to initialize the promotedPieces map
void initPromotedPieces();

// print move (for UCI)
void printMove(int move);

// print move list
// print move list (for debugging)
void printMoveList(moves *moveList);

#endif // MOVE_H
