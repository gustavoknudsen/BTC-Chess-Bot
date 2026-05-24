/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                BETTERTHANCRIS

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/



/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Initial Setup

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// headers
#include <stdio.h>
#include <string.h>
#include <sysinfoapi.h>
#include <unistd.h>
#include <windows.h>
#include <cmath> 
#include <stdint.h>

// create the bitboard data type (64Bit unsigned integers)
#define U64 unsigned long long

// FEN dedug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

// notation of squares (a8 is assinged 0, b8 is assigned 1, etc.)
enum 
{
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, noSq
};

static uint8_t PopCnt16[1 << 16];

// castling bits
/*
   0001    white king can castle to the king side
   0010    white king can castle to the queen side
   0100    black king can castle to the king side
   1000    black king can castle to the queen side
*/
enum 
{
    wk = 1, wq = 2, bk = 4, bq = 8
};

// pieces (caps for white, lower for black)
enum 
{
    P, N, B, R, Q, K, p, n, b, r, q, k
};

enum
{
    allPieces = 6
};

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

// colours (white=0, black=1, both=2)
enum
{
    white, black, both
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
const int castlingRights[64] = 
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
const int PieceTables[2][6][64] = {
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
const int mirrorScore[128] =
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
U64 rankMask[64];

// file mask bitboard
U64 fileMask[64];

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

// get rank of square
const int getRank[64] =
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
const int getFile[64] =
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

// highest possible value
#define infinity 50000
// mate value "higher bound" of score for a mate
#define mateValue 49000
// mate score "lower bound" of score for a mate
#define mateScore 48000

// game phase scores
const int openingPhaseScore = 6192; // if game stage score > 6192, in pure opening
const int endgamePhaseScore = 518; // if game stage score < 518, in pure endgame

// game phases
enum { 
    opening, 
    endgame, 
    middlegame   
};

// directions (currently no need for NESW)
enum {
    NORTH,
    NORTHEAST,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST,
    NORTHWEST,
};

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

//  struct to store mobility for a single side
typedef struct {
    int knight;
    int bishop;
    int rook;
    int queen;
} Mobility;

// mobility default to 0
Mobility mobilityW = {0, 0, 0, 0}; // White: knight, bishop, rook, queen
Mobility mobilityB = {0, 0, 0, 0}; // Black: knight, bishop, rook, queen

// structure for mobility info
typedef struct {
    U64 excludeMaskWhite = 0;
    U64 pinnedPiecesWhite = 0;
    U64 excludeMaskBlack = 0;
    U64 pinnedPiecesBlack = 0;
} MobilityInfo;


/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Standard Board
                  Definition
https://www.chessprogramming.org/Bitboard_Board-Definition

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

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

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

            Time Control Variables

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// exit from engine flag
int quit = 0;

// UCI "movestogo" command moves counter
int movestogo = 30;

// UCI "movetime" command time counter
int movetime = -1;

// UCI "time" command holder (ms)
int time = -1;

// UCI "inc" command's time increment holder
int inc = 0;

// UCI "starttime" command time holder
int starttime = 0;

// UCI "stoptime" command time holder
int stoptime = 0;

// variable to flag time control availability
int timeset = 0;

// variable to flag when the time is up
int stopped = 0;

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

           Misc Functions Stolen From
       Richard Allbert aka BluefeverSoftware
            (Mostly for UCI Protocol
                + time control)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// get time (use int start = getTime() --> and then (getTime - start))
int getTime()
{
    #ifdef WIN64
        return GetTickCount();
    #else
        struct timeval time_value;
        gettimeofday(&time_value, NULL);
        return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
    #endif
}

// leaf nodes (number of positions in test)
U64 nodes = 0;
int counter = 0;
  
int input_waiting()
{
    #ifndef WIN32
        fd_set readfds;
        struct timeval tv;
        FD_ZERO (&readfds);
        FD_SET (fileno(stdin), &readfds);
        tv.tv_sec=0; tv.tv_usec=0;
        select(16, &readfds, 0, 0, &tv);

        return (FD_ISSET(fileno(stdin), &readfds));
    #else
        static int init = 0, pipe;
        static HANDLE inh;
        DWORD dw;

        if (!init)
        {
            init = 1;
            inh = GetStdHandle(STD_INPUT_HANDLE);
            pipe = !GetConsoleMode(inh, &dw);
            if (!pipe)
            {
                SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
                FlushConsoleInputBuffer(inh);
            }
        }
        
        if (pipe)
        {
           if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
           return dw;
        }
        
        else
        {
           GetNumberOfConsoleInputEvents(inh, &dw);
           return dw <= 1 ? 0 : dw;
        }

    #endif
}

// read GUI/user input
void read_input()
{
    // bytes to read holder
    int bytes;
    
    // GUI/user input
    char input[256] = "", *endc;

    // "listen" to STDIN
    if (input_waiting())
    {
        // tell engine to stop calculating
        stopped = 1; 
        
        // loop to read bytes from STDIN
        do
        {
            // read bytes from STDIN
            bytes=read(fileno(stdin), input, 256);
        }
        
        // until bytes available
        while (bytes < 0);
        
        // searches for the first occurrence of '\n'
        endc = strchr(input,'\n');
        
        // if found new line set value at pointer to 0
        if (endc) *endc=0;
        
        // if input is available
        if (strlen(input) > 0)
        {
            // match UCI "quit" command
            if (!strncmp(input, "quit", 4))
            {
                // tell engine to terminate exacution    
                quit = 1;
            }

            // // match UCI "stop" command
            else if (!strncmp(input, "stop", 4))    {
                // tell engine to terminate exacution
                quit = 1;
            }
        }   
    }
}

// a bridge function to interact between search and GUI input
static void communicate() {
	// if time is up break here
    if(timeset == 1 && getTime() > stoptime) {
		// tell engine to stop calculating
		stopped = 1; 
	}
	
    // read GUI input
	read_input();
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Random Functions

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// initiliase pseudorandom number state
unsigned int randomState = 1804289383;

// Xorshift algorithm (exclusive or shift)
// https://en.wikipedia.org/wiki/Xorshift
// generate 32 bit legal numbers
unsigned int xorshift32()
{
    // get state
    unsigned int number = randomState;

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // update state
    randomState = number;

    // return state
    return number;
}

// (Tord Romstad's method https://www.chessprogramming.org/Looking_for_Magics)
// generate 64 bit legal numbers 
U64 random64()
{
    // initialise 4 random numbers
    U64 n1, n2, n3, n4;
    n1 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB
    n2 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB
    n3 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB
    n4 = (U64)(xorshift32()) & 0xFFFF; // slice 16 bits from MSFB

    // return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

// generate possible magic number
U64 genMagicNumber()
{
    return (random64() & random64() & random64());
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Bitboard Macros
                   and Manip

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// set macro (sets bit to 1 in the square)
#define setBit(bitboard, square) ((bitboard) |= (1ULL << (square)))

// get macro (gets 1 or 0 from bitboard)
#define getBit(bitboard, square) ((bitboard) & (1ULL << (square) ))

// pop (remove) macro (unsets bit)
#define popBit(bitboard, square) \
    do { \
        if (getBit((bitboard), (square))) { \
            (bitboard) ^= (1ULL << (square)); \
        } \
    } while (0) // do { ... } while (0) common idiom in macros to make them behave like a single statement

// function that counts bits in bitboard (static inline int for performance)
static inline int countBits(U64 bitboard)
{
    // create counter
    int count = 0;

    // loop while bitboard > 0
    while (bitboard)
    {
        // increase counter
        count++;
        // remove a bit from bitboard
        bitboard &= bitboard - 1;
    }

    // return count
    return count;
}

// Function to get the index of the most significant set bit (MSB)
static inline int getLSFBIndex(U64 bitboard)
{
    if (bitboard)
    {
        unsigned long idx;
        _BitScanForward64(&idx, bitboard);
        return idx;
    }
    else
    {
        // no bits
        return -1;
    }
}

// Function to get the index of the most significant set bit (MSB)
static inline int getMSBIndex(U64 bitboard)
{
    if (bitboard)
    {
        unsigned long idx;
        _BitScanReverse64(&idx, bitboard);
        return idx;
    }
    else
    {
        // no bits
        return -1;
    }
}

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

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Zobrist Hashing
https://www.chessprogramming.org/Zobrist_Hashing
                   
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// random piece keys (for hashing) [piece][square]
U64 pieceKeys[12][64];

// random enpassant keys (for hashing) [square]
U64 enpassantKeys[64];

// random castling keys (for hashing) (max 16 as 1111 is maximum in binary)
U64 castleKeys[16];

// random side key
U64 sideKey;

// initialise all random keys
void initRandomKeys()
{
    // re-update pseudorandom number state (so it is always the same)
    unsigned int randomState = 1804289383;
    
    // loop over all the pieces
    for (int piece = P; piece <= k; piece++)
    {
        // loop over all squares
        for (int square = 0; square < 64; square++)
        {
            // initialise piece key by giving a random U64 value
            pieceKeys[piece][square] = random64();
        }
    }

    // loop over board squares
    for (int square = 0; square < 64; square++)
    {
        // initialise enpassant key by giving a random U64 value
        enpassantKeys[square] = random64();
    }

    // loop over all possible castling keys
    for (int index = 0; index < 16; index++)
    {
        // initialise castle keys
        castleKeys[index] = random64();
    }

    // initialise sideKey
    sideKey = random64();

}

// generate (almost) unique hash key
U64 generateHashKey()
{
    // create final key variable and start at 0
    U64 finalKey = 0ULL;

    // piece bitboard copy (like in the makeMove function)
    U64 bitboard;

    // loop over piece bitboards (to add all pieces to hash key)
    for (int piece = P; piece <= k; piece++)
    {
        // get piece bitboard copy
        bitboard = bitboards[piece];

        // loop over all pieces in bitboard
        while (bitboard)
        {
            // get square value
            int square = getLSFBIndex(bitboard);

            // hash all the pieces
            finalKey ^= pieceKeys[piece][square];

            // pop current piece/bit
            popBit(bitboard, square);
        }
    }

    // if enpassant
    if (enpassant != noSq)
    {
        // hash the enpassant square
        finalKey ^= enpassantKeys[enpassant];
    }

    // add castling rights to hash
    finalKey ^= castleKeys[castle];

    // hash the side if black to move
    if (side == black) finalKey ^= sideKey;

    // return the hash key
    return finalKey;
}

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
#define encodeMove(source, target, piece, promoted, capture, double, enpassant, castling) \
(source) | (target << 6) | (piece << 12) | (promoted << 16) | \
(capture << 20) | (double << 21) | (enpassant << 22) | (castling << 23)

// get move source square
#define getSource(move) (move & 0x3F)

// get move target square
#define getTarget(move) ((move & 0xFC0) >> 6)

// get move piece
#define getPiece(move) ((move & 0xF000) >> 12)

// get promoted piece
#define getPromoted(move) ((move & 0xF0000) >> 16)

// get capture flag
#define getCapture(move) ((move & 0x100000) >> 20)

// get double push flag
#define getDouble(move) ((move & 0x200000) >> 21)

// get enpassant flag
#define getEnpassant(move) ((move & 0x400000) >> 22)

// get castling flag
#define getCastle(move) ((move & 0x800000) >> 23)

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Bitboard Commands
                        +
                FEN String Manip

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

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

void initAttacksTotal();

// parse FEN
void parseFEN(const char *fen)
{
    // reset board position
    memset(bitboards, 0ULL, sizeof(bitboards));
    memset(occupancies, 0ULL, sizeof(occupancies));
    mobilityW = {0, 0, 0, 0}; // White: knight, bishop, rook, queen
    mobilityB = {0, 0, 0, 0}; // Black: knight, bishop, rook, queen
    
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

    }
    else // no enpassant
    {
        enpassant = noSq;
    }

    // go to fifty move counter
    fen++;

    // get fifty move counter
    fifty = atoi(fen);

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

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Attack Tables

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

/*
    BitBoard Constants Visualisation

       Not A File              Not H File
  8  0 1 1 1 1 1 1 1       8  1 1 1 1 1 1 1 0
  7  0 1 1 1 1 1 1 1       7  1 1 1 1 1 1 1 0
  6  0 1 1 1 1 1 1 1       6  1 1 1 1 1 1 1 0
  5  0 1 1 1 1 1 1 1       5  1 1 1 1 1 1 1 0
  4  0 1 1 1 1 1 1 1       4  1 1 1 1 1 1 1 0
  3  0 1 1 1 1 1 1 1       3  1 1 1 1 1 1 1 0
  2  0 1 1 1 1 1 1 1       2  1 1 1 1 1 1 1 0
  1  0 1 1 1 1 1 1 1       1  1 1 1 1 1 1 1 0

     a b c d e f g h          a b c d e f g h

       Not AB File             Not GH File
  8  0 0 1 1 1 1 1 1       8  1 1 1 1 1 1 0 0
  7  0 0 1 1 1 1 1 1       7  1 1 1 1 1 1 0 0
  6  0 0 1 1 1 1 1 1       6  1 1 1 1 1 1 0 0
  5  0 0 1 1 1 1 1 1       5  1 1 1 1 1 1 0 0
  4  0 0 1 1 1 1 1 1       4  1 1 1 1 1 1 0 0
  3  0 0 1 1 1 1 1 1       3  1 1 1 1 1 1 0 0
  2  0 0 1 1 1 1 1 1       2  1 1 1 1 1 1 0 0
  1  0 0 1 1 1 1 1 1       1  1 1 1 1 1 1 0 0

     a b c d e f g h          a b c d e f g h


*/

// not A file global variable
const U64 notAFile = 18374403900871474942ULL;

// not H file constant
const U64 notHFile = 9187201950435737471ULL;

// not AB file constant
const U64 notABFile = 18229723555195321596ULL;

// not HG file constant
const U64 notGHFile = 4557430888798830399ULL;

// bishops occupancy bit count lookup table
const int bishopBits[64] = 
{
    6, 5, 5, 5, 5, 5, 5, 6, 
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
};

// rooks occupancy bit count lookup table
const int rookBits[64] =
{
    12, 11, 11, 11, 11, 11, 11, 12, 
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
};

// rook magic numbers
U64 rookMagicNumbers[64] = 
{
0x8a80104000800020ULL,
0x140002000100040ULL,
0x2801880a0017001ULL,
0x100081001000420ULL,
0x200020010080420ULL,
0x3001c0002010008ULL,
0x8480008002000100ULL,
0x2080088004402900ULL,
0x800098204000ULL,
0x2024401000200040ULL,
0x100802000801000ULL,
0x120800800801000ULL,
0x208808088000400ULL,
0x2802200800400ULL,
0x2200800100020080ULL,
0x801000060821100ULL,
0x80044006422000ULL,
0x100808020004000ULL,
0x12108a0010204200ULL,
0x140848010000802ULL,
0x481828014002800ULL,
0x8094004002004100ULL,
0x4010040010010802ULL,
0x20008806104ULL,
0x100400080208000ULL,
0x2040002120081000ULL,
0x21200680100081ULL,
0x20100080080080ULL,
0x2000a00200410ULL,
0x20080800400ULL,
0x80088400100102ULL,
0x80004600042881ULL,
0x4040008040800020ULL,
0x440003000200801ULL,
0x4200011004500ULL,
0x188020010100100ULL,
0x14800401802800ULL,
0x2080040080800200ULL,
0x124080204001001ULL,
0x200046502000484ULL,
0x480400080088020ULL,
0x1000422010034000ULL,
0x30200100110040ULL,
0x100021010009ULL,
0x2002080100110004ULL,
0x202008004008002ULL,
0x20020004010100ULL,
0x2048440040820001ULL,
0x101002200408200ULL,
0x40802000401080ULL,
0x4008142004410100ULL,
0x2060820c0120200ULL,
0x1001004080100ULL,
0x20c020080040080ULL,
0x2935610830022400ULL,
0x44440041009200ULL,
0x280001040802101ULL,
0x2100190040002085ULL,
0x80c0084100102001ULL,
0x4024081001000421ULL,
0x20030a0244872ULL,
0x12001008414402ULL,
0x2006104900a0804ULL,
0x1004081002402ULL
};

// bishop magic numbers
U64 bishopMagicNumbers[64] =
{
0x40040844404084ULL,
0x2004208a004208ULL,
0x10190041080202ULL,
0x108060845042010ULL,
0x581104180800210ULL,
0x2112080446200010ULL,
0x1080820820060210ULL,
0x3c0808410220200ULL,
0x4050404440404ULL,
0x21001420088ULL,
0x24d0080801082102ULL,
0x1020a0a020400ULL,
0x40308200402ULL,
0x4011002100800ULL,
0x401484104104005ULL,
0x801010402020200ULL,
0x400210c3880100ULL,
0x404022024108200ULL,
0x810018200204102ULL,
0x4002801a02003ULL,
0x85040820080400ULL,
0x810102c808880400ULL,
0xe900410884800ULL,
0x8002020480840102ULL,
0x220200865090201ULL,
0x2010100a02021202ULL,
0x152048408022401ULL,
0x20080002081110ULL,
0x4001001021004000ULL,
0x800040400a011002ULL,
0xe4004081011002ULL,
0x1c004001012080ULL,
0x8004200962a00220ULL,
0x8422100208500202ULL,
0x2000402200300c08ULL,
0x8646020080080080ULL,
0x80020a0200100808ULL,
0x2010004880111000ULL,
0x623000a080011400ULL,
0x42008c0340209202ULL,
0x209188240001000ULL,
0x400408a884001800ULL,
0x110400a6080400ULL,
0x1840060a44020800ULL,
0x90080104000041ULL,
0x201011000808101ULL,
0x1a2208080504f080ULL,
0x8012020600211212ULL,
0x500861011240000ULL,
0x180806108200800ULL,
0x4000020e01040044ULL,
0x300000261044000aULL,
0x802241102020002ULL,
0x20906061210001ULL,
0x5a84841004010310ULL,
0x4010801011c04ULL,
0xa010109502200ULL,
0x4a02012000ULL,
0x500201010098b028ULL,
0x8040002811040900ULL,
0x28000010020204ULL,
0x6000020202d0240ULL,
0x8918844842082200ULL,
0x4010011029020020ULL
};

// bishop attack masks array
U64 bishopMasks[64];

// rook attack masks array
U64 rookMasks[64];

// bishop attacks table ([square][occupancies/block]) 512 possible occupancies for bishops
U64 bishopAttacks[64][512];

// rook attacks table ([square][occupancies/block])
U64 rookAttacks[64][4096];

// create pawn table (two dimensional array of [side to move][square])
U64 pawnAttacks[2][64];

// create knight attack table (one dimensional array of [square])
U64 knightAttacks[64];

// create king attack table (one dimensional array of [square])
U64 kingAttacks[64];

// get pawn attacks (mask)
U64 maskPawnAttacks(int side, int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create the piece bitboard
    U64 pieceBitboard = 0ULL;

    // set the pieces on the board
    setBit(pieceBitboard, square);

    // for white
    if (side == 0)
    {
        // if pawn not in H file, then valid attack to the north east
        if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard >> 7;
        // if pawn not in A file, then valid attack to the north west
        if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard >> 9;
    }
    else // for black
    {
        // if pawn not in A file, then valid attack to the south east
        if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard << 7;
        // if pawn not in H file, then valid attack to the south west
        if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard << 9;
    }
    // return the attacks
    return attacksBitboard;
}

// get knight attacks (mask)
U64 maskKnightAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create the piece bitboard
    U64 pieceBitboard = 0ULL;

    // set the pieces on the board
    setBit(pieceBitboard, square);

    // if knight not on A file, North North West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= (pieceBitboard >> 17);
    // if knight not on A or B file, North West West is valid
    if (pieceBitboard & notABFile) attacksBitboard |= (pieceBitboard >> 10);
    // if knight not on H file, North North East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= (pieceBitboard >> 15);
    // if knight not on G or H file, North East East is valid
    if (pieceBitboard & notGHFile) attacksBitboard |= (pieceBitboard >> 6);

    // if knight not on A file, South South West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= (pieceBitboard << 15);
    // if knight not on A or B file, South West West is valid
    if (pieceBitboard & notABFile) attacksBitboard |= (pieceBitboard << 6);
    // if knight not on H file, South South East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= (pieceBitboard << 17);
    // if knight not on H file, South East East is valid
    if (pieceBitboard & notGHFile) attacksBitboard |= (pieceBitboard << 10);

    // return the attacks
    return attacksBitboard;
}

// get king attacks (mask)
U64 maskKingAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create the piece bitboard
    U64 pieceBitboard = 0ULL;

    // set the pieces on the board
    setBit(pieceBitboard, square);

    // if result of pieceBitboard >> 8 is non-zero, movement North is valid (check is acutally not needed)
    if (pieceBitboard >> 8) attacksBitboard |= (pieceBitboard >> 8);
    // if king not on H file, attack to the North East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard >> 7;
    // if king not on A file, attack to the North West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard >> 9;
    // if king not on A file, attack to the West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard >> 1;
    // if king not on H file, attack to the East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard << 1;
    // if king not on H file, attack to the South East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard << 9;
    // if king not on A file, attack to the South West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard << 7;
    // if result of pieceBitboard >> 8 is non-zero, movement South is valid (check is acutally not needed)
    if (pieceBitboard << 8) attacksBitboard |= (pieceBitboard << 8);

    // return the attacks
    return attacksBitboard;

}

// get bishop attacks (mask)
U64 maskBishopAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    /*    
    mask relevant bishop occupancy bits (for magic bitboard)
    e.g. for e4:
     8    0  0  0  0  0  0  0  0
     7    0  0  0  0  0  0  1  0
     6    0  1  0  0  0  1  0  0
     5    0  0  1  0  1  0  0  0
     4    0  0  0  0  0  0  0  0
     3    0  0  1  0  1  0  0  0
     2    0  1  0  0  0  1  0  0
     1    0  0  0  0  0  0  0  0

          a  b  c  d  e  f  g  h
    */

    // mask South East rays (r+, f+)
    for (ranks = targetRank + 1, files = targetFile + 1; ranks <= 6 && files <= 6; ranks++, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }
    // mask North East rays (r-, f+)
    for (ranks = targetRank - 1, files = targetFile + 1; ranks >= 1 && files <= 6; ranks--, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }    
    // mask South West rays (r+, f-)
    for (ranks = targetRank + 1, files = targetFile - 1; ranks <= 6 && files >=1; ranks++, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }    
    // mask North West  rays (r-, f-)
    for (ranks = targetRank - 1, files = targetFile - 1; ranks >= 1 && files >=1; ranks--, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }    

    // return the attacks
    return attacksBitboard;
}

// get rook attacks (mask)
U64 maskRookAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    /*    
    mask relevant rook occupancy bits (for magic bitboard)
    e.g. for e4:
     8    0  0  0  0  0  0  0  0
     7    0  0  0  0  1  0  0  0
     6    0  0  0  0  1  0  0  0
     5    0  0  0  0  1  0  0  0 
     4    0  1  1  1  0  1  1  0
     3    0  0  0  0  1  0  0  0
     2    0  0  0  0  1  0  0  0
     1    0  0  0  0  0  0  0  0

          a  b  c  d  e  f  g  h
    */

    // mask South rays
    for (ranks = targetRank + 1; ranks <= 6; ranks++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
    }
    // mask North rays
    for (ranks = targetRank - 1; ranks >= 1; ranks--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
    }
    // mask West rays
    for (files = targetFile - 1; files >= 1; files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
    }
    // mask East rays
    for (files = targetFile + 1; files <= 6; files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
    }

    // return the attacks
    return attacksBitboard;
}

// get bishop attacks on the fly (in the case of a piece 'blocking')
U64 bishopAttacksOTF(int square, U64 block)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    // generate ALL bishop attacks
    // mask South East rays (r+, f+)
    for (ranks = targetRank + 1, files = targetFile + 1; ranks <= 7 && files <= 7; ranks++, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;
    }
    // mask North East rays (r-, f+)
    for (ranks = targetRank - 1, files = targetFile + 1; ranks >= 0 && files <= 7; ranks--, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;
    }    
    // mask South West rays (r+, f-)
    for (ranks = targetRank + 1, files = targetFile - 1; ranks <= 7 && files >= 0; ranks++, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;

    }    
    // mask North West  rays (r-, f-)
    for (ranks = targetRank - 1, files = targetFile - 1; ranks >= 0 && files >= 0; ranks--, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;

    }    

    // return the attacks
    return attacksBitboard;
}

// get bishop attacks on the fly (in the case of a piece 'blocking')
U64 rookAttacksOTF(int square, U64 block)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    // mask South rays
    for (ranks = targetRank + 1; ranks <= 7; ranks++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + targetFile)) & block) break;

    }
    // mask North rays
    for (ranks = targetRank - 1; ranks >= 0; ranks--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + targetFile)) & block) break;
    }
    // mask West rays
    for (files = targetFile - 1; files >= 0; files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (targetRank * 8 + files)) & block) break;
    }
    // mask East rays
    for (files = targetFile + 1; files <= 7; files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (targetRank * 8 + files)) & block) break;
    }

    // return the attacks
    return attacksBitboard;
}

// initialise leapers attacks (pawns, knights, kings)
void initLeapersAttacks()
{
    // for every square get leapers attacks and store in array (or two-dimensional array)
    for (int square = 0; square < 64; square ++)
    {
        // initialise white pawn attacks
        pawnAttacks[white][square] = maskPawnAttacks(white, square);
        // initialise black pawn attacks
        pawnAttacks[black][square] = maskPawnAttacks(black, square);

        // initialise knight attacks
        knightAttacks[square] = maskKnightAttacks(square);

        // initialise king attacks
        kingAttacks[square] = maskKingAttacks(square);
    }
}

// function that sets occupancies
U64 setOccupancy(int index, int bitsInMask, U64 attackMask)
{
    // initialise occupancy map
    U64 occupancyMap = 0ULL;

    // loop over bits in attack mask
    for (int count = 0; count < bitsInMask; count++)
    {
        // get LSFB of mask and remove (pop) it
        int square = getLSFBIndex(attackMask);
        popBit(attackMask, square);

        // if the 'count'-th bit in 'index' is set to 1, 
        // set the corresponding bit in occupancyMap at position square
        if (index & (1 << count))
        {
            occupancyMap |= (1ULL << square);
        }
    }

    // return map
    return occupancyMap;
}

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

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

            Initialising Sliders
        Lasse Hansen's initial approach
https://www.chessprogramming.org/Magic_Bitboards

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// initialise slider piece attack tables (bishop flag)
void initSlidersAttacks(int bishop)
{
    // loop through squares
    for (int square = 0; square < 64; square++)
    {
        // get bishop and rook masks
        bishopMasks[square] = maskBishopAttacks(square);
        rookMasks[square] = maskRookAttacks(square);

        // get current mask (bishop or rook)
        U64 attackMask = bishop ? bishopMasks[square] : rookMasks[square];

        // initialise relevant bitcount for bishop or rook
        int bitCount = countBits(attackMask);

        // get occupancy index
        int occupancyI = (1 << bitCount);

        // loop over all occupancies indices
        for (int index = 0; index < occupancyI; index++)
        {
            // for bishops
            if (bishop)
            {
                // get occupancy
                U64 occupancy = setOccupancy(index, bitCount, attackMask);

                // get magic index
                int magicIndex = (occupancy * bishopMagicNumbers[square]) >> (64 - bishopBits[square]);

                // get bishop attacks
                bishopAttacks[square][magicIndex] = bishopAttacksOTF(square, occupancy);
            }
            // for rooks
            else
            {
                // get occupancy
                U64 occupancy = setOccupancy(index, bitCount, attackMask);

                // get magic index
                int magicIndex = (occupancy * rookMagicNumbers[square]) >> (64 - rookBits[square]);

                // get bishop attacks
                rookAttacks[square][magicIndex] = rookAttacksOTF(square, occupancy);
            }
        }
    }
}

// get bishop attacks (static inline as it is used in the move gen)
static inline U64 getBishopAttacks(int square, U64 occupancy)
{
    // get the bishop valid attacks
    occupancy &= bishopMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopBits[square];

    // return
    return bishopAttacks[square][occupancy];
}

// get rook attacks (static inline as it is used in the move gen)
static inline U64 getRookAttacks(int square, U64 occupancy)
{
    // get the rook valid attacks
    occupancy &= rookMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookBits[square];

    // return
    return rookAttacks[square][occupancy];

}

// get queen attacks (static inline as it is used in the move gen)
static inline U64 getQueenAttacks(int square, U64 occupancy)
{
    U64 final = 0ULL;
    
    // get bishop occupancies
    U64 bishopOccupancies = occupancy;

    // get rook occupancies
    U64 rookOccupancies = occupancy;

    // get the bishop valid attacks
    bishopOccupancies &= bishopMasks[square];
    bishopOccupancies *= bishopMagicNumbers[square];
    bishopOccupancies >>= 64 - bishopBits[square];

    // get bishop attacks
    final = bishopAttacks[square][bishopOccupancies];

    // get the rook valid attacks
    rookOccupancies &= rookMasks[square];
    rookOccupancies *= rookMagicNumbers[square];
    rookOccupancies >>= 64 - rookBits[square];

    // get bishop attacks
    final |= rookAttacks[square][rookOccupancies];

    // combine and return
    return final;
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

               Copy Make Functions
                    & Macros

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

#define copyBoard()                                                     \
U64 bitboardsCopy[12], occupanciesCopy[3], pieceAttackTablesCopy[2][7], pawnDoubleTablesCopy[2];           \
U64 weakCopy[2], stronglyProtectedCopy[2], safeCopy[2], defendedCopy[2], attackedBy2Copy[2];                                           \
int sideCopy, enpassantCopy, castleCopy, fiftyCopy, moveNumberCopy;     \
Mobility mobilityWCopy, mobilityBCopy;                             \
memcpy(bitboardsCopy, bitboards, 96);                                   \
memcpy(occupanciesCopy, occupancies, 24);                               \
memcpy(pieceAttackTablesCopy, pieceAttackTables, 112);                               \
memcpy(pawnDoubleTablesCopy, pawnDoubleTables, 16);                               \
memcpy(weakCopy, weak, 16);                               \
memcpy(stronglyProtectedCopy, stronglyProtected, 16);                               \
memcpy(safeCopy, safe, 16);                               \
memcpy(defendedCopy, defended, 16);                               \
memcpy(attackedBy2Copy, attackedBy2, 16);                               \
memcpy(&mobilityWCopy, &mobilityW, sizeof(Mobility));  \
memcpy(&mobilityBCopy, &mobilityB, sizeof(Mobility));  \
sideCopy = side, enpassantCopy = enpassant, castleCopy = castle;        \
fiftyCopy = fifty;                                                      \
moveNumberCopy = moveNumber;                                             \
U64 hashKeyCopy = hashKey;

#define undoBoard()                                                      \
memcpy(bitboards, bitboardsCopy, 96);                                    \
memcpy(occupancies, occupanciesCopy, 24);                                \
memcpy(pieceAttackTables, pieceAttackTablesCopy, 112);                                \
memcpy(pawnDoubleTables, pawnDoubleTablesCopy, 16);                                \
memcpy(weak, weakCopy, 16);                                \
memcpy(stronglyProtected, stronglyProtectedCopy, 16);                                \
memcpy(safe, safeCopy, 16);                                \
memcpy(defended, defendedCopy, 16);                                \
memcpy(attackedBy2, attackedBy2Copy, 16);                                \
memcpy(&mobilityW, &mobilityWCopy, sizeof(Mobility));  \
memcpy(&mobilityB, &mobilityBCopy, sizeof(Mobility));  \
side = sideCopy; enpassant = enpassantCopy, castle = castleCopy;         \
fifty = fiftyCopy;                                                       \
moveNumber = moveNumberCopy;                                             \
hashKey = hashKeyCopy;

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Move Generation
                   
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

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

// is square under attack function
static inline int isUnderAttack(int square, int side)
{
    // if attacked by white pawn
    if ((side == white) && (pawnAttacks[black][square] & bitboards[P])) return 1;
    // if attacked by black pawn
    if ((side == black) && (pawnAttacks[white][square] & bitboards[p])) return 1;

    // if attacked by knight
    if (knightAttacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;
    
    // if attacked by king
    if (kingAttacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

    // if attacked by bishop
    if (getBishopAttacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

    // if attacked by rook
    if (getRookAttacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

    // if attacked by queen
    if (getQueenAttacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

    // if hasn't returned yet, no attacks on square
    return 0;
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

// setup for mobility
static inline MobilityInfo setupMobility()
{
    U64 excludeMaskWhite = 0;
    U64 pinnedPiecesWhite = 0;
    U64 excludeMaskBlack = 0;
    U64 pinnedPiecesBlack = 0;

    // for white
    // get mask of pawns in rank 2 or 3, and king square
    excludeMaskWhite = ((rankMask[a2] & bitboards[P]) | (rankMask[a3] & bitboards[P]) | bitboards[K]);

    // get bitboard of pawns
    U64 bitboard = bitboards[P];

    // loop over all white pawns in bitboard
    while (bitboard)
    {
        // get square value
        int square = getLSFBIndex(bitboard);

        // add blocked pawns
        if (whiteBlockedMask[square] & occupancies[both])
        {
            // set bit of blocked pawns
            setBit(excludeMaskWhite, square);
        }

        // pop current piece/bit
        popBit(bitboard, square);
    }

    // get bitboard of enemy pawns
    bitboard = bitboards[p];

    // loop over all enemy pawns in bitboard 
    while (bitboard)
    {
        // get square value
        int square = getLSFBIndex(bitboard);

        // add enemy pawn attacks
        excludeMaskWhite |= pawnAttacks[!white][square];
        
        // pop current piece/bit
        popBit(bitboard, square);
    }
    
    // loop over all white pieces to check for pins
    for (int pieceBB = P; pieceBB <= Q; pieceBB++)
    {
        // set bitboard
        bitboard = bitboards[pieceBB];

        // loop over friendly pieces
        while (bitboard)
        {
            // get square value
            int square = getLSFBIndex(bitboard);
            
            // pop current piece/bit temporarily and occupancy bit
            popBit(bitboards[pieceBB], square);
            popBit(occupancies[white], square);
            popBit(occupancies[both], square);

            int inCheck = isUnderAttack(getLSFBIndex(bitboards[K]),
                                                    black);

            // if now in check
            if (inCheck)
            {
                // piece is pinned
                setBit(pinnedPiecesWhite, square);
            }

            // return piece
            setBit(bitboards[pieceBB], square);
            setBit(occupancies[white], square);
            setBit(occupancies[both], square);

            popBit(bitboard, square);
        }
    }

    // for black
    // get mask of pawns in rank 7 or 6, and king square
    excludeMaskBlack = ((rankMask[a7] & bitboards[p]) | (rankMask[a6] & bitboards[p]) | bitboards[k]);

    // get bitboard of pawns
    bitboard = bitboards[p];

    // loop over all black pawns in bitboard
    while (bitboard)
    {
        // get square value
        int square = getLSFBIndex(bitboard);

        // add blocked pawns
        if (blackBlockedMask[square] & occupancies[both])
        {
            // set bit of blocked pawns
            setBit(excludeMaskBlack, square);
        }

        // pop current piece/bit
        popBit(bitboard, square);
    }
    
    // get bitboard of enemy pawns
    bitboard = bitboards[P];

    // loop over all enemy pawns in bitboard
    while (bitboard)
    {
        // get square value
        int square = getLSFBIndex(bitboard);

        // add enemy pawn attacks
        excludeMaskBlack |= pawnAttacks[!black][square];
        
        // pop current piece/bit
        popBit(bitboard, square);
    }

    // loop over all black pieces to check for pins
    for (int pieceBB = p; pieceBB <= q; pieceBB++)
    {
        // set bitboard
        bitboard = bitboards[pieceBB];

        // loop over friendly pieces
        while (bitboard)
        {
            // get square value
            int square = getLSFBIndex(bitboard);
            
            // pop current piece/bit temporarily and occupancy bit
            popBit(bitboards[pieceBB], square);
            popBit(occupancies[black], square);
            popBit(occupancies[both], square);

            int inCheck = isUnderAttack(getLSFBIndex(bitboards[k]), white);

            // if now in check
            if (inCheck)
            {
                // piece is pinned
                setBit(pinnedPiecesBlack, square);
            }

            // return piece
            setBit(bitboards[pieceBB], square);
            setBit(occupancies[black], square);
            setBit(occupancies[both], square);

            popBit(bitboard, square);
        }
    }

    MobilityInfo info;
    info.excludeMaskBlack = excludeMaskBlack;
    info.excludeMaskWhite = excludeMaskWhite;
    info.pinnedPiecesBlack = pinnedPiecesBlack;
    info.pinnedPiecesWhite = pinnedPiecesWhite;

    // return info (bitboards)
    return info;
}

// get mobility for pinned piece
static inline int getPinnedMobility(int sq, int currentSide, U64 mobilityBB)
{
    int mobility = 0;
    
    // if on the same rank, only check mobility on that rank
    if (getRank[sq] == getRank[getLSFBIndex(bitboards[(currentSide == white) ? K : k])])
    {
        // count mobility in that rank only
        mobility = countBits(rankMask[sq] & mobilityBB);
        //printf("pinned same rank\n");
        //printBitBoard(rankMask[squarePiece] & mobilityBB);
    }
    // if on the same file, only check mobility on that file
    else if (getFile[sq] == getFile[getLSFBIndex(bitboards[(currentSide == white) ? K : k])])
    {
        // count mobility in that rank only
        mobility = countBits(fileMask[sq] & mobilityBB);
        //printf("pinned same file\n");
        //printBitBoard(fileMask[squarePiece] & mobilityBB);
    }
    // if north east of king or south west of king
    else if ((bitboards[(currentSide == white) ? K : k] & pinnedMasks[NORTHEAST][sq]) | (bitboards[(currentSide == white) ? K : k] & pinnedMasks[SOUTHWEST][sq]))
    {
        // count mobility in that rank only
        mobility = countBits((pinnedMasks[NORTHEAST][sq] | pinnedMasks[SOUTHWEST][sq]) & mobilityBB);
        //printf("pinned north east of king or south west of king\n");
        //printBitBoard((pinnedMasks[NORTHEAST][squarePiece] | pinnedMasks[SOUTHWEST][squarePiece]) & mobilityBB);
    }
    else // north west of king or south east of king
    {
        // count mobility in that rank only
        mobility = countBits((pinnedMasks[NORTHWEST][sq] | pinnedMasks[SOUTHEAST][sq]) & mobilityBB);
        //printf("pinned north west of king or south east of king\n");
        //printBitBoard((pinnedMasks[NORTHWEST][squarePiece] | pinnedMasks[SOUTHEAST][squarePiece]) & mobilityBB);
    }

    return mobility;
}


// get mobility of side
static inline void getMobility(int currentSide)
{
    MobilityInfo info = setupMobility();
    U64 excludeMask = (currentSide == white) ? info.excludeMaskWhite : info.excludeMaskBlack;
    U64 pinnedPieces = (currentSide == white) ? info.pinnedPiecesWhite : info.pinnedPiecesBlack;
    Mobility mobility = {0, 0, 0, 0};
    
    U64 attacks = 0;

    /* for knight mobility, only exclusions matter */
    U64 bb = bitboards[(currentSide == white) ? N : n];
    while (bb)
    {
        // get square
        int sq = getLSFBIndex(bb);
        
        /*
        // count bits that it can move to excluding the mask
        if (getBit(pinnedPieces, sq))
        {
            continue;
        }
        else
        {
        */
        
        mobility.knight += countBits(knightAttacks[sq] & ~excludeMask);
        
        popBit(bb, sq);
    }
    

    /* for bishop mobility, can look through friendly queen */
    bb = bitboards[(currentSide == white) ? B : b];
    while (bb)
    {
        // get square
        int sq = getLSFBIndex(bb);
        
        // exclude friendly queen square
        // get friendly queen bitboard
        U64 friendlyQueen = bitboards[(currentSide == white) ? Q : q];

        /*
        if (getBit(pinnedPieces, sq))
        {
            //mobility.bishop += getPinnedMobility(sq, currentSide, (getBishopAttacks(sq, occupancies[both] & ~friendlyQueen) & ~excludeMask));
            continue;
        }
        else
        {
        */
        // calculate mobility, considering bishop can look through friendly queen
        mobility.bishop += countBits(getBishopAttacks(sq, occupancies[both] & ~friendlyQueen) & ~excludeMask);
        
    
        popBit(bb, sq);
    }

    /* for rook, can look through friendly queen and rook */
    bb = bitboards[(currentSide == white) ? R : r];
    while (bb)
    {
        // get square
        int sq = getLSFBIndex(bb);

        // exclude friendly queen squares and rook squares
        // get friendly queen + rooks bitboard
        U64 friendlyQueenRook = bitboards[(currentSide == white) ? Q : q] | bitboards[(currentSide == white) ? R : r];

        /*
        if (getBit(pinnedPieces, sq))
        {
            //mobility.rook += getPinnedMobility(sq, currentSide, (getRookAttacks(sq, occupancies[both] & ~friendlyQueenRook) & ~excludeMask));
            continue;
        }
        else
        {
        */
            
        // calculate mobility, considering bishop can look through friendly queen
        mobility.rook += countBits(getRookAttacks(sq, occupancies[both] & ~friendlyQueenRook) & ~excludeMask);
        
    
        popBit(bb, sq);
    }

    /* for queen, CANNOT look through rook or bishop, also excludes attacks from enemy b, n, r */
    bb = bitboards[(currentSide == white) ? Q : q];
    while (bb)
    {
        // get square
        int sq = getLSFBIndex(bb);
        
        // make knight and rook variables for each side (of the enemy piece)
        int knight = (currentSide == white) ? n : N;
        int rook = (currentSide == white) ? r : R;

        // loop over enemy bishops, knights and rooks
        for (int pieceBB = knight; pieceBB <= rook; pieceBB++)
        {
            // get bitboard
            U64 bitboard = bitboards[pieceBB];
            
            while (bitboard)
            {
                // if knight
                if (pieceBB == n || pieceBB == N)
                {
                    // get square value
                    int square = getLSFBIndex(bitboard);
                    
                    // add enemy knight
                    excludeMask |= knightAttacks[square];
                    
                    // pop current piece/bit
                    popBit(bitboard, square);
                }
                else if (pieceBB == b || pieceBB == B)
                {
                    // get square value
                    int square = getLSFBIndex(bitboard);
                    
                    // add enemy bishop attacks
                    excludeMask |= getBishopAttacks(square, occupancies[both]);
                    
                    // pop current piece/bit
                    popBit(bitboard, square);
                }
                else if (pieceBB == r || pieceBB == R)
                {
                    // get square value
                    int square = getLSFBIndex(bitboard);
                    
                    // add enemy bishop attacks
                    excludeMask |= getRookAttacks(square, occupancies[both]);
                    
                    // pop current piece/bit
                    popBit(bitboard, square);
                }
            }


        }

        /*
        if (getBit(pinnedPieces, sq))
        {
            //mobility.queen += getPinnedMobility(sq, currentSide, (getQueenAttacks(sq, occupancies[both]) & ~excludeMask));
            continue;
        }
        else
        {
        */
        // set queen mobility
        mobility.queen += countBits(getQueenAttacks(sq, occupancies[both]) & ~excludeMask);
        

        popBit(bb, sq);
    }

   if (currentSide == white)
   {
    mobilityW = mobility;
   }
   else
   {
    mobilityB = mobility;
   }
}

// generate all moves (not all legal)
static inline void generateMoves(moves *moveList)
{
    // create move count 
    moveList->count = 0;

    // creating move's source square and target square
    int sourceSquare; int targetSquare;

    // create a copy of piece's bitboard and attacks
    U64 bitboard; U64 attacks;

    // loop over all bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // assign correct bitboard
        bitboard = bitboards[piece];

        // generate white pawns and white king castling
        if (side == white)
        {
            // if white pawn
            if (piece == P)
            {
                // loop over all white pawns
                while (bitboard) // loop until bitboard has no more bits (no more white pawns)
                {
                    // update source and target squares
                    sourceSquare = getLSFBIndex(bitboard);
                    targetSquare = sourceSquare - 8; // white pawn one square forward

                    // get all quiet pawn moves (no captures)
                    if (!(targetSquare < a8) && !getBit(occupancies[both], targetSquare)) // target square exists and no other piece in front
                    {
                        // pawn promotion
                        if (sourceSquare >= a7 && sourceSquare <= h7)
                        {
                            // add move into a move list
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 0, 0, 0, 0));
                        }
                        else
                        {
                            // add single pawn push to move list
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));

                            // add double pawn push to move list
                            if ((sourceSquare >= a2 && sourceSquare <= h2) && !getBit(occupancies[both], targetSquare - 8))
                            {
                                addMove(moveList, encodeMove(sourceSquare, targetSquare - 8, piece, 0, 0, 1, 0, 0));
                            }
                        }
                    }
                    // assign value to pawn attacks bitboard
                    attacks = pawnAttacks[side][sourceSquare] & occupancies[black]; // where a black piece is and pawn attacks
                    // pawn captures
                    while (attacks) // loop over all attack bits
                    {
                        // update target square  
                        targetSquare = getLSFBIndex(attacks);

                        // if pawn promotion as well
                        if (sourceSquare >= a7 && sourceSquare <= h7)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 1, 0, 0, 0));
                        }
                        else // not a promotion
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                        }

                        // pop current pawn attack
                        popBit(attacks, targetSquare);
                    }

                    // get enpassant captures
                    if (enpassant != noSq)
                    {
                        // create bitboard with valid enpassant squares
                        U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);

                        // if enpassant is possible
                        if (enpassantAttacks)
                        {
                            // create enpassant target square
                            int targetEnpassant = getLSFBIndex(enpassantAttacks);
                            addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
                        }

                    }

                    // pop the current white pawn
                    popBit(bitboard, sourceSquare);
                }

            }
            // check castling moves
            if (piece == K)
            {
                // king side castling
                if (castle & wk)
                {
                    // empty squares between king and right rook
                    if (!getBit(occupancies[both], f1) && !getBit(occupancies[both], g1))
                    {
                        // make sure king and adjacent square is not under attack
                        if (!isUnderAttack(e1, black) && !isUnderAttack(f1, black))
                        {
                            addMove(moveList, encodeMove(e1, g1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                // queen side castling
                if (castle & wq)
                {
                    // empty squares between king and left rook
                    if (!getBit(occupancies[both], d1) && !getBit(occupancies[both], c1) && !getBit(occupancies[both], b1))
                    {
                        // make sure king and adjacent square is not under attack (and also b1)
                        if (!isUnderAttack(e1, black) && !isUnderAttack(d1, black))
                        {
                            addMove(moveList, encodeMove(e1, c1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }
        // generate black pawns and black king castling
        else
        {
            // if black pawn
            if (piece == p)
            {
                // loop over all black pawns
                while (bitboard) // loop until bitboard has no more bits (no more black pawns)
                {
                    // update source and target squares
                    sourceSquare = getLSFBIndex(bitboard);
                    targetSquare = sourceSquare + 8; // black pawn one square forward

                    // get all quiet pawn moves (no captures)
                    if (!(targetSquare > h1) && !getBit(occupancies[both], targetSquare)) // target square exists and no other piece in front
                    {
                        // pawn promotion
                        if (sourceSquare >= a2 && sourceSquare <= h2)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 0, 0, 0, 0));
                        }
                        else
                        {
                            // add single pawn push to move list
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));

                            // add double pawn push to move list
                            if ((sourceSquare >= a7 && sourceSquare <= h7) && !getBit(occupancies[both], targetSquare + 8))
                            {
                                addMove(moveList, encodeMove(sourceSquare, targetSquare + 8, piece, 0, 0, 1, 0, 0));
                            }
                        }
                    }
                    // assign value to pawn attacks bitboard
                    attacks = pawnAttacks[side][sourceSquare] & occupancies[white]; // where a black piece is and pawn attacks
                    // pawn captures
                    while (attacks) // loop over all attack bits
                    {
                        // update target square  
                        targetSquare = getLSFBIndex(attacks);

                        // if pawn promotion as well
                        if (sourceSquare >= a2 && sourceSquare <= h2)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 1, 0, 0, 0));
                        }
                        else // not a promotion
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                        }

                        // pop current pawn attack
                        popBit(attacks, targetSquare);
                    }

                    // get enpassant captures
                    if (enpassant != noSq)
                    {
                        // create bitboard with valid enpassant squares
                        U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);

                        // if enpassant is possible
                        if (enpassantAttacks)
                        {
                            // create enpassant target square
                            int targetEnpassant = getLSFBIndex(enpassantAttacks);
                            addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop the current white pawn
                    popBit(bitboard, sourceSquare);
                }
            }
            // check castling moves
            if (piece == k)
            {
                // king side castling
                if (castle & bk)
                {
                    // empty squares between king and right rook
                    if (!getBit(occupancies[both], f8) && !getBit(occupancies[both], g8))
                    {
                        // make sure king and adjacent square is not under attack
                        if (!isUnderAttack(e8, white) && !isUnderAttack(f8, white))
                        {
                            addMove(moveList, encodeMove(e8, g8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                // queen side castling
                if (castle & bq)
                {
                    // empty squares between king and left rook
                    if (!getBit(occupancies[both], d8) && !getBit(occupancies[both], c8) && !getBit(occupancies[both], b8))
                    {
                        // make sure king and adjacent square is not under attack (and also b1)
                        if (!isUnderAttack(e8, white) && !isUnderAttack(d8, white))
                        {
                            addMove(moveList, encodeMove(e8, c8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }
        // generate knight moves
        if ((side == white) ? piece == N : piece == n)
        {
            // loop over all knights on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = knightAttacks[sourceSquare] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate bishop moves
        if ((side == white) ? piece == B : piece == b)
        {
            // loop over all bishops on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = getBishopAttacks(sourceSquare, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate rook moves
        if ((side == white) ? piece == R : piece == r)
        {
            // loop over all rooks on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = getRookAttacks(sourceSquare, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate queen moves
        if ((side == white) ? piece == Q : piece == q)
        {
            // loop over all queens on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = getQueenAttacks(sourceSquare, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate king moves
        if ((side == white) ? piece == K : piece == k)
        {
            // loop over all queens on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = kingAttacks[sourceSquare] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }
    }

    // update mobility
    getMobility(white);
    getMobility(black);

}

// move type key (all moves = 0, captures = 1)
enum
{
    allMoves, capturesOnly
};

void initAttacksTotal();

// make move function
static inline int makeMove(int move, int moveType)
{
    // non-captures
    if (moveType == allMoves)
    {
        // copy board
        copyBoard(); 

        // get move values
        int sourceSq = getSource(move);
        int targetSq = getTarget(move);
        int piece = getPiece(move);
        int promoted = getPromoted(move);
        int capture = getCapture(move);
        int doublePush = getDouble(move);
        int enpass = getEnpassant(move);
        int castling = getCastle(move);

        // move the piece
        popBit(bitboards[piece], sourceSq);
        setBit(bitboards[piece], targetSq);

        // hash piece (remove from source and update new square)
        hashKey ^= pieceKeys[piece][sourceSq]; // remove from source square
        hashKey ^= pieceKeys[piece][targetSq]; // add piece to new square
        
        // increment fifty move
        fifty++;
        
        // increment move number
        moveNumber++;

        // if pawn move or capture
        if ((piece == P || piece == p) || capture)
        {
            // reset fifty move
            fifty = 0;
        }

        // hash enpassant remove enpassant from hash
        if (enpassant != noSq) 
        {
            hashKey ^= enpassantKeys[enpassant];
        }

        // update enpassant square after any move
        enpassant = noSq;

        // get capture
        if (capture)
        {
            // get range of piece bitboards depending on side
            int startPiece, endPiece;

            // if white to move, get indices for black pieces
            if (side == white)
            {
                startPiece = p;
                endPiece = k;
            }
            else // for black get white piece range
            {
                startPiece = P;
                endPiece = K;
            }

            // loop over enemy's bitboard and remove the bit on the target square if it exists
            for (int bitPiece = startPiece; bitPiece <= endPiece; bitPiece++)
            {
                if (getBit(bitboards[bitPiece], targetSq))
                {
                    // pop bit if it exists on target square
                    popBit(bitboards[bitPiece], targetSq);

                    // remove taken piece from hash
                    hashKey ^= pieceKeys[bitPiece][targetSq];

                    // break out of loop as piece has been found
                    break;
                }
            }
            
        }

        // if promotion
        if (promoted)
        {
            // white to move
            if (side == white)
            {
                // erase pawn from target square in bitboard
                popBit(bitboards[P], targetSq);

                // remove pawn from hash key
                hashKey ^= pieceKeys[P][targetSq];
            }
            // black
            else
            {
                // erase pawn from target square in bitboard
                popBit(bitboards[p], targetSq);

                // remove pawn from hash key
                hashKey ^= pieceKeys[p][targetSq];
            }

            // place the promoted piece on the board
            setBit(bitboards[promoted], targetSq);

            // add promoted piece to hash
            hashKey ^= pieceKeys[promoted][targetSq];
        }

        // enpassant
        if (enpass)
        {
            // erase the taken pawn + remove hash key
            if (side == white) {
                // erase pawn from bitboard
                popBit(bitboards[p], targetSq + 8);

                // remove pawn from hash key
                hashKey ^= pieceKeys[p][targetSq + 8];
            } 
            else 
            {
                // erase pawn from bitboard
                popBit(bitboards[P], targetSq - 8);

                // remove pawn from hash key
                hashKey ^= pieceKeys[P][targetSq - 8];
            }
        }

        // if double pawn push
        if (doublePush)
        {
            // white to move
            if (side == white)
            {
                // set enpassant square
                enpassant = targetSq + 8;

                // hash enpassant square
                hashKey ^= enpassantKeys[targetSq + 8];
            }
            // black to move
            else
            {
                // set enpassant square
                enpassant = targetSq - 8;

                // hash enpassant square
                hashKey ^= enpassantKeys[targetSq - 8];
            }
        }

        // if castle
        if (castling)
        {
            // move rook depending on colour and side
            switch (targetSq)
            {
                // white castles king
                case (g1):
                    // move rook on h1
                    popBit(bitboards[R], h1);
                    setBit(bitboards[R], f1);

                    // hash new rook position
                    hashKey ^= pieceKeys[R][h1];
                    hashKey ^= pieceKeys[R][f1];

                    break;
                // white castles queen
                case (c1):
                    popBit(bitboards[R], a1);
                    setBit(bitboards[R], d1);

                    // hash new rook position
                    hashKey ^= pieceKeys[R][a1];
                    hashKey ^= pieceKeys[R][d1];

                    break;

                // black castles king
                case (g8):
                    popBit(bitboards[r], h8);
                    setBit(bitboards[r], f8);

                    // hash new rook position
                    hashKey ^= pieceKeys[r][h8];
                    hashKey ^= pieceKeys[r][f8];

                    break;

                // black castles queen
                case (c8):
                    popBit(bitboards[r], a8);
                    setBit(bitboards[r], d8);

                    hashKey ^= pieceKeys[r][a8];
                    hashKey ^= pieceKeys[r][d8];

                    break;
            }
        }

        // hash castling (remove castling rights from hash key)
        hashKey ^= castleKeys[castle];
        
        // update castling rights
        castle &= castlingRights[sourceSq]; // in case king/rook was moved
        castle &= castlingRights[targetSq]; // in case rook was captured

        // hash castling back after update
        hashKey ^= castleKeys[castle];

        // update the occupancies
        memset(occupancies, 0ULL, 24); // resets the occupancies (all 0)
        
        // loop over white piece bitboards
        for (int bitPiece = P; bitPiece <= K; bitPiece++)
        {
            // update white occupancies
            occupancies[white] |= bitboards[bitPiece];
        }
        // loop over black piece bitboards
        for (int bitPiece = p; bitPiece <= k; bitPiece++)
        {
            // update black occupancies
            occupancies[black] |= bitboards[bitPiece];
        }
        // update combined occupancies
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        // change the side after move is made
        side ^= 1;

        // hash side variable
        hashKey ^= sideKey;

        initAttacksTotal();
        
        // ===== debug hash key incremental update ==== //
        /*
        U64 hashFromScratch = generateHashKey();

        // in case hash key does not match the incrementally updated one
        // interrupt exe
        if (hashKey != hashFromScratch)
        {
            printf("\nmake move\n");
            printf("move: "); printMove(move);
            printBoard();
            printf("hash key should be %llx\n", hashFromScratch);
            getchar();
        }        
        */


        // make sure king has not been exposed to a check (not a valid move)
        if (isUnderAttack((side == white) ? getLSFBIndex(bitboards[k]) : getLSFBIndex(bitboards[K]), side))
        {
            // illegal move
            undoBoard();
            
            // return 0 
            return 0;
        }
        else
        {
            // move is made
            return 1;
        }
    }


    // captures only
    else
    {
        // check if capture
        if (getCapture(move))
        {
            // make the move without the check
            return makeMove(move, allMoves);
        }
        else
        {
            // no move is made
            return 0;
        }

    }
    // so all paths have a return, even though none should get here
    //return makeMove(move, allMoves);
}

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

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                   Evaluation
                   
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

/* Evaluation Constants (except piece tables) */

// default material scores
int defaultMaterialScore[12] = {
    100,      // white pawn score
    300,      // white knight scrore
    320,      // white bishop score
    500,      // white rook score
   900,      // white queen score
  10000,      // white king score
   -100,      // black pawn score
   -300,      // black knight scrore
   -320,      // black bishop score
   -500,      // black rook score
  -900,      // black queen score
 -10000,      // black king score
};


// material scores [stage][piece]
int materialScore[2][12] = {
{
    82,      // white pawn score
    337,      // white knight scrore
    365,      // white bishop score
    477,      // white rook score
   1025,      // white queen score
  10000,      // white king score
   -82,      // black pawn score
   -337,      // black knight scrore
   -365,      // black bishop score
   -477,      // black rook score
  -1025,      // black queen score
 -10000,      // black king score
},
{
    94,      // white pawn score
    281,      // white knight scrore
    297,      // white bishop score
    512,      // white rook score
   936,      // white queen score
  10000,      // white king score
   -94,      // black pawn score
   -281,      // black knight scrore
   -297,      // black bishop score
   -512,      // black rook score
  -936,      // black queen score
 -10000,      // black king score
}
};

// material adjustment based on pawn number
int knightAdj[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 12 };
int rookAdj[9] = { 15,  12,   9,  6,  3,  0, -3, -6, -9 };

// double pawns penalty (values from old Stockfish, subject to change {mg, eg})
const int doublePawnPenalty[2] = {-11, -56};

// isolated pawn penalty (values from old Stockfish, subject to change {mg, eg})
const int isolatedPawnPenalty[2] = {-5, -15};

// backward pawn penalty (values from old Stockfish, subject to change {mg, eg})
const int backwardPawnPenalty[2] = {-9, -24};

// passed pawn rank bonus [stage][rank] (values from old Stockfish, subject to change)
const int passedPawnRankBonus[2][8] =
{
    {
        0, 0, 5, 12, 10, 57, 163, 271
    },
    {
        0, 0, 18, 23, 31, 62, 167, 250
    }
};

// passed pawn passed file bonus [stage][file]
const int passedPawnFileBonus[2][8] =
{
    {
        -1, 0, -9, -30, -30, -9, 0, -1
    },
    {
        7, 9, -8, -14, -14, -8, 9, 7
    }
};

// connected pawn [rank]
const int connectedPawnBonus[8] = { 0, 7, 8, 12, 29, 48, 86, 100};

// weak and unopposed isolated pawn
const int weakUnopposed[2] = {-13, -27};

// attacked2unsupported

// semi open file score (values from old Stockfish, subject to change {mg, eg})
const int semiOpenFileScore[2] = {18, 7};

// open file score (values from old Stockfish, subject to change {mg, eg})
const int openFileScore[2] = {44, 20};

// mobility bonus (values from old Stockfish, subject to change {mg, eg})
// [piece][# of attacked squares != friendly pieces][stage]
const int mobilityBonus[6][32][2] = 
{
    // pawns
    {
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}
    },
    // knights
    {
        {-38, -33}, {-25, -23}, {-12, -13}, {0, -3}, {12, 7}, {25, 17}, 
        {31, 22}, {38, 27}, {38, 27}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    }, 
    // bishops
    {
        {-25, -30}, {-11, -16}, {3, -2}, {17, 12}, {31, 26}, {45, 40}, 
        {57, 52}, {65, 60}, {71, 65}, {74, 69}, {76, 71}, {78, 73}, 
        {79, 74}, {80, 75}, {81, 76}, {81, 76}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}
    }, 
    // rooks
    {
        {-20, -36}, {-14, -19}, {-8, -3}, {-2, 13}, {4, 29}, {10, 46}, 
        {14, 62}, {19, 79}, {23, 95}, {26, 106}, {27, 111}, {28, 114}, 
        {29, 116}, {30, 117}, {31, 118}, {32, 118}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}
    }, 
    // queens
    {
        {-10, -18}, {-8, -13}, {-6, -7}, {-3, -2}, {-1, 3}, {1, 8}, 
        {3, 13}, {5, 19}, {8, 23}, {10, 27}, {12, 32}, {15, 34}, 
        {16, 35}, {17, 35}, {18, 35}, {20, 35}, {20, 35}, {20, 35}, 
        {20, 35}, {20, 35}, {20, 35}, {20, 35}, {20, 35}, {20, 35}, 
        {20, 35}, {20, 35}, {20, 35}, {20, 35}, {20, 35}, {20, 35}, 
        {20, 35}, {20, 35}
    },
    // king
    {
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
        {0, 0}, {0, 0}
    }
};

// attacking king zone attack weight table [piece number]
const int attackWeight[8] = {0, 0, 50, 75, 88, 94, 97, 99};

const int KingAttackValues[6] = { 0, 0, 77, 55, 44, 10 };

// tempo bonus
const int tempoBonus = 28;

// shelter strength bonuses
int ShelterStrength[4][7] = {
    { -6, 81, 93, 58, 39, 18, 25 },
    { -43, 61, 35, -49, -29, -11, -63 },
    { -10, 75, 23, -2, 32, 3, -45 },
    { -39, -13, -29, -52, -48, -67, -166 }
};

// unblocked storm bonuses
int UnblockedStorm[4][7] = {
    { 89, -285, -185, 93, 57, 45, 51 },
    { 44, -18, 123, 46, 39, -7, 23 },
    { 4, 52, 162, 37, 7, -14, -2 },
    { -10, -14, 90, 15, 2, -7, -16 }
};

// create attack info struct
struct AttackInfo {
    int numberAttackers;
    int valueAttacks; 
    int numberAttacks;
};

// set file or rank mask
U64 setFileOrRankMask(int file, int rank)
{
    // get the file or rank mask
    U64 mask = 0ULL;

    // loop over all squares via ranks and files
    for (int rankNumber = 0; rankNumber < 8; rankNumber ++)
    {
        for (int fileNumber = 0; fileNumber < 8; fileNumber ++)
        {
            // get square
            int square = rankNumber * 8 + fileNumber;

            // if there is a file
            if (file != -1)
            {
                // if file is the inputed file
                if (fileNumber == file)
                {
                    // set the bit on mask
                    mask |= setBit(mask, square);
                }
            }
            // if there is a rank
            else if (rank != -1)
            {
                // if file is the inputed file
                if (rankNumber == rank)
                {
                    // set the bit on mask
                    mask |= setBit(mask, square);
                }
            }
        }
    }

    // return the mask
    return mask;
}

// initialise evaluation masks
void initEvalMasks()
{
    // get all file + rank masks + isolated pawn masks + passed pawn masks
    
    // loop over all squares via ranks and files
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // create file mask for square
            fileMask[square] |= setFileOrRankMask(file, -1);

            // create rank mask for square
            rankMask[square] |= setFileOrRankMask(-1, rank);

            // create isolated pawn mask
            isolatedMask[square] |= setFileOrRankMask(file - 1, -1);
            isolatedMask[square] |= setFileOrRankMask(file + 1, -1);

            // support square

            // calculate support squares for white  
            if (file > 0 && rank < 7) // avoid edge cases on the left and bottom edges of the board
            {
                // bottom left support square
                setBit(whiteSupportMask[square], square + 7);
            }
            if (file < 7 && rank < 7) // avoid edge cases on the right and bottom edges of the board
            {
                // bottom right support square
                setBit(whiteSupportMask[square], square + 9);
            }

            // calculate support squares for black  
            if (file > 0 && rank > 0) // avoid edge cases on the left and bottom edges of the board
            {
                // top left support square
                setBit(blackSupportMask[square], square - 9);
            }
            if (file < 7 && rank > 0) // avoid edge cases on the right and bottom edges of the board
            {
                // bottom right support square
                setBit(blackSupportMask[square], square - 7);
            }

            // phalanx mask

            // calculate phalanx squares  
            if (file > 0) // avoid edge cases on the left and bottom edges of the board
            {
                // left phalanx square
                setBit(phalanxMask[square], square - 1);
            }
            if (file < 7) // avoid edge cases on the right and bottom edges of the board
            {
                // right phalanx square
                setBit(phalanxMask[square], square + 1);
            }
        }
    }
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;
            
            // white passed pawn masks
            whitePassedMask[square] |= setFileOrRankMask(file - 1, -1);
            whitePassedMask[square] |= setFileOrRankMask(file, -1);
            whitePassedMask[square] |= setFileOrRankMask(file + 1, -1);

            // white opposed pawn mask
            whiteOpposedMask[square] |= setFileOrRankMask(file, -1);

            // loop over wrong ranks
            for (int i = 0; i < (8 - rank); i++)
            {
                // reset wrong bits
                whitePassedMask[square] &= ~rankMask[(7 - i) * 8 + file];
                whiteOpposedMask[square] &= ~rankMask[(7 - i) * 8 + file];
            }

            // black passed pawn masks
            blackPassedMask[square] |= setFileOrRankMask(file - 1, -1);
            blackPassedMask[square] |= setFileOrRankMask(file, -1);
            blackPassedMask[square] |= setFileOrRankMask(file + 1, -1);

            // black opposed mask
            blackOpposedMask[square] |= setFileOrRankMask(file, -1);

            // loop over wrong ranks
            for (int i = 0; i < rank + 1; i++)
            {
                // reset wrong bits
                blackPassedMask[square] &= ~rankMask[i * 8 + file];
                blackOpposedMask[square] &= ~rankMask[i * 8 + file];
            }
        }
    }

    // set up king zone masks

    // loop through all squares on board
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // add squares king can reach
            whiteKingZoneMask[square] |= kingAttacks[square];
            blackKingZoneMask[square] |= kingAttacks[square];

            // add three squares in front

            // add left squares
            if (file > 0) // ensure it's not the first file
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 17));
                }


                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 15));
                }
            }
            else // king on first file (A)
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 14));
                }
                if (rank > 0)
                {
                    setBit(whiteKingZoneMask[square], (square - 6));
                }
                if (rank < 7)
                {
                    setBit(whiteKingZoneMask[square], (square + 10));
                }

                setBit(whiteKingZoneMask[square], (square + 2));



                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 18));
                }
                if (rank < 7)
                {
                    setBit(blackKingZoneMask[square], (square + 10));
                }
                if (rank > 0)
                {
                    setBit(blackKingZoneMask[square], (square - 6));
                }
                
                setBit(blackKingZoneMask[square], (square + 2));

            }


            // add right squares
            if (file < 7) // ensure it's not the first file
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 15));
                }


                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 17));
                }
            }
            else // king on last file (H)
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 18));
                }
                if (rank > 0)
                {
                    setBit(whiteKingZoneMask[square], (square - 10));
                }
                if (rank < 7)
                {
                    setBit(whiteKingZoneMask[square], (square + 6));
                }

                
                setBit(whiteKingZoneMask[square], (square - 2));
                

                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 14));
                }
                if (rank < 7)
                {
                    setBit(blackKingZoneMask[square], (square + 6));
                }
                if (rank > 0)
                {
                    setBit(blackKingZoneMask[square], (square - 10));
                }

                
                setBit(blackKingZoneMask[square], (square - 2));

            }



            // add middle squares

            // for white
            if (rank > 1)
            {
                setBit(whiteKingZoneMask[square], (square - 16));
            }


            // for black
            if (rank < 6)
            {
                setBit(blackKingZoneMask[square], (square + 16));
            }

        }
    }
    // loop over all squares
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // for white
            if (rank > 0)
            {
                setBit(whiteBlockedMask[square], (square - 8));
            }

            // for black
            if (rank < 7)
            {
                setBit(blackBlockedMask[square], (square + 8));
            }
        }
    }
    // loop over all squares for pinned masks
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            for (int direction = 0; direction < 8; direction++)
            {
                // get square
                int square = rank * 8 + file;

                // make mask
                U64 mask = 0;

                // make bitboard with square
                U64 bitboard = 1ULL << square;

                // make shifted bitboard
                U64 shiftedBitboard = shift(bitboard, direction);

                while (shiftedBitboard) 
                {
                    // update mask with shifted board
                    mask |= shiftedBitboard;
                    // get new shifted bitboard
                    shiftedBitboard = shift(shiftedBitboard, direction);
                }
                // init pinnedmask for current square and direction
                pinnedMasks[direction][square] = mask;
            }

        }
    }

    // get forward ranks bb
    // loop over all squares
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // for white
            for (int r = rank - 1; r >= 0; r--)
            {
                forwardRanksMasks[white][square] |= setFileOrRankMask(-1, r);
            }
            // for black
            for (int r = rank + 1; r <= 8; r++)
            {
                forwardRanksMasks[black][square] |= setFileOrRankMask(-1, r);
            }
        
        }
    }

}

void inline initAttacksTotal()
{
    // reset current tables
    memset(pieceAttackTables, 0, 112);
    memset(pawnDoubleTables, 0, 16);
    memset(attackedBy2, 0, 16);
    memset(defended, 0, 16);
    memset(safe, 0, 16);
    memset(weak, 0, 16);
    memset(stronglyProtected, 0, 16);

    // create pawn counter
    int pawns = 0;

    // pass bitboards for double+ attacks [side][pawn or total = 1]
    U64 firstPass[2][2] = {0};
    U64 secondPass[2][2] = {0};

    // loop over pieces
    for (int piece = P; piece <= k; piece++)
    {
        int side = (piece == P) ? white : black;

        U64 bitboard = bitboards[piece];
        while (bitboard)
        {
            // get square value
            int square = getLSFBIndex(bitboard);

            // if pawn
            if (piece == P || piece == p)
            {
                int side = (piece == P) ? white : black;

                U64 attacks = pawnAttacks[side][square];

                // update piece attack tables
                pieceAttackTables[side][P] |= attacks;

                // Check for double attacks by pawns
                secondPass[side][P] |= firstPass[side][P] & attacks;
                firstPass[side][P] |= attacks;
            }
            // if knight
            else if (piece == N || piece == n)
            {
                int side = (piece == N) ? white : black;

                U64 attacks = knightAttacks[square];

                pieceAttackTables[side][N] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else if (piece == B || piece == b)
            {
                int side = (piece == B) ? white : black;

                U64 attacks = getBishopAttacks(square, occupancies[both]);

                pieceAttackTables[side][B] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else if (piece == R || piece == r)
            {
                int side = (piece == R) ? white : black;

                U64 attacks = getRookAttacks(square, occupancies[both]);

                pieceAttackTables[side][R] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else if (piece == Q || piece == q)
            {
                int side = (piece == Q) ? white : black;

                U64 attacks = getQueenAttacks(square, occupancies[both]);

                pieceAttackTables[side][Q] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else // king
            {
                int side = (piece == K) ? white : black;

                U64 attacks = kingAttacks[square];

                pieceAttackTables[side][K] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }

            // pop current piece/bit
            popBit(bitboard, square);
        }
    }
    pawnDoubleTables[white] = secondPass[white][P];
    pawnDoubleTables[black] = secondPass[black][P];

    pieceAttackTables[white][allPieces] = pieceAttackTables[white][P] | pieceAttackTables[white][N] | pieceAttackTables[white][B] 
                                            | pieceAttackTables[white][R]|pieceAttackTables[white][Q] | pieceAttackTables[white][K];

    pieceAttackTables[black][allPieces] = pieceAttackTables[black][P] | pieceAttackTables[black][N] | pieceAttackTables[black][B] 
                                            | pieceAttackTables[black][R]|pieceAttackTables[black][Q] | pieceAttackTables[black][K];

    attackedBy2[white] = secondPass[white][1] | (pieceAttackTables[white][P] & firstPass[white][1]) | secondPass[white][P];
    attackedBy2[black] = secondPass[black][1] | (pieceAttackTables[black][P] & firstPass[black][1]) | secondPass[black][P];    
    
    /*
    // strongly protected squares are squares protected by pawns or 2 enemy pieces
    stronglyProtected[white] = pieceAttackTables[white][P] | (attackedBy2[white] & ~attackedBy2[black]);
    stronglyProtected[black] = pieceAttackTables[black][P] | (attackedBy2[black] & ~attackedBy2[white]);
    */

    /*
    // defended squares
    defended[white] = (occupancies[white] & ~bitboards[P]) & stronglyProtected[white];
    defended[black] = (occupancies[black] & ~bitboards[p]) & stronglyProtected[black];
    */

    // weak squares are not strongly protected and under attack
    weak[white] = pieceAttackTables[black][allPieces] & ~attackedBy2[white] & (~pieceAttackTables[white][allPieces] | pieceAttackTables[white][K] | pieceAttackTables[white][Q]);
    weak[black] = pieceAttackTables[white][allPieces] & ~attackedBy2[black] & (~pieceAttackTables[black][allPieces] | pieceAttackTables[black][K] | pieceAttackTables[black][Q]);

    /*
    // safe squares that are not attacked or are defended
    safe[white] = ~pieceAttackTables[black][allPieces] | pieceAttackTables[white][allPieces];
    safe[black] = ~pieceAttackTables[white][allPieces] | pieceAttackTables[black][allPieces];
    */
}

// scale attacks on king based off of attack value and number of attackers
int scaleAttacks(AttackInfo info) {
  int result = 0;
  int numberAttackers = info.numberAttackers;
  int attackValue = info.valueAttacks;

  switch (numberAttackers) {
  case 0: 
    result = attackValue * attackWeight[0] / 100; 
    break;
  case 1: 
    result = attackValue * attackWeight[1] / 100;
    break;
  case 2: 
    result = attackValue * attackWeight[2] / 100;
    break;
  case 3: 
    result = attackValue * attackWeight[3] / 100;
    break;
  case 4: 
    result = attackValue * attackWeight[4] / 100;
    break;
  default:
    result = attackValue * 2;
  }

  return -result;
}

// get number of attackers and attack value on king zone for side (disregarding their own pieces except for their pawns)
AttackInfo getAttackInfo(int kingSide)
{
    AttackInfo info;
    info.numberAttackers = 0;
    info.valueAttacks = 0;
    info.numberAttacks = 0;

    // for white
    if (!kingSide)
    {
        // get white king square
        int kingSquare = getLSFBIndex(bitboards[K]);

        // check knight attacks

        // get black knights bitboard
        U64 bitboard = bitboards[n];
        while (bitboard)
        {
            // get square of current knight
            int attackerSquare = getLSFBIndex(bitboard);
            
            U64 overlap = knightAttacks[attackerSquare] & whiteKingZoneMask[kingSquare] & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;
            if (overlap)
            { 
                info.valueAttacks += KingAttackValues[N];
                info.numberAttackers++;
            }
            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check bishop attacks
        // get black bishops bitboard
        bitboard = bitboards[b];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getBishopAttacks(attackerSquare, (occupancies[white] | bitboards[p]));
            U64 overlap = (attacks & whiteKingZoneMask[kingSquare]) & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;
            
            if (overlap)
            {
                info.valueAttacks += KingAttackValues[B];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check rook attacks
        // get black rook bitboard
        bitboard = bitboards[r];

        while (bitboard)
        {
            // get square of current rook
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getRookAttacks(attackerSquare, (occupancies[white] | bitboards[p]));
            U64 overlap = (attacks & whiteKingZoneMask[kingSquare]) & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackValues[R];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check queen attacks
        // get black queen bitboard
        bitboard = bitboards[q];

        while (bitboard)
        {
            // get square of current queen
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getQueenAttacks(attackerSquare, (occupancies[white] | bitboards[p]));
            U64 overlap = (attacks & whiteKingZoneMask[kingSquare]) & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackValues[Q];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }
    }
    
    // for black
    else
    {
        // get white king square
        int kingSquare = getLSFBIndex(bitboards[k]);
        
        // check knight attacks

        // get white knights bitboard
        U64 bitboard = bitboards[N];
        while (bitboard)
        {
            // get square of current knight
            int attackerSquare = getLSFBIndex(bitboard);
            
            U64 overlap = knightAttacks[attackerSquare] & blackKingZoneMask[kingSquare] & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackValues[N];
                info.numberAttackers++;
            }
            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check bishop attacks
        // get white bishops bitboard
        bitboard = bitboards[B];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getBishopAttacks(attackerSquare, (occupancies[black] | bitboards[P]));
            U64 overlap = (attacks & blackKingZoneMask[kingSquare]) & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackValues[B];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check rook attacks
        // get white rook bitboard
        bitboard = bitboards[R];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getRookAttacks(attackerSquare, (occupancies[black] | bitboards[P]));
            U64 overlap = (attacks & blackKingZoneMask[kingSquare]) & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackValues[R];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check queen attacks
        // get white queen bitboard
        bitboard = bitboards[Q];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getQueenAttacks(attackerSquare, (occupancies[black] | bitboards[P]));
            U64 overlap = (attacks & blackKingZoneMask[kingSquare]) & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackValues[Q];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

    }

    return info;
}

// function that evaluates the pawn penalty [file][white or black] (does both pawn storms and pawn shield)
int KingPawnPenalty(int file, int side)
{
    int penalty = 0;
    
    // white pawn
    if (side == white)
    {
        /* pawn shield */
        // white pawn hasnt moved
        if (getBit(bitboards[P], 6 * 8 + file));
        // pawn moved once
        else if (getBit(bitboards[P], 5 * 8 + file))
        {
            penalty -= 10;
        }
        // pawn moved more than once
        else if (fileMask[file] & bitboards[P])
        {
            penalty -= 20;
        }
        // no pawn on file
        else
        {
            penalty -= 25;
        }
        
        /* pawn storm + open file towards king */
        // if no enemy pawn -> open file
        if (!(fileMask[file] & bitboards[p]))
        {
            penalty -= 15;
        }
        // enemy pawn on 3rd rank
        else if (getBit(bitboards[p], 5 * 8 + file))
        {
            penalty -= 10;
        }
        // enemy pawn on 4th rank
        else if (getBit(bitboards[p], 4 * 8 + file))
        {
            penalty -= 5;
        }
    }
    // black pawn
    else
    {
        /* pawn shield */
        // black pawn hasnt moved
        if (getBit(bitboards[p], 1 * 8 + file));
        // pawn moved once
        else if (getBit(bitboards[p], 2 * 8 + file))
        {
            penalty -= 10;
        }
        // pawn moved more than once
        else if (fileMask[file] & bitboards[p])
        {
            penalty -= 20;
        }
        // no pawn on file
        else
        {
            penalty -= 25;
        }
        
        /* pawn storm + open file towards king */
        // if no enemy pawn -> open file
        if (!(fileMask[file] & bitboards[P]))
        {
            penalty -= 15;
        }
        // enemy pawn on 3rd rank
        else if (getBit(bitboards[p], 2 * 8 + file))
        {
            penalty -= 10;
        }
        // enemy pawn on 4th rank
        else if (getBit(bitboards[p], 3 * 8 + file))
        {
            penalty -= 5;
        }
    }
    return penalty;
}

// game phase calculator function
static inline int getGameStageScore()
{    
    // white & black game phase scores
    int whitePieceScores = 0; int blackPieceScores = 0;
    
    // loop over white pieces (except for pawns and kings)
    for (int piece = N; piece <= Q; piece++)
        whitePieceScores += countBits(bitboards[piece]) * materialScore[opening][piece];
    
    // loop over black pieces (except for pawns and kings)
    for (int piece = n; piece <= q; piece++)
        blackPieceScores += countBits(bitboards[piece]) * -materialScore[opening][piece];
    
    // return game phase score
    return whitePieceScores + blackPieceScores;
}

// Interpolation function
static inline int interpolate(int openingValue, int endgameValue, int stageScore) {
    return (openingValue * stageScore + endgameValue * (openingPhaseScore - stageScore)) / openingPhaseScore;
}

// evaluate material only for a side
static inline int evaluateMaterial(int side)
{
    // evaluation score
    int score = 0;

    // copy of the current pieces
    U64 bitboard;

    // get piece and square
    int piece, square, pawn, king;
    
    if (side == white)
    {
        pawn = P;
        king = K;
    }
    else
    {
        pawn = p;
        king = k;
    }

    // loop over all bits in bitboard
    for (int bitPiece = pawn; bitPiece <= king; bitPiece++)
    {
        // get bitboard copy of the current piece
        bitboard = bitboards[bitPiece];

        // loop over bits in bitboard
        while (bitboard)
        {
            // get piece
            piece = bitPiece;

            // get square
            square = getLSFBIndex(bitboard);

            // score material of the piece
            score += defaultMaterialScore[piece];

            // pop current bit
            popBit(bitboard, square);
        }
    }
    
    // return final eval based on side
    return (side == white) ? score : -score;
}

// get manhattan distance in chessboard
inline int distance(int s1, int s2) 
{
    int file1 = getFile[s1];
    int rank1 = getRank[s1];
    int file2 = getFile[s2];
    int rank2 = getRank[s2];
    return abs(file1 - file2) + abs(rank1 - rank2);
}

// clamp file (return input file unless outside of bounds of the clamp)
inline int clamp(int sq, int file1, int file2)
{
    int sqFile = getFile[sq];
    int file1File = getFile[file1];
    int file2File = getFile[file2];

    if (sqFile < file1File) {
        return file1File;
    } else if (sqFile > file2File) {
        return file2File;
    } else {
        return sqFile;
    }
}

// frontmost square returns the most advanced bit relative to the colour
inline int frontMostSquare(int currentSide, U64 bb) 
{ 
    return (currentSide == white) ? getLSFBIndex(bb) : getMSBIndex(bb); 
}

// get relative rank for side
int relativeRank(int currentSide, int rank)
{
    if (currentSide == white)
    {
        // relative rank for white is the same
        return rank;
    }
    else
    {
        // inverse rank for black
        return 7 - rank;
    }
}

// create attack info struct
struct kingShelter {
    int mgBonus;
    int egBonus; 
};

// get king shelter bonus for side
static inline kingShelter getKingShelter(int currentSide, int ksq, kingShelter shelterScore)
{
    // get our pawns and their pawns (not counting pawns behind our king)
    U64 ourPawns = bitboards[(currentSide == white) ? P : p] & ~forwardRanksMasks[!currentSide][ksq];
    U64 theirPawns = bitboards[(currentSide == white) ? p : P] & ~forwardRanksMasks[!currentSide][ksq];

    // initial bonus
    kingShelter bonus;
    bonus.mgBonus = 5;
    bonus.egBonus = 5;

    int fileCenter = getFile[clamp(ksq, b1, g1)];
    
    U64 b;

    // loop over the file to the left, center, and right
    for (int f = fileCenter - 1; f <= fileCenter + 1; f++)
    {
        // get bb of our pawns and the current file
        b = ourPawns & fileMask[f];
        // get our rank
        int ourRank = b ? relativeRank(currentSide, getRank[frontMostSquare(!currentSide, b)]) : 0;

        // get bb of enemy pawns and the current file
        b = theirPawns & fileMask[f];
        int theirRank = b ? relativeRank(currentSide, getRank[frontMostSquare(!currentSide, b)]) : 0;

        //printf("f: %d, not f: %d\n", f, 7 - f);

        int d = std::min(f, 7 - f);
        //printf("d: %d\n", d);
        bonus.mgBonus += ShelterStrength[d][ourRank];

        if (ourRank && (ourRank == theirRank - 1))
        {
            bonus.mgBonus -= 82 * (theirRank == 2);
            bonus.egBonus -= 82 * (theirRank == 2);
        }
        else
        {
            bonus.mgBonus -= UnblockedStorm[d][theirRank];
        }
    }

    if (bonus.mgBonus > shelterScore.mgBonus)
    {
        return bonus;
    }
    else
    {
        return shelterScore;
    }
}

// get jubg safety
inline kingShelter getKingSafety(int currentSide)
{
    // get king square for side
    int ksq = getLSFBIndex(bitboards[(currentSide == white) ? K : k]);

    // get current castling rights
    int cRights = castle;

    // set min pawn distance to 8 if pawn is non-zero, 0 if no pawns
    U64 pawns = bitboards[(currentSide == white) ? P : p];
    int minPawnDist = pawns ? 8 : 0;

    // if pawn 1 square away from king
    if (pawns & kingAttacks[ksq])
    {
        // min pawn distance is 1
        minPawnDist = 1;
    }
    
    // else, find min pawn distance
    else while (pawns)
    {
        int sq = getLSFBIndex(pawns);
        int dist = distance(ksq, sq);

        // replace min distance if dist is smaller
        if (dist < minPawnDist)
        {
            minPawnDist = dist;
        }

        // pop bit
        popBit(pawns, sq);
    }

    // evaluate shelter
    kingShelter shelter;
    shelter.mgBonus = -infinity;
    shelter.egBonus = 0;
    // get first shelter score
    shelter = getKingShelter(currentSide, ksq, shelter);

    
    // if we can castle use the bonus after the castling 
    if (currentSide == white)
    {
        // check king castle is possible
        if (cRights & wk)
        {
            shelter = getKingShelter(currentSide, g1, shelter);
        }
        if (cRights & wq)
        {
            shelter = getKingShelter(currentSide, c1, shelter);
        }
    }
    else // for black
    {
        // check king castle is possible
        if (cRights & bk)
        {
            shelter = getKingShelter(currentSide, g8, shelter);
        }
        if (cRights & bq)
        {
            shelter = getKingShelter(currentSide, c8, shelter);
        }
    }
    

    shelter.egBonus -= 16 * minPawnDist;

    return shelter;
}

// evaluate a position
static inline int evaluate()
{
    // game phase score
    int stageScore = getGameStageScore();

    // game stage (opening, mg, eg)
    int stage = 0;

    // set game stage
    if (stageScore > openingPhaseScore)
    {
        stage = opening;
    }
    else if (stageScore < endgamePhaseScore)
    {
        stage = endgame;
    }
    else
    {
        stage = middlegame;
    }

    // evaluation score
    int score = 0;

    // add tempo bonus
    (side == white) ? score += tempoBonus : score -= tempoBonus;

    // copy of the currena6t pieces
    U64 bitboard;

    // get piece and square
    int piece, square;
    
    // get penalty and bonus variables
    int doublePawns, isolatedPawns, backwardPawns;

    // loop over all bits in bitboard
    for (int bitPiece = P; bitPiece <= k; bitPiece++)
    {
        // get bitboard copy of the current piece
        bitboard = bitboards[bitPiece];

        // loop over bits in bitboard
        while (bitboard)
        {
            // get piece
            piece = bitPiece;

            // get square
            square = getLSFBIndex(bitboard);

            // if in middlegame
            if (stage == middlegame)
            {
                // interpolate value
                score += interpolate(materialScore[opening][piece], materialScore[endgame][piece], stageScore);
            }
            else // in pure opening or eg
            {
                // score piece w/ pure values
                score += materialScore[stage][piece];
            }

            // add positional score
            switch (piece)
            {
                // white pieces
                case P: 
                {
                    // double pawn count (returns number of pawns in file)
                    doublePawns = countBits(bitboards[P] & fileMask[square]);
                    
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score += interpolate(PieceTables[opening][P][square], PieceTables[endgame][P][square], stageScore);
                    
                        // on double pawns (or even triple)
                        if (doublePawns > 1)
                        {
                            score += interpolate(doublePawnPenalty[opening], doublePawnPenalty[endgame], stageScore);
                        }
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score += PieceTables[stage][P][square]; 

                        // on double pawns (or even triple)
                        if (doublePawns > 1)
                        {
                            score += doublePawnPenalty[stage];
                        }
                    }
                    
                    // get if pawn is weak and unopposed (no black pawn in front)
                    int isUnopposed = ((whiteOpposedMask[square] & bitboards[p]) == 0) ? 1 : 0;  

                    // if passed pawn
                    if ((whitePassedMask[square] & bitboards[p]) == 0)
                    {
                        // give bonus for rank
                        score += interpolate(passedPawnRankBonus[opening][getRank[square]], passedPawnRankBonus[endgame][getRank[square]], stageScore);
                        
                        // give bonus for file
                        score += interpolate(passedPawnFileBonus[opening][getFile[square]], passedPawnFileBonus[endgame][getFile[square]], stageScore);
                    }

                    // connected pawn calculations

                    // get if pawn is phalanx
                    int phalanx = ((phalanxMask[square] & bitboards[P]) != 0) ? 1 : 0;  
                    // get if pawn is supported
                    int supported = ((whiteSupportMask[square] & bitboards[P]) != 0) ? 1 : 0;

                    // get number of supports
                    int supportedCount = countBits(whiteSupportMask[square] & bitboards[P]);
                    
                    // check if backward (no support + enemy pawn controlling square in front + not in starter rank)
                    backwardPawns = ((supported == 0) && (pawnAttacks[white][square - 8] & bitboards[p])) ? 1 : 0;

                    // grant connected pawn bonus
                    if (supported | phalanx)
                    {
                        score += connectedPawnBonus[getRank[square]] * (phalanx ? 3 : 2) / (!isUnopposed ? 2 : 1) + 17 * supportedCount;
                    }

                    // if isolated pawn
                    else if (((bitboards[P] & isolatedMask[square]) == 0))
                    {
                        int isolatedPen = 0;
                        int weakPen = 0;

                        // if in middlegame
                        if (stage == middlegame)
                        {
                            // interpolate value                        
                            isolatedPen = interpolate(isolatedPawnPenalty[opening], isolatedPawnPenalty[endgame], stageScore);
                            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
                        }
                        else // in pure opening or eg
                        {
                            isolatedPen = isolatedPawnPenalty[stage];
                            weakPen = weakUnopposed[stage];
                        }

                        // apply penalty if there is no black pawn opposite
                        if (isUnopposed)
                        {
                            score += isolatedPen + weakPen;
                        }
                        else
                        {
                            score += isolatedPen;
                        }
                    }                    
                    
                    // update score with backward penalty
                    else if (backwardPawns)
                    {
                        int backwardPen = 0;
                        int weakPen = 0;

                        // if in middlegame
                        if (stage == middlegame)
                        {
                            // interpolate value                        
                            backwardPen = interpolate(backwardPawnPenalty[opening], backwardPawnPenalty[endgame], stageScore);
                            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
                        }
                        else // in pure opening or eg
                        {
                            backwardPen = backwardPawnPenalty[stage];
                            weakPen = weakUnopposed[stage];
                        }

                        score += backwardPen + weakPen;
                    }

                    break;
                }

                case N: 
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value psq
                        score += interpolate(PieceTables[opening][N][square], PieceTables[endgame][N][square], stageScore);

                        // interpolate mobility
                        score += interpolate(mobilityBonus[N][mobilityW.knight][opening], mobilityBonus[N][mobilityW.knight][endgame], stageScore);

                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score += PieceTables[stage][N][square]; 

                        // score piece mobility purely
                        score += mobilityBonus[N][mobilityW.knight][stage];
                    }

                    // apply penalty/bonus adjustment according to number of pawns
                    score += knightAdj[countBits(bitboards[P])];
                    break;
                }
                case B:
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score += interpolate(PieceTables[opening][B][square], PieceTables[endgame][B][square], stageScore);

                        // interpolate mobility
                        score += interpolate(mobilityBonus[B][mobilityW.bishop][opening], mobilityBonus[B][mobilityW.bishop][endgame], stageScore);

                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score += PieceTables[stage][B][square]; 

                        // score piece mobility purely
                        score += mobilityBonus[B][mobilityW.bishop][stage];
                    }

                    break;
                } 
                case R: 
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score += interpolate(PieceTables[opening][R][square], PieceTables[endgame][R][square], stageScore);

                        // interpolate mobility
                        score += interpolate(mobilityBonus[R][mobilityW.rook][opening], mobilityBonus[R][mobilityW.rook][endgame], stageScore);
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score += PieceTables[stage][R][square]; 

                        // score piece mobility purely
                        score += mobilityBonus[R][mobilityW.rook][stage];
                    }

                    // apply penalty/bonus adjustment according to number of pawns
                    score += rookAdj[countBits(bitboards[P])];

                    //  if semi open file
                    if ((bitboards[P] & fileMask[square]) == 0)
                    {
                        score += interpolate(semiOpenFileScore[opening], semiOpenFileScore[endgame], stageScore);
                    }
                    //  if open file
                    if (((bitboards[P] | bitboards[p]) & fileMask[square]) == 0)
                    {
                        score += interpolate(openFileScore[opening], openFileScore[endgame], stageScore);
                    }

                    break;
                }
                case Q: 
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score += interpolate(PieceTables[opening][Q][square], PieceTables[endgame][Q][square], stageScore);

                        // interpolate mobility
                        score += interpolate(mobilityBonus[Q][mobilityW.queen][opening], mobilityBonus[Q][mobilityW.queen][endgame], stageScore);
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score += PieceTables[stage][Q][square]; 

                        // score piece mobility purely
                        score += mobilityBonus[Q][mobilityW.queen][stage];
                    }

                    break;
                }
                case K:
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score += interpolate(PieceTables[opening][K][square], PieceTables[endgame][K][square], stageScore);
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score += PieceTables[stage][K][square]; 
                    }
                    
                    int kingPenalty = 0;

                    
                    AttackInfo info = getAttackInfo(white);

                    kingPenalty += info.numberAttackers * info.valueAttacks
                                + 69 * info.numberAttacks
                                + 185 * countBits(whiteKingZoneMask[square] & weak[white])
                                - 100 * bool(pieceAttackTables[white][N] & pieceAttackTables[white][K])
                                - 35 * bool(pieceAttackTables[white][B] & pieceAttackTables[white][K])
                                - 873 * !countBits(bitboards[q])
                                - 7;
                
                    // transform king penalty
                    if (kingPenalty > 100)
                    {
                        score -= interpolate(kingPenalty * kingPenalty / 4096, kingPenalty / 16, stageScore);
                    }
                    
                    
                    // get king shelter values
                    
                    kingShelter bonus = getKingSafety(white);
                    
                    
                    //printf("mg: %d\neg: %d\n", bonus.mgBonus, bonus.egBonus);
                    /*
                    if (bonus.mgBonus < -100 || bonus.mgBonus > 100 || bonus.egBonus < -100 || bonus.mgBonus > 100)
                    {
                        printf("mg: %d\neg: %d\n", bonus.mgBonus, bonus.egBonus);

                        printBoard();
                    }
                    */
                    score += interpolate(bonus.mgBonus, bonus.egBonus, stageScore);
                    
                    break;
                }

                // black pieces
                case p: 
                {
                    // double pawn count (returns number of pawns in file)
                    doublePawns = countBits(bitboards[p] & fileMask[square]);

                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score -= interpolate(PieceTables[opening][P][mirrorScore[square]], PieceTables[endgame][P][mirrorScore[square]], stageScore);
                        
                        // interpolate double pawns
                        if (doublePawns > 1)
                        {
                            score -= interpolate(doublePawnPenalty[opening], doublePawnPenalty[endgame], stageScore);
                        }
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score -= PieceTables[stage][P][mirrorScore[square]]; 

                        // on double pawns (or even triple)
                        if (doublePawns > 1)
                        {
                            score -= doublePawnPenalty[stage];
                        }
                    }

                    // get if pawn is weak and unopposed (no white pawn in front)
                    int isUnopposed = ((blackOpposedMask[square] & bitboards[P]) == 0) ? 1 : 0;  

                    // if passed pawn
                    if ((blackPassedMask[square] & bitboards[P]) == 0)
                    {
                        // give bonus for rank
                        score -= interpolate(passedPawnRankBonus[opening][getRank[mirrorScore[square]]], passedPawnRankBonus[endgame][getRank[mirrorScore[square]]], stageScore);
                        
                        // give bonus for file
                        score -= interpolate(passedPawnFileBonus[opening][getFile[mirrorScore[square]]], passedPawnFileBonus[endgame][getFile[mirrorScore[square]]], stageScore);
                    }

                    // connected pawns
                    
                    // get if pawn is phalanx
                    int phalanx = ((phalanxMask[square] & bitboards[p]) != 0) ? 1 : 0;  
                    // get if pawn is supported
                    int supported = ((blackSupportMask[square] & bitboards[p]) != 0) ? 1 : 0;  
                    
                    // get number of supports
                    int supportedCount = countBits(blackSupportMask[square] & bitboards[p]);

                    // check if backward (no support + enemy pawn controlling square in front + not in starter rank)
                    backwardPawns = ((supported == 0) && (pawnAttacks[black][square + 8] & bitboards[P])) ? 1 : 0;

                    // grant connected pawn bonus
                    if (supported | phalanx)
                    {
                        score -= connectedPawnBonus[getRank[mirrorScore[square]]] * (phalanx ? 3 : 2) / (!isUnopposed ? 2 : 1) + + 17 * supportedCount;
                    }

                    // if isolated pawn
                    else if ((bitboards[p] & isolatedMask[square]) == 0)
                    {
                        int isolatedPen = 0;
                        int weakPen = 0;

                        // if in middlegame
                        if (stage == middlegame)
                        {
                            // interpolate value
                            isolatedPen = interpolate(isolatedPawnPenalty[opening], isolatedPawnPenalty[endgame], stageScore);
                            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
                        }
                        else // in pure opening or eg
                        {
                            isolatedPen = isolatedPawnPenalty[stage];
                            weakPen = weakUnopposed[stage];
                        }

                        // apply penalty if there is no black pawn opposite
                        if (isUnopposed)
                        {
                            score -= isolatedPen + weakPen;
                        }
                        else
                        {
                            score -= isolatedPen;
                        }
                    }                    
                    
                    // update score with backward penalty
                    else if (backwardPawns)
                    {
                        int backwardPen = 0;
                        int weakPen = 0;

                        // if in middlegame
                        if (stage == middlegame)
                        {
                            // interpolate value
                            backwardPen = interpolate(backwardPawnPenalty[opening], backwardPawnPenalty[endgame], stageScore);
                            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
                        }
                        else // in pure opening or eg
                        {
                            backwardPen = backwardPawnPenalty[stage];
                            weakPen = weakUnopposed[stage];
                        }

                        score -= backwardPen + weakPen;
                    }

                    break;
                }

                case n: 
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score -= interpolate(PieceTables[opening][N][mirrorScore[square]], PieceTables[endgame][N][mirrorScore[square]], stageScore);
                    
                        // interpolate mobility
                        score -= interpolate(mobilityBonus[N][mobilityB.knight][opening], mobilityBonus[N][mobilityB.knight][endgame], stageScore);

                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score -= PieceTables[stage][N][mirrorScore[square]]; 

                        // score mobility purely on stage
                        score -= mobilityBonus[N][mobilityB.knight][stage];
                    }

                    // apply penalty/bonus adjustment according to number of pawns
                    score -= knightAdj[countBits(bitboards[p])];
                    
                    break;
                }
                case b: 
                {                    
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score -= interpolate(PieceTables[opening][B][mirrorScore[square]], PieceTables[endgame][B][mirrorScore[square]], stageScore);
                    
                        // interpolate mobility
                        score -= interpolate(mobilityBonus[B][mobilityB.bishop][opening], mobilityBonus[B][mobilityB.bishop][endgame], stageScore);
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score -= PieceTables[stage][B][mirrorScore[square]]; 

                        // score mobility purely on stage
                        score -= mobilityBonus[B][mobilityB.bishop][stage];

                    }
                    
                    break;
                }
                case r:
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score -= interpolate(PieceTables[opening][R][mirrorScore[square]], PieceTables[endgame][R][mirrorScore[square]], stageScore);
                        
                        // interpolate mobility
                        score -= interpolate(mobilityBonus[R][mobilityB.rook][opening], mobilityBonus[R][mobilityB.rook][endgame], stageScore);

                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score -= PieceTables[stage][R][mirrorScore[square]]; 

                        // score mobility purely on stage
                        score -= mobilityBonus[R][mobilityB.rook][stage];

                    }

                    // apply penalty/bonus adjustment according to number of pawns
                    score -= rookAdj[countBits(bitboards[p])];
                    
                    //  if semi open file
                    if ((bitboards[p] & fileMask[square]) == 0)
                    {
                        score -= interpolate(semiOpenFileScore[opening], semiOpenFileScore[endgame], stageScore);
                    }
                    //  if open file
                    if (((bitboards[P] | bitboards[p]) & fileMask[square]) == 0)
                    {
                        score -= interpolate(openFileScore[opening], openFileScore[endgame], stageScore);
                    }

                    break;
                }
                case q: 
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score -= (PieceTables[opening][Q][mirrorScore[square]] * stageScore + PieceTables[endgame][Q][mirrorScore[square]] * (openingPhaseScore - stageScore)) / openingPhaseScore;
                        
                        // interpolate mobility
                        score -= interpolate(mobilityBonus[Q][mobilityB.queen][opening], mobilityBonus[Q][mobilityB.queen][endgame], stageScore);
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score -= PieceTables[stage][Q][mirrorScore[square]]; 

                        // score mobility purely on stage
                        score -= mobilityBonus[Q][mobilityB.queen][stage];

                    }
                    
                    break;
                }
                case k:
                {
                    // if in middlegame
                    if (stage == middlegame)
                    {
                        // interpolate value
                        score -= interpolate(PieceTables[opening][K][mirrorScore[square]], PieceTables[endgame][K][mirrorScore[square]], stageScore);
                    }
                    else // in pure opening or eg
                    {
                        // score piece w/ pure positional values
                        score -= PieceTables[stage][K][mirrorScore[square]]; 
                    }
                    
                    
                    int kingPenalty = 0;
                    AttackInfo info = getAttackInfo(black);

                    kingPenalty += info.numberAttackers * info.valueAttacks
                                + 69 * info.numberAttacks
                                + 185 * countBits(blackKingZoneMask[square] & weak[black])
                                - 100 * bool(pieceAttackTables[black][N] & pieceAttackTables[black][K])
                                - 35 * bool(pieceAttackTables[black][B] & pieceAttackTables[black][K])
                                - 873 * !countBits(bitboards[Q])
                                - 7;

                    // transform king penalty
                    if (kingPenalty > 100)
                    {
                        score += interpolate(kingPenalty * kingPenalty / 4096, kingPenalty / 16, stageScore);
                    }

                    
                    
                    // get king shelter values
                    kingShelter bonus = getKingSafety(black);
                    
                    /*
                    //printf("mg: %d\neg: %d\n", bonus.mgBonus, bonus.egBonus);
                    if (bonus.mgBonus < -300 || bonus.mgBonus > 300 || bonus.egBonus < -300 || bonus.mgBonus > 300)
                    {
                        printf("mg: %d\neg: %d\n", bonus.mgBonus, bonus.egBonus);
                        printBoard();
                    }
                    */
                    score -= interpolate(bonus.mgBonus, bonus.egBonus, stageScore);
                    

                    break;
                }
            }

            // pop current bit
            popBit(bitboard, square);
        }
    }
    // fifty move penalty to stop infinite shuffling
    score *= static_cast<float>(100 - fifty) / 100.0f;

    // return final eval based on side
    return (side == white) ? static_cast<int>(score) : -static_cast<int>(score);
}

// test king eval
void testKingEval()
{
    // king penalty
    // print attack info
    AttackInfo infoWhite = getAttackInfo(white);
    AttackInfo infoBlack = getAttackInfo(black);

    printf("white attack info: \nnumberAttackers: %d\nvalueAttackers:%d\nnumberAttacks:%d\n", infoWhite.numberAttackers, infoWhite.valueAttacks, infoWhite.numberAttacks);
    printf("black attack info: \nnumberAttackers: %d\nvalueAttackers:%d\nnumberAttacks:%d\n", infoBlack.numberAttackers, infoBlack.valueAttacks, infoBlack.numberAttacks);

    // print king penalty stuff
    int kingPenaltyWhite = 0;
    int kingPenaltyBlack = 0;

}  

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

             Transposition Table
           (using Zobrist Hashing)
                   
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// number of hash entries
int hashEntries = 0;

// no hash entry
#define noHashEntry 100000 // to be outside of alpha-beta bounds

// transposition table flags
#define hashFlagExact 0
#define hashFlagAlpha 1
#define hashFlagBeta 2

// transposition table structure
typedef struct 
{
    U64 hashKey; 
    int depth; // depth of move
    int flag; // (if failed high, low or PV node)
    int score; // eval
    int move; // move
    int best; // best move in position
} tt;

// create transposition table
tt *transpositionTable = NULL;

// clear table
void clearTT()
{
    // get tt pointer to current hash entry
    tt *hashEntry;

    // loop over all entries
    for (hashEntry = transpositionTable; hashEntry < transpositionTable + hashEntries; hashEntry++)
    {
        // reset all tt fields
        hashEntry->hashKey = 0;
        hashEntry->depth = 0;
        hashEntry->flag = 0;
        hashEntry->score = 0;
        hashEntry->move = 0;
    }
}

// dynamically allocate memory for tt
void initTT(int mb)
{
    // hash size
    int hashSize = 0x100000 * mb;

    // get hash entries
    hashEntries = hashSize / sizeof(tt);

    // clear hash memory (initialised before)
    if (transpositionTable != NULL)
    {
        printf("    Clearing hash memory\n");

        // free memory
        free(transpositionTable);
    }

    // allocate memory
    transpositionTable = (tt *) malloc(hashEntries * sizeof(tt));

    // if allocation failed
    if (transpositionTable == NULL)
    {
        printf(" Failed to allocate memory, tryinr %dMB\n", mb/2);

        // try half size
        initTT(mb/2);
    }
    else // if allocation successful
    {
        // clear table
        clearTT();

    }
}

// read (probe) hash
static inline int probeHash(int alpha, int beta, int depth, int* best)
{
    // create TT pointer (points to the element of the tt)
    // since keys are stored using modulo, % should be used to avoid collisions
    tt *hashEntry = &transpositionTable[hashKey % hashEntries];

    // check if exact position
    if (hashEntry->hashKey == hashKey)
    {
        // check if the same depth
        if (hashEntry->depth >= depth)
        {
            // get stored score from TT
            int score = hashEntry->score;
            
            // adjust score for mate scores
            if (score < -mateScore) score += ply;
            if (score > mateScore) score -= ply;

            // match the flag
            // PV node
            if (hashEntry->flag == hashFlagExact)
            {
                // return pv score
                return score;
            }
            // alpha score (fail low)
            if ((hashEntry->flag == hashFlagAlpha) && (score <= alpha))
            {
                // return alpha
                return alpha;
            }
            // beta score (fail high)
            if ((hashEntry->flag == hashFlagBeta) && (score >= beta))
            {
                // return beta
                return beta;
            }
        }

        // store best move
        *best = hashEntry->best;
    }
    // if no match
    return noHashEntry;
}

// write (record) hash
static inline void recordHash(int score, int depth, int hashFlag, int move, int best)
{
    // create TT pointer (points to the element of the tt)
    // since keys are stored using modulo, % should be used to avoid collisions
    tt *hashEntry = &transpositionTable[hashKey % hashEntries];

    // adjust score (for mating scores)
    if (score < -mateScore) score -= ply;
    if (score > mateScore) score += ply;

    // write hash entry
    hashEntry->hashKey = hashKey;
    hashEntry->score = score;
    hashEntry->depth = depth;
    hashEntry->flag = hashFlag;
    hashEntry->move = move;
    hashEntry->best = best;
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                    Search
                   
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// table for mvvlva
/*

    (Victims) Pawn  Knight  Bishop  Rook  Queen  King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600
        
*/                 

// Most Valuable Victim - Least Valuable Attacker [attacker][victim]
static int mvvlva[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

// max ply we can reach in a search
#define maxPly 64

// 2 killer moves [id][ply]
int killerMoves[2][maxPly];

// history moves
int historyMoves[12][64];

/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5
      
      0    m1   m2   m3   m4   m5   m6
      
      1    0    m2   m3   m4   m5   m6 
      
      2    0    0    m3   m4   m5   m6
      
      3    0    0    0    m4   m5   m6
       
      4    0    0    0    0    m5   m6
      
      5    0    0    0    0    0    m6
*/

// PV length [ply]
int PVLength[maxPly];

// PV table [ply][ply]
int PVTable[maxPly][maxPly];

// follow PV and score PV (if follow = 1, follow pv, 0 we don't)
int followPV, scorePV;

// enable pv move scoring
static inline void enablePVScoring(moves *moveList)
{
    // disable pv follow
    followPV = 0;

    // loop over moves
    for (int count = 0; count < moveList->count; count++)
    {
        // make sure its a pv move
        if (PVTable[0][ply] == moveList->moves[count])
        {
            // enable move scoring
            scorePV = 1;

            // enable following (since if we find this pv move, we continue in pv)
            followPV = 1;
        }
    }
}

/*  =======================
         Move ordering
    =======================
    
    1. PV move
    2. Captures in MVV/LVA
    3. 1st killer move
    4. 2nd killer move
    5. History moves
    6. Unsorted moves
*/

// score move function (for move ordering)
static inline int scoreMove(int move)
{
    // if pv move scoring is enabled
    if (scorePV)
    {
        // check if we are in pv
        if (PVTable[0][ply] == move)
        {
            // disable score flag
            scorePV = 0;
            
            // return high for pv move so it is searched first
            return 20000;
        }
    }

    if (getCapture(move))
    {
        // create target piece variable
        int targetPiece = P;

        // get range of piece bitboards depending on side
        int startPiece, endPiece;

        // if white to move, get indices for black pieces
        if (side == white)
        {
            startPiece = p;
            endPiece = k;
        }
        else // for black get white piece range
        {
            startPiece = P;
            endPiece = K;
        }

        // loop over enemy's bitboard and remove the bit on the target square if it exists
        for (int bitPiece = startPiece; bitPiece <= endPiece; bitPiece++)
        {
            if (getBit(bitboards[bitPiece], getTarget(move)))
            {
                targetPiece = bitPiece;
                // break out of loop as piece has been found
                break;
            }
        }

        // score move using mvvlva[source][target]
        return mvvlva[getPiece(move)][targetPiece] + 10000;
    }

    //score quiet move
    else
    {
        // score 1st killer move
        if (killerMoves[0][ply] == move)
        {
            return 9000;
        }

        // score 2nd killer move
        else if (killerMoves[1][ply] == move)
        {
            return 8000;
        }

        // score history move
        else
        {
            return historyMoves[getPiece(move)][getTarget(move)];
        }
    }

    return 0;
}

// sort moves (for better pruning)
static inline void sortMoves(moves *moveList, int best)
{
    // create move scores
    int moveScores[moveList->count];

    // score all moves
    for (int count = 0; count < moveList->count; count++)
    {
        // if hash has best move
        if (best == moveList->moves[count])
        {
            // score move (for priority)
            moveScores[count] = 30000;
        }
        else
        {
            // score move normally
            moveScores[count] = scoreMove(moveList->moves[count]);

        }
    }

    // loop over move in list (specialMovesFound depending on if there was a PV move)
    for (int current = 0; current < moveList->count; current++)
    {
        // loop over next move
        for (int next = current + 1; next < moveList->count; next++)
        {
            // compare current and next move
            if (moveScores[current] < moveScores[next])
            {
                // swap scores
                int tempScore = moveScores[current];
                moveScores[current] = moveScores[next];
                moveScores[next] = tempScore;

                // swap moves
                int tempMove = moveList->moves[current];
                moveList->moves[current] = moveList->moves[next];
                moveList->moves[next] = tempMove;
            }
        }
    }
}

// print move scores
void printMoveScores(moves *moveList)
{
    for (int count = 0; count < moveList->count; count++)
    {
        printMove(moveList->moves[count]);
        printf(" score: %d\n", scoreMove(moveList->moves[count]));
    }
}

// repetition detection
static inline int isRepetition()
{
    // loop over repetition index range
    for (int index = 0; index < repetitionIndex; index++)
    {
        if (repetitionTable[index] == hashKey)
        {
            return 1;
        }
    }
    
    // if no repetition
    return 0;
}

// quiescence search (to stop the horizon effect)
static inline int quiescence(int alpha, int beta)
{
    // every 2047 nodes check for user input
    if ((nodes & 2047) == 0)
    {
        // listen to GUI or user input
        communicate();
    }
    
    // add nodes
    nodes++;
    
    // drop search if max ply
    if (ply > maxPly - 1)
    {
        // just evaluate position
        return evaluate();
    }

    // evaluate position
    int eval = evaluate();
    
    // fail-hard beta cutoff (score can't go outside of alpha beta bounds)
    if (eval >= beta)
    {
        // node fails high 
        return beta;
    }

    // if found a better move
    if (eval > alpha)
    {
        // PV (principal variation) node
        alpha = eval;
    }

    // create moves list
    moves moveList[1];

    // gen moves
    generateMoves(moveList);

    // sort moves mvvlva
    sortMoves(moveList, 0);

    // loop over moves
    for (int count = 0; count < moveList->count; count++)
    {
        // copy the board
        copyBoard();

        // increment ply
        ply++;

        // increment repetition & store hash
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        // only make legal moves
        if (makeMove(moveList->moves[count], capturesOnly) == 0)
        {
            // decrease ply
            ply--;

            // decrease repetition & store hash
            repetitionIndex--;

            // skip
            continue;
        }

        // score the move
        int score = -quiescence(-beta, -alpha);

        // decrease ply
        ply--;

        // decrease repetition & store hash
        repetitionIndex--;

        // undo move
        undoBoard();

        // return 0 if time is up
        if (stopped == 1)
        {
            return 0;
        }

        // if found a better move
        if (score > alpha)
        {
            // PV (principal variation) node
            alpha = score;

            // fail-hard beta cutoff (score can't go outside of alpha beta bounds)
            if (score >= beta)
            {
                // node fails high 
                return beta;
            }
        }
    } 
    // node fails low
    return alpha;
}

const int fullDepthMoves = 4;
const int reductionLimit = 3;

// negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth)
{       
    // create PV length
    PVLength[ply] = ply;
    
    // create move variable
    int move, score;

    // best move
    int bestMove = 0;

    // get hash flag (initially as alpha flag)
    int hashFlag = hashFlagAlpha;

    // if position repetition occurs
    if (ply && isRepetition() || fifty >= 100)
        // return draw score
        return 0;

    // check if pv node
    int pvNode = beta - alpha > 1;

    // read hash if it exists and is not pv node
    if (ply && (score = probeHash(alpha, beta, depth, &bestMove)) != noHashEntry && pvNode == 0)
    {
        // return score of the move without search
        return score;
    }
    
    // check for input from gui or user every 2047 nodes
    if ((nodes & 2047) == 0)
    {
        // listen to inputs
        communicate();
    }

    // recursion escape
    if (depth == 0)
    {
        // run quiescence search
        return quiescence(alpha, beta);
    }

    // drop search if max ply
    if (ply > maxPly - 1)
    {
        // just evaluate position
        return evaluate();
    }

    // increment nodes
    nodes++;

    // incheck?
    int inCheck = isUnderAttack((side == white) ? getLSFBIndex(bitboards[K]) : 
                                                        getLSFBIndex(bitboards[k]),
                                                        side ^ 1);
    
    // if in check, increase depth
    if (inCheck)
    {
        depth++;
    }

    // legal moves counter
    int legalMoves = 0;

    // get static eval
    int eval = evaluate();

    /*
        skips moves if material balance plus gain of the move and safety margin
        does not improve alpha
    */
    // reverse futility pruning (RFP or static null move pruning)
	if (depth < 3 && !pvNode && !inCheck &&  abs(beta - 1) > -infinity + 100)
	{   
        // define evaluation margin
		int evalMargin = 120 * depth;
		
		// evaluation margin substracted from static evaluation score fails high
		if (eval - evalMargin >= beta)
		    // evaluation margin substracted from static evaluation score
			return eval - evalMargin;
	}

    // null move pruning
    if (depth >= 3 && inCheck == 0 && ply)
    {
        // copy board
        copyBoard();

        // increment ply to sync
        ply++; 

        // increment repetition & store hash
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        // update hash key
        if (enpassant != noSq)
        {
            hashKey ^= enpassantKeys[enpassant];
        }

        // reset enpassant square
        enpassant = noSq;

        // switch the side to give opponent a second move
        side ^= 1;

        // hash the side
        hashKey ^= sideKey;

        // search with less depth (depth - 1 - R, R=2)
        score = -negamax(-beta, -beta + 1, depth - 1 - 2);

        // bring back sync
        ply--;

        // decrease repetition & store hash
        repetitionIndex--;

        // restore board and variables
        undoBoard();

        // return 0 if time is up
        if (stopped == 1)
        {
            return 0;
        }

        // check fail hard beta cutoff
        if (score >= beta)
        {
            // node fails high
            return beta;
        }
    }

    /*
        If the static evaluation indicates a fail-low node, but q-search fails high, 
        the score of the reduced fail-high search is returned, since there was obviously 
        a winning capture raising the score
        from: https://www.chessprogramming.org/Razoring
    */
    // razoring (same idea as implemented in Strelka)
    if (!pvNode && !inCheck && depth <= 3)
    {
        // get static eval and add bonus
        score = eval + 125;
        
        // get new score
        int newScore;
        
        // static evaluation indicates a fail-low node
        if (score < beta)
        {
            // on depth 1
            if (depth == 1)
            {
                // get quiscence score
                newScore = quiescence(alpha, beta);
                
                // return quiescence score if it's greater then static evaluation score
                return (newScore > score) ? newScore : score;
            }
            
            // add second bonus to static evaluation
            score += 175;
            
            // static evaluation indicates a fail-low node
            if (score < beta && depth <= 2)
            {
                // get quiscence score
                newScore = quiescence(alpha, beta);
                
                // quiescence score indicates fail-low node
                if (newScore < beta)
                    // return quiescence score if it's greater then static evaluation score
                    return (newScore > score) ? newScore : score;
            }
        }
	}

    // create moves list
    moves moveList[1];

    // gen moves
    generateMoves(moveList);
    
    // if we are following pv line
    if (followPV)
    {
        // enable pv score
        enablePVScoring(moveList);
    }

    // sort moves mvvlva
    sortMoves(moveList, bestMove);

    // number of moves searched
    int movesSearched = 0;

    // loop over moves
    for (int count = 0; count < moveList->count; count++)
    {
        int move = moveList->moves[count];

        // copy the board
        copyBoard();

        // increment ply
        ply++;

        // increment repetition & store hash
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        // only make legal moves
        if (makeMove(moveList->moves[count], allMoves) == 0)
        {
            // decrease ply
            ply--;

            // decrease repetition & store hash
            repetitionIndex--;

            // skip
            continue;
        }

        // add legal move
        legalMoves++;

        // opponent in check flag (for lmr)
        int opponentInCheck = isUnderAttack((side == white) ? getLSFBIndex(bitboards[K]) : 
                                                        getLSFBIndex(bitboards[k]),
                                                        side ^ 1);
    
        // if no moves searched
        if (movesSearched == 0)
        {
            // do a normal search
            score = -negamax(-beta, -alpha, depth -1);
        }
        else
        {
            // condition to initiate late move reductions
            if (movesSearched >= fullDepthMoves && 
                        depth >= reductionLimit && 
                        inCheck == 0 && pvNode == 0)
            {
                // if move is promotion or capture, 
                if (getPromoted(move) || getCapture(move))
                {
                    // if moves give check, search w reduced depth of 2
                    if (opponentInCheck)
                    {
                        score = -negamax(-alpha - 1, -alpha, depth - 1 - 2);
                    }
                    else // if move does not give check, reduce depth by 3
                    {
                        score = -negamax(-alpha - 1, -alpha, depth - 1 - 3);
                    }
                }
                else // quiet move
                {

                    // search this move with reduced depth
                    double depthAdjustment = 0.7844 + std::log(depth) * std::log(moveNumber) / 2.4696;
                    // Ensure the resulting depth is an integer and not less than 1 (values taken from Ethereal)
                    int adjustedDepth = static_cast<int>(std::max(1.0, depth - 1 - depthAdjustment));
                    score = -negamax(-alpha - 1, -alpha, adjustedDepth);

                }
            }
            else // ensure full-depth search
            {
                score = alpha + 1;
            }
            // PVS (principal variation search)
            if (score > alpha)
            {
                // once you find a move with a score between alpha and beta
                // search for the rest of the moves trying to prove they are bad
                // this is faster than searching thinking the other moves are good
                score = -negamax(-alpha - 1, - alpha, depth - 1);

                // if it finds out it is wrong (there is a better move)
                // you have to search with normal alpha beta again
                // this is a waste of time but happens not that often so overall it's worth it
                if ((score > alpha) && (score < beta))
                {
                    // search normally again if move happened to be better than pv
                    score = -negamax(-beta, -alpha, depth-1);
                }
            }
        }

        // decrease ply
        ply--;

        // decrease repetition & store hash
        repetitionIndex--;

        // undo move
        undoBoard();

        // return 0 if time is up
        if (stopped == 1)
        {
            return 0;
        }

        // add move searched
        movesSearched++;

        // if found a better move
        if (score > alpha)
        {
            // switch hash flag to pv node
            hashFlag = hashFlagExact;
            
            // store best move
            bestMove = moveList->moves[count];

            // only store history moves on quiet moves
            if (getCapture(moveList->moves[count]) == 0)
            {
                // store history moves
                historyMoves[getPiece(moveList->moves[count])][getTarget(moveList->moves[count])] += depth;
            }
            // PV (principal variation) node
            alpha = score;

            // write PV move
            PVTable[ply][ply] = moveList->moves[count];

            // loop over next ply
            for (int nextPly = ply + 1; nextPly < PVLength[ply + 1]; nextPly++)
            {
                // copy move from deeper ply and add to current ply's line
                PVTable[ply][nextPly] = PVTable[ply + 1][nextPly];
            }

            // update PV length
            PVLength[ply] = PVLength[ply + 1];

            // fail-hard beta cutoff (score can't go outside of alpha beta bounds)
            if (score >= beta)
            {
                // store hash with beta flag and beta value
                recordHash(beta, depth, hashFlagBeta, moveList->moves[count], bestMove);

                // on quiet moves
                if (getCapture(moveList->moves[count]) == 0)
                {
                    // store killer moves
                    killerMoves[1][ply] = killerMoves[0][ply];
                    killerMoves[0][ply] = moveList->moves[count];
                }
                
                // node fails high 
                return beta;
            }
        }
    } 

    // no legal moves currently
    if (legalMoves == 0)
    {
        // king in check (chekmate)
        if (inCheck)
        {
            // return mating score (+ ply to find checkmates when using larger depths)
            return -mateValue + ply;
        }
        // king not in check (stalemate)
        else
        {
            // return stalemate score
            return 0;
        }
    }
    
    // store hash with alpha flag and alpha scoreb 
    recordHash(alpha, depth, hashFlag, move, bestMove);

    // node fails low
    return alpha;
}

// search position
void search(int depth)
{
    // create score
    int score = 0;

    // reset node counter
    nodes = 0;

    // reset no more time flag
    stopped = 0;

    // reset pv follow and pv scores
    followPV = 0;
    scorePV = 0;

    // clear tt
    clearTT();
    
    // clear helper data structures for search
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(PVTable, 0, sizeof(PVTable));
    memset(PVLength, 0, sizeof(PVLength));

    // give initial alpha and beta values
    int alpha = -infinity;
    int beta = infinity;

    // iterative deepening
    for (int currentDepth = 1; currentDepth <= depth; currentDepth++)
    {
        // no more time
        if (stopped == 1)
        {
            // stop calculating
            break;
        }
        
        // activate follow pv flag
        followPV = 1;

        // find best move
        score = negamax(alpha, beta, currentDepth);

        // we are outside of the window so we try again with a full window
        if ((score <= alpha) || (score >= beta))
        {
            alpha = -infinity;
            beta = infinity;
            continue;
        }

        // decrease window for next node
        alpha = score - 50;
        beta = score + 50;

        // print score/mate info
        // if pv length is not 0
        
        if (PVLength[0])
        {
            if (score > mateScore || score < -mateScore)
            {
                printf("info score mate %d ", (score > 0) ? ((mateValue-score)/2+1) : (-mateValue+(mateValue-score)/2));
            }
            else
            {
                printf("info score cp %d ", score);
            }
            
            // print depth, nodes, pv info
            printf("depth %d nodes %lld pv ", currentDepth, nodes);
            
            // loop over moves in pv line
            for (int count = 0; count < PVLength[0]; count++)
            {
                // print move (PVTable[0] has the principle variation)
                printMove(PVTable[0][count]);
                printf(" ");
            }
            printf("\n");
        }   
           
    }


    // best move placeholder
    printf("bestmove ");
    printMove(PVTable[0][0]);
    printf("\n");    
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                UCI Protocol
          Thanks to Richard Allbert
                   
<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

int moveOverhead = 300;

// parse user or GUI string input (like e4e5 or h7h8q for example)
int parseMove(const char* moveString)
{
    // create move list
    moves moveList[1];

    // generate moves
    generateMoves(moveList);

    // get source square (and convert to int the same way as before)
    int sourceSq = (moveString[0] - 'a') + (8 - (moveString[1] - '0')) * 8;

    // get target square
    int targetSq = (moveString[2] - 'a') + (8 - (moveString[3] - '0')) * 8;

    // loop over move list
    for (int moveCount = 0; moveCount < moveList->count; moveCount++)
    {
        // get current move
        int move = moveList->moves[moveCount];

        // check if source + target exist in move list
        if (sourceSq == getSource(move) && targetSq == getTarget(move))
        {
            // get promoted piece (if it exists)
            int promoted = getPromoted(move);

            // there is a promotion
            if (promoted)
            {
                // if promotes to queen
                if ((promoted == Q || promoted == q) && moveString[4] == 'q')
                {
                    return move;
                }
                // if promotes to rook
                else if ((promoted == R || promoted == r) && moveString[4] == 'r')
                {
                    return move;
                }
                // if promotes to knight
                else if ((promoted == N || promoted == n) && moveString[4] == 'n')
                {
                    return move;
                }
                // if promotes to bishop
                else if ((promoted == B || promoted == b) && moveString[4] == 'b')
                {
                    return move;
                }
                // in case illegal promotion (continue the loop)
                continue;
            }

            // if no promotion, return move
            return move;
        }
    }

    // move not in move list so illegal (or error with move gen)
    return 0;
}

/*
    UCI 'position' protocols that have to be accounted for
    
    start position
    position startpos
    
    start position + moves:
    position startpos moves e2e4 e7e5
    
    position from FEN string:
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 
    
    position from fen string + moves:
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 moves e2a6 e8g8
*/
// parse the UCI 'position' protocol
void parsePosition(const char* command)
{
    // shift pointer past 'position '
    command += 9;

    // current character that the pointer is at
    const char *currentChar = command;

    if (strncmp(command, "startpos", 8) == 0)
    {
        // if we find 'startpos', set up starting position
        parseFEN(start_position);
    }
    else // check if it is FEN protocol
    {
        // check for FEN
        currentChar = strstr(command, "fen");

        // if no FEN, 
        if (currentChar == NULL)
        {
            parseFEN(start_position);
        }
        // found FEN
        else
        {
            // move pointer to fen string
            currentChar += 4;
            
            // set up board with fen
            parseFEN(currentChar);
        }
    }

    // get moves if they exist
    currentChar = strstr(command, "moves");

    // if moves found
    if (currentChar != NULL)
    {
        // shift pointer until move
        currentChar += 6;

        // loop all moves in this move string
        while(*currentChar)
        {
            // parse the next move
            int move = parseMove(currentChar);

            // if no more moves
            if (move == 0)
            {
                break;
            }

            // increment repetition
            repetitionIndex++;

            // write hash into rep table
            repetitionTable[repetitionIndex] = hashKey;

            // make move on board
            makeMove(move, allMoves);

            // move pointer to the next move (until it reaches a space)
            while(*currentChar && *currentChar != ' ')
            {
                currentChar++;
            }

            // move pointer past the empty space
            currentChar++;
        }
    }
    // update attacks
    initAttacksTotal();

    // printboard()
}

// reset time variables
void resetTimeControl()
{
    // reset timing
    quit = 0;
    movestogo = 30;
    movetime = -1;
    time = -1;
    inc = 0;
    starttime = 0;
    stoptime = 0;
    timeset = 0;
    stopped = 0;
}

// parse the UCI 'go' protocol
void parseGo(const char *command)
{
    // reset variables
    resetTimeControl();

    // create depth variable
    int depth = -1;

    // create current depth pointer
    const char *argument = NULL;

    // infinite search
    if ((argument = strstr(command,"infinite"))) {}

    // match UCI "binc" command
    if ((argument = strstr(command,"binc")) && side == black)
    {
        // parse black time increment
        inc = atoi(argument + 5);
    }

    // match UCI "winc" command
    if ((argument = strstr(command,"winc")) && side == white)
    {
        // parse white time increment
        inc = atoi(argument + 5);
    }

    // match UCI "wtime" command
    if ((argument = strstr(command,"wtime")) && side == white)
    {
        // parse white time limit
        time = atoi(argument + 6);
    }

    // match UCI "btime" command
    if ((argument = strstr(command,"btime")) && side == black)
    {
        // parse black time limit
        time = atoi(argument + 6);
    }

    // match UCI "movestogo" command
    if ((argument = strstr(command,"movestogo")))
    {
        // parse number of moves to go
        movestogo = atoi(argument + 10);
    }

    // match UCI "movetime" command
    if ((argument = strstr(command,"movetime")))
    {
        // parse amount of time allowed to spend to make a move
        movetime = atoi(argument + 9);
    }

    // for fixed depth search
    if (argument = strstr(command, "depth"))
    {
        // move pointer to depth then convert string to integer and then update depth variable 
        depth = atoi(argument + 6);
    }
    
    // if there is no move time
    if(movetime != -1)
    {
        // set time equal to move time
        time = movetime;

        // set moves to go to 1
        movestogo = 1;
    }
    
    // start measuring time
    starttime = getTime();

    // get depth
    depth = depth;

    // if there is time control
    if(time != -1)
    {
        // flag of time control
        timeset = 1;

        // set up timing
        time /= movestogo;

        time -= moveOverhead;
        
        // "illegal" (empty) move bug fix
        if (time > 1500) time -= 100;

        // if time is up
        if (time < 0)
        {
            // restore negative time to 0
            time = 0;
            
            // inc lag compensation on 0+inc time controls
            inc -= 50;
            
            // timing for 0 seconds left and no inc
            if (inc < 0) inc = 1;
        }

        // get stop time
        stoptime = starttime + time + inc - 50;

        // treat increment as seconds per move when time is almost up
        if (time < 1500 && inc && depth == 64) stoptime = starttime + inc - 50;

    }

    // if depth is not available
    if(depth == -1)
    {
        // set depth to 64 plies (takes ages to complete...)
        depth = 64;
    }

        // print debug info
    //printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
    //time, starttime, stoptime, depth, timeset);

    // search position
    search(depth);
}

// UCI loop for communication
void uciLoop()
{
    // init max hash size and default mb
    int maxHash = 128;
    int mb = 64;

    // reset STDIN and STDOUT buffers
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    // define user or GUI input buffer (large number in case of large input)
    char input[2000];
    /*
    // print information
    printf("id name BetterThanCris 2.3\n");
    printf("id author Gustavo Knudsen\n");
    printf("uciok\n");
    fflush(stdout);
    */

    // loop
    while (1)
    {
        // reset input
        memset(input, 0, sizeof(input));

        // send output to gui
        fflush(stdout);

        // get input
        if (!fgets(input, 2000, stdin))
        {
            // continue the loop (returned NULL and no code should be executed)
            continue;
        }
        
        // trim newline character and null characters
        input[strcspn(input, "\n")] = 0;
        input[strcspn(input, "\x00")] = 0;

        // check 'isready' from uci
        if (strncmp(input, "isready", 7) == 0)
        {
            // answer with readyok to show that the engine is ready
            printf("readyok\n");
            fflush(stdout);
            continue;
        }

        // check position from uci
        else if (strncmp(input, "position", 8) == 0)
        {
            // parse position command
            parsePosition(input);

            // clear tt
            clearTT();
        }

        // check 'ucinewgame' command
        else if (strncmp(input, "ucinewgame", 10) == 0)
        {
            // clear tt
            clearTT();
            
            // call position with 'startpos'
            parsePosition("position startpos");
        }

        // check 'go' command
        else if (strncmp(input, "go", 2) == 0)
        {
            // parse go command
            parseGo(input);
        }

        // check UCI 'quit' command
        else if (strncmp(input, "quit", 4) == 0)
        {
            // exit (break)
            break;
        }

        // check uci command
        else if (strncmp(input, "uci", 3) == 0)
        {
            // print info 
            printf("id name BetterThanCrisV2.3\n");
            printf("id author Gustavo Knudsen\n");
            printf("uciok\n");
            printf("option name Move Overhead type spin default 10 min 0 max 5000\n");

            
            fflush(stdout);
        }        

        // hash value option
        else if (!strncmp(input, "setoption name Hash value ", 26))
        {
            // init MB
            sscanf(input, "%*s %*s %*s %*s %d", &mb);

            // adjust mb if beyond bounds
            if (mb < 4) mb = 4;
            if (mb > maxHash) mb = maxHash;

            // set tt table size
            printf("Set hash table size to %dMB\n", mb);
            fflush(stdout);
            initTT(mb);
        }
        // move overhead option
        else if (!strncmp(input, "setoption name Move Overhead value ", 34))
        {
            // init move overhead
            sscanf(input, "%*s %*s %*s %*s %d", &moveOverhead);

            // adjust move overhead if beyond reasonable bounds
            if (moveOverhead < 0) moveOverhead = 0;
            if (moveOverhead > 1000) moveOverhead = 1000;

            // set move overhead
            printf("Set move overhead to %dms\n", moveOverhead);
            fflush(stdout);
        }

        
    }
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Initialise All
                   Variables

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// initialise all necessary variables
void initAll()
{    
    // init leapers attacks
    initLeapersAttacks();

    // init slider attacks
    initSlidersAttacks(1);
    initSlidersAttacks(0);

    // init char pieces
    initCharPieces();

    // init piece promotion key
    initPromotedPieces();

    // init evaluation masks
    initEvalMasks();

    // init random keys (for hashing)
    initRandomKeys();

    // init transposition table with 12MB
    initTT(12);

    /*
    init magic numbers (naturally not on but can be used as well if 
    you want to generate them from scratch)
    */
    // initMagicNumbers();
    
}

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Main Function

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

int main()
{
    // init all variabkes
    initAll();

    clearTT();
    
    int debug = 0;

    // debugging purposes
    if (debug)
    {
        parseFEN(start_position);
        printBoard();

    }
    else
        // GUI
        uciLoop();

    return 0;
}