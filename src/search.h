#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"
#include "move.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                    Search

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// Most Valuable Victim - Least Valuable Attacker [attacker][victim]
extern int mvvlva[12][12];

// 2 killer moves [id][ply]
extern int killerMoves[2][maxPly];

// history moves
extern int historyMoves[12][64];

// PV length [ply]
extern int PVLength[maxPly];

// PV table [ply][ply]
extern int PVTable[maxPly][maxPly];

// follow PV and score PV (if follow = 1, follow pv, 0 we don't)
extern int followPV, scorePV;

extern const int fullDepthMoves;
extern const int reductionLimit;

// search position
void search(int depth);

// print move scores
void printMoveScores(moves *moveList);

#endif // SEARCH_H
