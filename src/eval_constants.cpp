#include "eval_constants.h"

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
    126,
    781,
    825,
    1276,
    2538,
    10000,
    -126,
    -781,
    -825,
    -1276,
    -2538,
    -10000,
},
{
    208,
    854,
    915,
    1380,
    2682,
    10000,
    -208,
    -854,
    -915,
    -1380,
    -2682,
    -10000,
}
};

// material adjustment based on pawn number
int knightAdj[9] = { -31, -25, -18, -12, -6,  0,  6, 12, 18 };
int rookAdj[9] = { 23,  18,  14,  9,  5,  0, -5, -9, -14 };

// double pawns penalty (values from old Stockfish, subject to change {mg, eg})
int doublePawnPenalty[2] = {-11, -51};

// Early doubled pawns (additional penalty when no enemy pawns fixed)
int doubledEarlyPenalty[2] = {-17, -7};

// Isolated pawn penalty
int isolatedPawnPenalty[2] = {-1, -20};

// Backward pawn penalty
int backwardPawnPenalty[2] = {-6, -19};

// Weak unopposed pawn penalty (for isolated/backward pawns with no enemy pawns ahead)
int weakUnopposed[2] = {-15, -18};

// passed pawn rank bonus [stage][rank] (values from old Stockfish, subject to change)
int passedPawnRankBonus[2][8] =
{
    {
        0, 0, 5, 12, 10, 57, 163, 271
    },
    {
        0, 0, 18, 23, 31, 62, 167, 250
    }
};

// passed pawn passed file bonus [stage][file]
int passedPawnFileBonus[2][8] =
{
    {
        -1, 0, -9, -30, -30, -9, 0, -1
    },
    {
        7, 9, -8, -14, -14, -8, 9, 7
    }
};

// Connected pawn bonus by rank
int connectedPawnBonus[8] = {0, 3, 7, 7, 15, 54, 86, 0};

int blockedPawnBonus[2][2] = {
    {19, 8},    // Rank 5
    {7, -3}     // Rank 6
};

// semi open file score (values from old Stockfish, subject to change {mg, eg})
int semiOpenFileScore[2] = {18, 7};

// open file score (values from old Stockfish, subject to change {mg, eg})
int openFileScore[2] = {44, 20};

int RookOnClosedFile[2] = { 10, 5 };
int RookOnOpenFile[2][2] = {
    { 18, 8 },   // Semi-open file
    { 49, 26 }   // Fully open file
};

// mobility bonus (values from old Stockfish, subject to change {mg, eg})
// [piece][# of attacked squares != friendly pieces][stage]
int mobilityBonus[6][32][2] =
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
        {-62, -79}, {-53, -57}, {-12, -31}, {-3, -17}, {3, 7}, {12, 13},
        {21, 16}, {28, 21}, {37, 26}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
    // bishops
    {
        {-47, -59}, {-20, -25}, {14, -8}, {29, 12}, {39, 21}, {53, 40},
        {53, 56}, {60, 58}, {62, 65}, {69, 72}, {78, 78}, {83, 87},
        {91, 88}, {96, 98}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
    // rooks
    {
        {-60, -82}, {-24, -15}, {0, 17}, {3, 43}, {4, 72}, {14, 100},
        {20, 102}, {30, 122}, {41, 133}, {41, 139}, {41, 153}, {45, 160},
        {57, 165}, {58, 170}, {67, 175}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
    // queens
    {
        {-29, -49}, {-16, -29}, {-8, -8}, {-8, 17}, {18, 39}, {25, 54},
        {23, 59}, {37, 73}, {41, 76}, {54, 95}, {65, 95}, {68, 101},
        {69, 124}, {70, 128}, {70, 132}, {70, 133}, {71, 136}, {72, 140},
        {74, 147}, {76, 149}, {90, 153}, {104, 169}, {105, 171}, {106, 171},
        {112, 178}, {114, 185}, {114, 187}, {119, 221}, {0, 0}, {0, 0},
        {0, 0}, {0, 0}
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
int attackWeight[8] = {0, 0, 50, 75, 88, 94, 97, 99};

// indexed by BTC piece enum {P,N,B,R,Q,K}; values from old Stockfish KingAttackWeights
int KingAttackWeights[6] = { 0, 76, 46, 45, 14, 0 };

// safe-check bonus by attacker type {P,N,B,R,Q,K} x {single, multiple}, from old Stockfish SafeCheck
int SafeCheck[6][2] = {
    {   0,    0 },
    { 805, 1292 },  // knight
    { 650,  984 },  // bishop
    {1071, 1886 },  // rook
    { 730, 1128 },  // queen
    {   0,    0 }
};

// king danger contribution weights (old-Stockfish starting values; Texel-tuned in BTC units)
int kdWeakRing      = 183;
int kdUnsafeCheck   = 148;
int kdBlocker       = 98;
int kdKingAttacks   = 69;
int kdFlankAttack   = 3;
int kdNoQueen       = 873;
int kdKnightDefense = 100;
int kdShelter       = 6;
int kdFlankDefense  = 4;
int kdInit          = 37;

// tempo bonus
int tempoBonus = 28;

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

int OutpostBonusKnight[2]      = { 54, 34 };
int UncontestedOutpost[2]      = {  0, 10 };
int BishopOnKingRing[2]        = { 24,  0 };
int BishopXRayPawns[2]         = {  4,  5 };
int FlankAttacks[2]            = {  8,  0 };
int Hanging[2]                 = { 72, 40 };
int KnightOnQueen[2]           = { 16, 11 };
int LongDiagonalBishop[2]      = { 45,  0 };
int MinorBehindPawn[2]         = { 18,  3 };
int PassedFile[2]              = { 13,  8 };
int PawnlessFlank[2]           = { 19, 97 };
int ReachableOutpost[2]        = { 33, 19 };
int RestrictedPiece[2]         = {  6,  7 };
int RookOnKingRing[2]          = { 16,  0 };
int SliderOnQueen[2]           = { 62, 21 };
int ThreatByKing[2]            = { 24, 87 };
int ThreatByPawnPush[2]        = { 48, 39 };
int ThreatBySafePawn[2]        = {167, 99 };
int TrappedRook[2]             = { 55, 13 };
int WeakQueenProtection[2]     = { 14,  0 };
int WeakQueen[2]               = { 57, 19 };
int KingProtectorKnight[2]     = {  9,  9 };
int OutpostBonusBishop[2]      = { 31, 25 };  // Outpost[1] for bishop
int KingProtectorBishop[2]     = {  7,  9 };  // KingProtector[1] for bishop
int BishopPairBonus[2]         = { 25, 50 };
int WeakLever[2]               = { -2, -57};
int BishopPawnsPenalty[4][2]   = {
    { 3, 8 }, // Edge distance 0 (a/h files)
    { 3, 9 }, // Edge distance 1 (b/g files)
    { 2, 7 }, // Edge distance 2 (c/f files)
    { 3, 7 }  // Edge distance 3 (d/e files)
};
int CorneredBishop             = 50;      // Penalty for cornered bishop in Chess960

int ThreatByMinor[6][2] = {
    {  0,   0 },
    {  6,  37 },
    { 64,  50 },
    { 82,  57 },
    {103, 130 },
    { 81, 163 }
};

int ThreatByRook[6][2] = {
    {  0,  0 },
    { 54, 42 },
    { 56, 43 },
    { 66, 44 },
    { 86, 60 },
    {  0,  0 }
};
