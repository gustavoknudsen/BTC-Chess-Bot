#ifndef POSITION_H
#define POSITION_H

#include "defs.h"
#include "bitboard.h"
#include "attacks.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Initial Setup
                  (constants)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// conversion of index to square name
extern const char* squareNames[];

// ASCII pieces for board printing
extern char ASCIIpieces[13];

extern int charPieces[128];

void initCharPieces();

extern const U64 lightSquares;
extern const U64 darkSquares;
extern const U64 centerSquares;

/*
Castling Key

   king & rooks didn't move:     1111 & 1111  =  1111    15

          white king  moved:     1111 & 1100  =  1100    12
    white king's rook moved:     1111 & 1110  =  1110    14
   white queen's rook moved:     1111 & 1101  =  1101    13

           black king moved:     1111 & 0011  =  1011    3
    black king's rook moved:     1111 & 1011  =  1011    11
   black queen's rook moved:     1111 & 0111  =  0111    7
*/
// castling rights update
extern const int castlingRights[64];

// piece tables [stage][piece][square] (taken from PESTO)
extern const int PieceTables[2][6][64];

// mirror square values
extern const int mirrorScore[128];

/*
    Example masks for pawn structures

          Rank mask            File mask           Isolated mask        Passed pawn mask
        for square a6        for square f2         for square g2          for square c4

    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 1 0 0    8  0 0 0 0 0 1 0 1     8  0 1 1 1 0 0 0 0
    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 1 0 0    7  0 0 0 0 0 1 0 1     7  0 1 1 1 0 0 0 0
    6  1 1 1 1 1 1 1 1    6  0 0 0 0 0 1 0 0    6  0 0 0 0 0 1 0 1     6  0 1 1 1 0 0 0 0
    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 1 0 0    5  0 0 0 0 0 1 0 1     5  0 1 1 1 0 0 0 0
    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 1 0 0    4  0 0 0 0 0 1 0 1     4  0 0 0 0 0 0 0 0
    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 1 0 0    3  0 0 0 0 0 1 0 1     3  0 0 0 0 0 0 0 0
    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 1 0 0    2  0 0 0 0 0 1 0 1     2  0 0 0 0 0 0 0 0
    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 1 0 0    1  0 0 0 0 0 1 0 1     1  0 0 0 0 0 0 0 0

       a b c d e f g h       a b c d e f g h       a b c d e f g h        a b c d e f g h
*/

// rank mask bitboard
extern U64 rankMask[64];

// file mask bitboard
extern U64 fileMask[64];

// these are filled by initEvalMasks after fileMask and rankMask are built
extern U64 centerFiles;
extern U64 outpostRanksWhite;
extern U64 outpostRanksBlack;

// king-flank file groups [file] and per-side camp (own half + middle), for king safety
extern U64 kingFlankMask[8];
extern U64 campMask[2];

// squares strictly between two aligned squares (empty if not on a line), for blocker/pin detection
extern U64 betweenMask[64][64];

//  isolated pawn mask bitboard
extern U64 isolatedMask[64];

// passed white pawn mask
extern U64 whitePassedMask[64];

// passed pawn mask
extern U64 blackPassedMask[64];

// white opposed pawn mask
extern U64 whiteOpposedMask[64];

// black opposed pawn mask
extern U64 blackOpposedMask[64];

// support mask for white pawns
extern U64 whiteSupportMask[64];

// support mask for black pawns
extern U64 blackSupportMask[64];

// phalanx mask for each square (pawns next to each other on the same rank)
extern U64 phalanxMask[64];

// white king zone masks (king moves and 3 squares in front towards enemy king)
extern U64 whiteKingZoneMask[64];

// black king zone masks (king moves and 3 squares in front towards enemy king)
extern U64 blackKingZoneMask[64];

// white blocked pawn mask
extern U64 whiteBlockedMask[64];

// black blocked pawn mask
extern U64 blackBlockedMask[64];

// masks for pins in [direction][square]
extern U64 pinnedMasks[8][64];

// masks for forward ranks [side][square] (if [black][d3], all sqs on rank 1 and 2 will return )
extern U64 forwardRanksMasks[2][64];

// adjacent files mask for each file
extern U64 adjacentFilesMask[8];

// get rank of square
extern const int getRank[64];

// get file of square
extern const int getFile[64];

// game phase scores
extern const int openingPhaseScore; // if game stage score > 6192, in pure opening
extern const int endgamePhaseScore; // if game stage score < 518, in pure endgame

// keeping track of all attacks by certain pieces [side][piece]
extern U64 pieceAttackTables[2][7];

// keeping track of double attacks by pawns [side]
extern U64 pawnDoubleTables[2];

// squares attacked by 2 pieces of the colour
extern U64 attackedBy2[2];

// squares that are weak (not strongly protected or under attack)
extern U64 weak[2];

// safe squares
extern U64 safe[2];

// strongly protected squares
extern U64 stronglyProtected[2];

// strongly protected non-pawn pieces
extern U64 defended[2];

// non pawn enemies
extern U64 nonPawnEnemies[2];

// current and potential pawn attacks
extern U64 pawnSpans[2];

extern U64 mobilityAreaWhite;
extern U64 mobilityAreaBlack;

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Standard Board
                  Definition
https://www.chessprogramming.org/Bitboard_Board-Definition

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// initialise piece bitboards (6 black pieces, 6 white pieces)
extern U64 bitboards[12];

// initialise occupancy bitboards (white occupancies, black occupancies, all occupancies)
extern U64 occupancies[3];

// current side (side to move)
extern int side;

// enpassant square
extern int enpassant;

// castling rights
extern int castle;

// (almost) unique position identifier (hash key / position id)
extern U64 hashKey;

// repetition table
extern U64 repetitionTable[1000]; // 1000 -> number of plies, assuming maximum 500 moves game

// repetition index (starts at 0)
extern int repetitionIndex;

// half move counter (ply)
extern int ply;

// fifty move rule
extern int fifty;

// move number
extern int moveNumber;

struct PositionCache {
    // store the full 64-bit zobrist key, not a truncated int
    // truncating gave us many false cache hits across unrelated positions
    U64 positionHash;
    U64 pinnedPieces;
    U64 pinnedRays[64];      // Store pin rays for each square
    U64 regularExcludeMask;  // Regular exclusion mask
    U64 queenExcludeMask;    // Queen-specific exclusion mask
    int side;                // Store which side the cache is for
};

extern PositionCache positionCache;

// parse FEN
void parseFEN(const char *fen);

// print board function
void printBoard();

#endif // POSITION_H
