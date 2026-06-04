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

// piece-square tables [stage][piece][square]
extern const int PieceTables[2][6][64] = {
    {
        { // MidPawnTable
            0,    0,    0,    0,    0,    0,    0,    0,
           -7,    6,   -2,  -11,    4,  -14,   10,   -9,
            3,  -11,   -6,   22,   -8,   -5,  -14,  -11,
           11,   -4,  -11,    2,   11,    0,  -12,    5,
           -3,  -20,    8,   19,   39,   17,    2,   -5,
           -9,  -15,   11,   15,   31,   23,    6,  -20,
            2,    4,   11,   18,   16,   21,    9,   -3,
            0,    0,    0,    0,    0,    0,    0,    0,
        },
        { // MidKnightTable
         -201,  -83,  -56,  -26,  -26,  -56,  -83, -201,
          -67,  -27,    4,   37,   37,    4,  -27,  -67,
           -9,   22,   58,   53,   53,   58,   22,   -9,
          -34,   13,   44,   51,   51,   44,   13,  -34,
          -35,    8,   40,   49,   49,   40,    8,  -35,
          -61,  -17,    6,   12,   12,    6,  -17,  -61,
          -77,  -41,  -27,  -15,  -15,  -27,  -41,  -77,
         -175,  -92,  -74,  -73,  -73,  -74,  -92, -175,
        },
        { // MidBishopTable
          -34,    1,  -10,  -16,  -16,  -10,    1,  -34,
          -12,  -10,    4,    0,    0,    4,  -10,  -12,
          -11,    4,    1,    8,    8,    1,    4,  -11,
           -8,   20,   15,   22,   22,   15,   20,   -8,
           -4,    8,   18,   27,   27,   18,    8,   -4,
           -5,   15,   -4,   12,   12,   -4,   15,   -5,
          -11,    6,   13,    3,    3,   13,    6,  -11,
          -37,   -4,   -6,  -16,  -16,   -6,   -4,  -37,
        },
        { // MidRookTable
          -17,  -19,   -1,    9,    9,   -1,  -19,  -17,
           -2,   12,   16,   18,   18,   16,   12,   -2,
          -22,   -2,    6,   12,   12,    6,   -2,  -22,
          -27,  -15,   -4,    3,    3,   -4,  -15,  -27,
          -13,   -5,   -4,   -6,   -6,   -4,   -5,  -13,
          -25,  -11,   -1,    3,    3,   -1,  -11,  -25,
          -21,  -13,   -8,    6,    6,   -8,  -13,  -21,
          -31,  -20,  -14,   -5,   -5,  -14,  -20,  -31,
        },
        { // MidQueenTable
           -2,   -2,    1,   -2,   -2,    1,   -2,   -2,
           -5,    6,   10,    8,    8,   10,    6,   -5,
           -4,   10,    6,    8,    8,    6,   10,   -4,
            0,   14,   12,    5,    5,   12,   14,    0,
            4,    5,    9,    8,    8,    9,    5,    4,
           -3,    6,   13,    7,    7,   13,    6,   -3,
           -3,    5,    8,   12,   12,    8,    5,   -3,
            3,   -5,   -5,    4,    4,   -5,   -5,    3,
        },
        { // MidKingTable
           59,   89,   45,   -1,   -1,   45,   89,   59,
           88,  120,   65,   33,   33,   65,  120,   88,
          123,  145,   81,   31,   31,   81,  145,  123,
          154,  179,  105,   70,   70,  105,  179,  154,
          164,  190,  138,   98,   98,  138,  190,  164,
          195,  258,  169,  120,  120,  169,  258,  195,
          278,  303,  234,  179,  179,  234,  303,  278,
          271,  327,  271,  198,  198,  271,  327,  271,
        },
    },
    {
        { // EndPawnTable
            0,    0,    0,    0,    0,    0,    0,    0,
           -1,  -14,   13,   22,   24,   17,    7,    7,
           27,   18,   19,   29,   30,    9,    8,   14,
           12,    6,    2,   -6,   -5,   -4,   14,    9,
            7,    1,   -8,   -2,  -14,  -13,  -11,   -6,
           -9,   -7,  -10,    5,    2,    3,   -8,   -5,
           -8,   -6,    9,    5,   16,    6,   -6,  -18,
            0,    0,    0,    0,    0,    0,    0,    0,
        },
        { // EndKnightTable
         -100,  -88,  -56,  -17,  -17,  -56,  -88, -100,
          -69,  -50,  -51,   12,   12,  -51,  -50,  -69,
          -51,  -44,  -16,   17,   17,  -16,  -44,  -51,
          -45,  -16,    9,   39,   39,    9,  -16,  -45,
          -35,   -2,   13,   28,   28,   13,   -2,  -35,
          -40,  -27,   -8,   29,   29,   -8,  -27,  -40,
          -67,  -54,  -18,    8,    8,  -18,  -54,  -67,
          -96,  -65,  -49,  -21,  -21,  -49,  -65,  -96,
        },
        { // EndBishopTable
          -32,  -29,  -26,  -17,  -17,  -26,  -29,  -32,
          -22,  -14,   -1,    1,    1,   -1,  -14,  -22,
          -21,    4,    3,    4,    4,    3,    4,  -21,
          -12,   -1,  -10,   11,   11,  -10,   -1,  -12,
          -14,   -4,    0,   12,   12,    0,   -4,  -14,
          -11,   -1,   -1,    7,    7,   -1,   -1,  -11,
          -26,   -9,  -12,    1,    1,  -12,   -9,  -26,
          -40,  -21,  -26,   -8,   -8,  -26,  -21,  -40,
        },
        { // EndRookTable
           18,    0,   19,   13,   13,   19,    0,   18,
            4,    5,   20,   -5,   -5,   20,    5,    4,
            6,    1,   -7,   10,   10,   -7,    1,    6,
           -5,    8,    7,   -6,   -6,    7,    8,   -5,
           -6,    1,   -9,    7,    7,   -9,    1,   -6,
            6,   -8,   -2,   -6,   -6,   -2,   -8,    6,
          -12,   -9,   -1,   -2,   -2,   -1,   -9,  -12,
           -9,  -13,  -10,   -9,   -9,  -10,  -13,   -9,
        },
        { // EndQueenTable
          -74,  -52,  -43,  -34,  -34,  -43,  -52,  -74,
          -50,  -27,  -24,   -8,   -8,  -24,  -27,  -50,
          -38,  -18,  -11,    1,    1,  -11,  -18,  -38,
          -29,   -6,    9,   21,   21,    9,   -6,  -29,
          -23,   -3,   13,   24,   24,   13,   -3,  -23,
          -39,  -18,   -9,    3,    3,   -9,  -18,  -39,
          -54,  -31,  -22,   -4,   -4,  -22,  -31,  -54,
          -69,  -57,  -47,  -26,  -26,  -47,  -57,  -69,
        },
        { // EndKingTable
           11,   59,   73,   78,   78,   73,   59,   11,
           47,  121,  116,  131,  131,  116,  121,   47,
           92,  172,  184,  191,  191,  184,  172,   92,
           96,  166,  199,  199,  199,  199,  166,   96,
          103,  156,  172,  172,  172,  172,  156,  103,
           88,  130,  169,  175,  175,  169,  130,   88,
           53,  100,  133,  135,  135,  133,  100,   53,
            1,   45,   85,   76,   76,   85,   45,    1,
        },
    },
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
// game phase thresholds, scaled to old-Stockfish material values (per-side full ~8302)
extern const int openingPhaseScore = 15196; // game stage score above this is pure opening
extern const int endgamePhaseScore = 1841;  // game stage score below this is pure endgame

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
