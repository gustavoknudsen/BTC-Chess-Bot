#ifndef EVAL_CONSTANTS_H
#define EVAL_CONSTANTS_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

           Evaluation Constants
            (except piece tables)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

/* Evaluation Constants (except piece tables) */

// default material scores
extern int defaultMaterialScore[12];

// material scores [stage][piece]
extern int materialScore[2][12];

// material adjustment based on pawn number
extern int knightAdj[9];
extern int rookAdj[9];

// double pawns penalty (values from old Stockfish, subject to change {mg, eg})
extern const int doublePawnPenalty[2];

// Early doubled pawns (additional penalty when no enemy pawns fixed)
extern const int doubledEarlyPenalty[2];

// Isolated pawn penalty
extern const int isolatedPawnPenalty[2];

// Backward pawn penalty
extern const int backwardPawnPenalty[2];

// Weak unopposed pawn penalty (for isolated/backward pawns with no enemy pawns ahead)
extern const int weakUnopposed[2];

// passed pawn rank bonus [stage][rank] (values from old Stockfish, subject to change)
extern const int passedPawnRankBonus[2][8];

// passed pawn passed file bonus [stage][file]
extern const int passedPawnFileBonus[2][8];

// Connected pawn bonus by rank
extern const int connectedPawnBonus[8];

extern const int blockedPawnBonus[2][2];

// semi open file score (values from old Stockfish, subject to change {mg, eg})
extern const int semiOpenFileScore[2];

// open file score (values from old Stockfish, subject to change {mg, eg})
extern const int openFileScore[2];

extern const int RookOnClosedFile[2];
extern const int RookOnOpenFile[2][2];

// mobility bonus (values from old Stockfish, subject to change {mg, eg})
// [piece][# of attacked squares != friendly pieces][stage]
extern const int mobilityBonus[6][32][2];

// attacking king zone attack weight table [piece number]
extern const int attackWeight[8];

// king attack weight by attacker piece type, indexed by piece enum {P,N,B,R,Q,K}
extern const int KingAttackWeights[6];

// safe-check bonus added to kingDanger, [attacker piece type][single=0/multiple=1]
extern const int SafeCheck[6][2];

// tempo bonus
extern const int tempoBonus;

// shelter strength bonuses
extern int ShelterStrength[4][7];

// unblocked storm bonuses
extern int UnblockedStorm[4][7];

extern const int OutpostBonusKnight[2];
extern const int UncontestedOutpost[2];
extern const int BishopOnKingRing[2];
extern const int BishopXRayPawns[2];
extern const int FlankAttacks[2];
extern const int Hanging[2];
extern const int KnightOnQueen[2];
extern const int LongDiagonalBishop[2];
extern const int MinorBehindPawn[2];
extern const int PassedFile[2];
extern const int PawnlessFlank[2];
extern const int ReachableOutpost[2];
extern const int RestrictedPiece[2];
extern const int RookOnKingRing[2];
extern const int SliderOnQueen[2];
extern const int ThreatByKing[2];
extern const int ThreatByPawnPush[2];
extern const int ThreatBySafePawn[2];
extern const int TrappedRook[2];
extern const int WeakQueenProtection[2];
extern const int WeakQueen[2];
extern const int KingProtectorKnight[2];
extern const int OutpostBonusBishop[2];  // Outpost[1] for bishop
extern const int KingProtectorBishop[2];  // KingProtector[1] for bishop
extern const int BishopPairBonus[2];
extern const int WeakLever[2];
extern const int BishopPawnsPenalty[4][2];
extern const int CorneredBishop;      // Penalty for cornered bishop in Chess960

extern const int ThreatByMinor[6][2];

extern const int ThreatByRook[6][2];

#endif // EVAL_CONSTANTS_H
