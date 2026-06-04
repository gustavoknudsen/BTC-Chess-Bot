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
extern int doublePawnPenalty[2];

// Early doubled pawns (additional penalty when no enemy pawns fixed)
extern int doubledEarlyPenalty[2];

// Isolated pawn penalty
extern int isolatedPawnPenalty[2];

// Backward pawn penalty
extern int backwardPawnPenalty[2];

// Weak unopposed pawn penalty (for isolated/backward pawns with no enemy pawns ahead)
extern int weakUnopposed[2];

// passed pawn rank bonus [stage][rank] (values from old Stockfish, subject to change)
extern int passedPawnRankBonus[2][8];

// passed pawn passed file bonus [stage][file]
extern int passedPawnFileBonus[2][8];

// Connected pawn bonus by rank
extern int connectedPawnBonus[8];

extern int blockedPawnBonus[2][2];

// semi open file score (values from old Stockfish, subject to change {mg, eg})
extern int semiOpenFileScore[2];

// open file score (values from old Stockfish, subject to change {mg, eg})
extern int openFileScore[2];

extern int RookOnClosedFile[2];
extern int RookOnOpenFile[2][2];

// mobility bonus (values from old Stockfish, subject to change {mg, eg})
// [piece][# of attacked squares != friendly pieces][stage]
extern int mobilityBonus[6][32][2];

// attacking king zone attack weight table [piece number]
extern int attackWeight[8];

// king attack weight by attacker piece type, indexed by piece enum {P,N,B,R,Q,K}
extern int KingAttackWeights[6];

// safe-check bonus added to kingDanger, [attacker piece type][single=0/multiple=1]
extern int SafeCheck[6][2];

// king danger contribution weights (see getKingDanger); tuned in BTC units
extern int kdWeakRing;      // per weak square in the king ring
extern int kdUnsafeCheck;   // per unsafe check square
extern int kdBlocker;       // per piece blocking a slider attack on our king
extern int kdKingAttacks;   // per attack on the king zone
extern int kdFlankAttack;   // coefficient of kingFlankAttack^2 / 8
extern int kdNoQueen;       // bonus (subtracted) when the enemy has no queen
extern int kdKnightDefense; // bonus when a knight defends the king
extern int kdShelter;       // coefficient of shelter mg / 8 (subtracted)
extern int kdFlankDefense;  // per defended king-flank square (subtracted)
extern int kdInit;          // constant offset

// tempo bonus
extern int tempoBonus;

// shelter strength bonuses
extern int ShelterStrength[4][7];

// unblocked storm bonuses
extern int UnblockedStorm[4][7];

extern int OutpostBonusKnight[2];
extern int UncontestedOutpost[2];
extern int BishopOnKingRing[2];
extern int BishopXRayPawns[2];
extern int FlankAttacks[2];
extern int Hanging[2];
extern int KnightOnQueen[2];
extern int LongDiagonalBishop[2];
extern int MinorBehindPawn[2];
extern int PassedFile[2];
extern int PawnlessFlank[2];
extern int ReachableOutpost[2];
extern int RestrictedPiece[2];
extern int RookOnKingRing[2];
extern int SliderOnQueen[2];
extern int ThreatByKing[2];
extern int ThreatByPawnPush[2];
extern int ThreatBySafePawn[2];
extern int TrappedRook[2];
extern int WeakQueenProtection[2];
extern int WeakQueen[2];
extern int KingProtectorKnight[2];
extern int OutpostBonusBishop[2];  // Outpost[1] for bishop
extern int KingProtectorBishop[2];  // KingProtector[1] for bishop
extern int BishopPairBonus[2];
extern int WeakLever[2];
extern int BishopPawnsPenalty[4][2];
extern int CorneredBishop;      // Penalty for cornered bishop in Chess960

extern int ThreatByMinor[6][2];

extern int ThreatByRook[6][2];

#endif // EVAL_CONSTANTS_H
