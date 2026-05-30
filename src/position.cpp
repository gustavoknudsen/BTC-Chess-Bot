#include "position.h"
#include "zobrist.h"
#include "evaluation.h"

// conversion of index to square name
const char* squareNames[] =
{
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

// ASCII pieces for board printing
char ASCIIpieces[13] = "PNBRQKpnbrqk";

int charPieces[128];

void initCharPieces() {
    charPieces['P'] = P;
    charPieces['N'] = N;
    charPieces['B'] = B;
    charPieces['R'] = R;
    charPieces['Q'] = Q;
    charPieces['K'] = K;
    charPieces['p'] = p;
    charPieces['n'] = n;
    charPieces['b'] = b;
    charPieces['r'] = r;
    charPieces['q'] = q;
    charPieces['k'] = k;
}

extern const U64 lightSquares = 0x55AA55AA55AA55AAULL;
extern const U64 darkSquares = 0xAA55AA55AA55AA55ULL;
extern const U64 centerSquares = (1ULL << d4) | (1ULL << e4) | (1ULL << d5) | (1ULL << e5);

// castling rights update
extern const int castlingRights[64] =
{
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

// piece tables [stage][piece][square] (taken from PESTO)
extern const int PieceTables[2][6][64] = {
    {
        // Mid-game tables
        {
            // mgPawnTable
              0,   0,   0,   0,   0,   0,  0,   0,
             98, 134,  61,  95,  68, 126, 34, -11,
             -6,   7,  26,  31,  65,  56, 25, -20,
            -14,  13,   6,  21,  23,  12, 17, -23,
            -27,  -2,  -5,  12,  17,   6, 10, -25,
            -26,  -4,  -4, -10,   3,   3, 33, -12,
            -35,  -1, -20, -23, -15,  24, 38, -22,
              0,   0,   0,   0,   0,   0,  0,   0,
        },
        {
            // mgKnightTable
            -167, -89, -34, -49,  61, -97, -15, -107,
             -73, -41,  72,  36,  23,  62,   7,  -17,
             -47,  60,  37,  65,  84, 129,  73,   44,
              -9,  17,  19,  53,  37,  69,  18,   22,
             -13,   4,  16,  13,  28,  19,  21,   -8,
             -23,  -9,  12,  10,  19,  17,  25,  -16,
             -29, -53, -12,  -3,  -1,  18, -14,  -19,
            -105, -21, -58, -33, -17, -28, -19,  -23,
        },
        {
            // mgBishopTable
            -29,   4, -82, -37, -25, -42,   7,  -8,
            -26,  16, -18, -13,  30,  59,  18, -47,
            -16,  37,  43,  40,  35,  50,  37,  -2,
             -4,   5,  19,  50,  37,  37,   7,  -2,
             -6,  13,  13,  26,  34,  12,  10,   4,
              0,  15,  15,  15,  14,  27,  18,  10,
              4,  15,  16,   0,   7,  21,  33,   1,
            -33,  -3, -14, -21, -13, -12, -39, -21,
        },
        {
            // mgRookTable
             32,  42,  32,  51, 63,  9,  31,  43,
             27,  32,  58,  62, 80, 67,  26,  44,
             -5,  19,  26,  36, 17, 45,  61,  16,
            -24, -11,   7,  26, 24, 35,  -8, -20,
            -36, -26, -12,  -1,  9, -7,   6, -23,
            -45, -25, -16, -17,  3,  0,  -5, -33,
            -44, -16, -20,  -9, -1, 11,  -6, -71,
            -19, -13,   1,  17, 16,  7, -37, -26,
        },
        {
            // mgQueenTable
            -28,   0,  29,  12,  59,  44,  43,  45,
            -24, -39,  -5,   1, -16,  57,  28,  54,
            -13, -17,   7,   8,  29,  56,  47,  57,
            -27, -27, -16, -16,  -1,  17,  -2,   1,
             -9, -26,  -9, -10,  -2,  -4,   3,  -3,
            -14,   2, -11,  -2,  -5,   2,  14,   5,
            -35,  -8,  11,   2,   8,  15,  -3,   1,
             -1, -18,  -9,  10, -15, -25, -31, -50,
        },
        {
            // mgKingTable
            -65,  23,  16, -15, -56, -34,   2,  13,
             29,  -1, -20,  -7,  -8,  -4, -38, -29,
             -9,  24,   2, -16, -20,   6,  22, -22,
            -17, -20, -12, -27, -30, -25, -14, -36,
            -49,  -1, -27, -39, -46, -44, -33, -51,
            -14, -14, -22, -46, -44, -30, -15, -27,
              1,   7,  -8, -64, -43, -16,   9,   8,
            -15,  36,  12, -54,   8, -28,  24,  14,
        }
    },
    {
        // End-game tables
        {
            // egPawnTable
              0,   0,   0,   0,   0,   0,   0,   0,
            178, 173, 158, 134, 147, 132, 165, 187,
             94, 100,  85,  67,  56,  53,  82,  84,
             32,  24,  13,   5,  -2,   4,  17,  17,
             13,   9,  -3,  -7,  -7,  -8,   3,  -1,
              4,   7,  -6,   1,   0,  -5,  -1,  -8,
             13,   8,   8,  10,  13,   0,   2,  -7,
              0,   0,   0,   0,   0,   0,   0,   0,
        },
        {
            // egKnightTable
            -58, -38, -13, -28, -31, -27, -63, -99,
            -25,  -8, -25,  -2,  -9, -25, -24, -52,
            -24, -20,  10,   9,  -1,  -9, -19, -41,
            -17,   3,  22,  22,  22,  11,   8, -18,
            -18,  -6,  16,  25,  16,  17,   4, -18,
            -23,  -3,  -1,  15,  10,  -3, -20, -22,
            -42, -20, -10,  -5,  -2, -20, -23, -44,
            -29, -51, -23, -15, -22, -18, -50, -64,
        },
        {
            // egBishopTable
            -14, -21, -11,  -8, -7,  -9, -17, -24,
             -8,  -4,   7, -12, -3, -13,  -4, -14,
              2,  -8,   0,  -1, -2,   6,   0,   4,
             -3,   9,  12,   9, 14,  10,   3,   2,
             -6,   3,  13,  19,  7,  10,  -3,  -9,
            -12,  -3,   8,  10, 13,   3,  -7, -15,
            -14, -18,  -7,  -1,  4,  -9, -15, -27,
            -23,  -9, -23,  -5, -9, -16,  -5, -17,
        },
        {
            // egRookTable
            13, 10, 18, 15, 12,  12,   8,   5,
            11, 13, 13, 11, -3,   3,   8,   3,
             7,  7,  7,  5,  4,  -3,  -5,  -3,
             4,  3, 13,  1,  2,   1,  -1,   2,
             3,  5,  8,  4, -5,  -6,  -8, -11,
            -4,  0, -5, -1, -7, -12,  -8, -16,
            -6, -6,  0,  2, -9,  -9, -11,  -3,
            -9,  2,  3, -1, -5, -13,   4, -20,
        },
        {
            // egQueenTable
             -9,  22,  22,  27,  27,  19,  10,  20,
            -17,  20,  32,  41,  58,  25,  30,   0,
            -20,   6,   9,  49,  47,  35,  19,   9,
              3,  22,  24,  45,  57,  40,  57,  36,
            -18,  28,  19,  47,  31,  34,  39,  23,
            -16, -27,  15,   6,   9,  17,  10,   5,
            -22, -23, -30, -16, -16, -23, -36, -32,
            -33, -28, -22, -43,  -5, -32, -20, -41,
        },
        {
            // egKingTable
            -74, -35, -18, -18, -11,  15,   4, -17,
            -12,  17,  14,  17,  17,  38,  23,  11,
             10,  17,  23,  15,  20,  45,  44,  13,
             -8,  22,  24,  27,  26,  33,  26,   3,
            -18,  -4,  21,  24,  27,  23,   9, -11,
            -19,  -3,  11,  21,  23,  16,   7,  -9,
            -27, -11,   4,  13,  14,   4,  -5, -17,
            -53, -34, -21, -11, -28, -14, -24, -43,
        }
    }
};

// mirror square values
extern const int mirrorScore[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

// rank mask bitboard
U64 rankMask[64];

// file mask bitboard
U64 fileMask[64];

// filled by initEvalMasks after fileMask and rankMask are built
// (initialising here at static-init time would read fileMask and rankMask
//  before they are populated, giving 0)
U64 centerFiles = 0ULL;
U64 outpostRanksWhite = 0ULL;
U64 outpostRanksBlack = 0ULL;
U64 kingFlankMask[8] = {0ULL};
U64 campMask[2] = {0ULL};
U64 betweenMask[64][64] = {{0ULL}};

//  isolated pawn mask bitboard
U64 isolatedMask[64];

// passed white pawn mask
U64 whitePassedMask[64];

// passed pawn mask
U64 blackPassedMask[64];

// white opposed pawn mask
U64 whiteOpposedMask[64];

// black opposed pawn mask
U64 blackOpposedMask[64];

// support mask for white pawns
U64 whiteSupportMask[64];

// support mask for black pawns
U64 blackSupportMask[64];

// phalanx mask for each square (pawns next to each other on the same rank)
U64 phalanxMask[64];

// white king zone masks (king moves and 3 squares in front towards enemy king)
U64 whiteKingZoneMask[64];

// black king zone masks (king moves and 3 squares in front towards enemy king)
U64 blackKingZoneMask[64];

// white blocked pawn mask
U64 whiteBlockedMask[64];

// black blocked pawn mask
U64 blackBlockedMask[64];

// masks for pins in [direction][square]
U64 pinnedMasks[8][64];

// masks for forward ranks [side][square] (if [black][d3], all sqs on rank 1 and 2 will return )
U64 forwardRanksMasks[2][64];

// adjacent files mask for each file
U64 adjacentFilesMask[8];

// get rank of square
extern const int getRank[64] =
{
    7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0
};

// get file of square
extern const int getFile[64] =
{
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7
};

// game phase scores
extern const int openingPhaseScore = 6192; // if game stage score > 6192, in pure opening
extern const int endgamePhaseScore = 750; // if game stage score < 518, in pure endgame

// keeping track of all attacks by certain pieces [side][piece]
U64 pieceAttackTables[2][7];

// keeping track of double attacks by pawns [side]
U64 pawnDoubleTables[2];

// squares attacked by 2 pieces of the colour
U64 attackedBy2[2];

// squares that are weak (not strongly protected or under attack)
U64 weak[2];

// safe squares
U64 safe[2];

// strongly protected squares
U64 stronglyProtected[2];

// strongly protected non-pawn pieces
U64 defended[2];

// non pawn enemies
U64 nonPawnEnemies[2];

// current and potential pawn attacks
U64 pawnSpans[2];

U64 mobilityAreaWhite = 0;
U64 mobilityAreaBlack = 0;

// initialise piece bitboards (6 black pieces, 6 white pieces)
U64 bitboards[12];

// initialise occupancy bitboards (white occupancies, black occupancies, all occupancies)
U64 occupancies[3];

// current side (side to move)
int side;

// enpassant square
int enpassant = noSq;

// castling rights
int castle;

// (almost) unique position identifier (hash key / position id)
U64 hashKey;

// repetition table
U64 repetitionTable[1000]; // 1000 -> number of plies, assuming maximum 500 moves game

// repetition index (starts at 0)
int repetitionIndex = 0;

// half move counter (ply)
int ply = 0;

// fifty move rule
int fifty = 0;

// move number
int moveNumber = 0;

PositionCache positionCache;

// parse FEN
void parseFEN(const char *fen)
{
    // reset board position
    memset(bitboards, 0ULL, sizeof(bitboards));
    memset(occupancies, 0ULL, sizeof(occupancies));

    // reset variables
    side = 0;
    enpassant = noSq;
    castle = 0;

    // reset repetition index
    repetitionIndex = 0;

    // reset fifty move
    fifty = 0;

    // reset move number
    moveNumber = 0;

    // reset rep table
    memset(repetitionTable, 0ULL, sizeof(repetitionTable));

    // loop over all squares via ranks and files
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            // get current square
            int square = rank * 8 + file;

            // check if character is an uppercase or lowercase letter
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
            {
                // get piece type as an int to use as the index of the bitboards
                int piece = charPieces[*fen];

                // set piece on the correct bitboard
                setBit(bitboards[piece], square);

                // increment pointer (to go through all)
                fen++;
            }

            // empty square numbers
            if (*fen >= '0' && *fen <= '9')
            {
                // create offset (convert character to int)
                int offset = *fen - '0';

                int piece = -1;

                // loop over piece bitboards
                for (int pieceI = P; pieceI <= k; pieceI++)
                {
                    // if there is a piece on square
                    if (getBit(bitboards[pieceI], square))
                    {
                        // change piece to piece index
                        piece = pieceI;
                    }
                }

                // if no piece, fix the file
                if (piece == -1)
                {
                    file--;
                }

                // fix file
                file += offset;

                // increment pointer (to go through all)
                fen++;
            }

            // change ranks when '/'
            if (*fen == '/')
            {
                fen++;
            }
        }
    }
    // move pointer to account for space
    fen++;

    // get what side to move
    (*fen == 'w') ? (side = white) :(side = black);

    // move pointer twice (to go to castling)
    fen += 2;

    // get castling rights
    while (*fen != ' ') // while still in castling rights
    {
        switch (*fen)
        {
            case 'K': castle |= wk; break;
            case 'Q': castle |= wq; break;
            case 'k': castle |= bk; break;
            case 'q': castle |= bq; break;
            case '-': break;
        }
        // increment pointer
        fen++;
    }

    // got to enpassant square
    fen++;

    // get enpassant square
    if (*fen != '-')
    {
        // get file and rank of enpassant
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');

        // update enpassant
        enpassant = rank * 8 + file;

        // Skip past the en passant square (file and rank)
        fen += 2;
    }
    else // no enpassant
    {
        enpassant = noSq;
        // Skip past the '-'
        fen++;
    }

    // Skip to the fifty move counter field
    while (*fen && *fen == ' ') fen++;

    // get fifty move counter
    if (*fen != '-') {
        fifty = atoi(fen);

        // Skip past all digits of the fifty move counter
        while (*fen && *fen >= '0' && *fen <= '9') fen++;
    } else {
        fifty = 0;
        fen++; // Skip the '-'
    }

    // Skip spaces to get to the move number field
    while (*fen && *fen == ' ') fen++;

    // Now parse the move number if present
    if (*fen && *fen >= '0' && *fen <= '9') {
        moveNumber = atoi(fen);
    } else {
        moveNumber = 1; // Default if not specified
    }

    // get new occupancies
    // loop over white pieces
    for (int piece = P; piece <= K; piece++)
    {
        occupancies[white] |= bitboards[piece];
    }
    // loop over black pieces
    for (int piece = p; piece <= k; piece++)
        {
            occupancies[black] |= bitboards[piece];
        }

    // get combined occupancies
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];

    // get hash key
    hashKey = generateHashKey();

    initAttacksTotal();
}

// print board function
void printBoard()
{
    printf("\n");
    // loop over all squares (via rank and files)
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            // get current square value
            int square = rank * 8 + file;

            // print labels
            if (!file)
            {
                printf("  %d  ", 8 - rank);
            }

            // create piece variable
            int piece = -1;

            // loop over all piece bitboards to see which piece is on square
            for (int pieceIndex = P; pieceIndex <= k; pieceIndex++)
            {
                if (getBit(bitboards[pieceIndex], square))
                {
                    piece = pieceIndex;
                }
            }

            printf(" %c", (piece == -1) ? '.' : ASCIIpieces[piece]);
        }
        // print line
        printf("\n");
    }
    // print the rest of the labels
    printf("\n      a b c d e f g h\n");

    // print info
    printf("     Side:      %s\n", (!side) ? "white" : "black");
    printf("     Enpass:    %s\n", (enpassant != noSq) ? squareNames[enpassant] : "none");
    printf("     Castling:  %c%c%c%c\n", (castle & wk) ? 'K' : '-',
                                        (castle & wq) ? 'Q' : '-',
                                        (castle & bk) ? 'k' : '-',
                                        (castle & bq) ? 'q' : '-');
    // print hash key for position
    printf("     Hash key: %llx\n\n", hashKey);

}
