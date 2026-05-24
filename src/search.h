#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"
#include "move.h"
#include <stdint.h>

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                    Search

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// Most Valuable Victim - Least Valuable Attacker [attacker][victim]
extern int mvvlva[12][12];

// 2 killer moves [id][ply]
extern int killerMoves[2][maxPly];

// gravity main history [piece][to]
// signed score in [-HIST_MAX, +HIST_MAX], bonus on cutoff, malus on
// previously-searched non-best quiets. used for quiet move ordering.
extern int16_t mainHistory[12][64];

// gravity capture history [piece][to][captured piece]
// scored the same way as mainHistory but for captures, used to refine
// MVV/LVA ordering across captures of the same piece.
extern int16_t captureHistory[12][64][12];

// 1-ply continuation history [prevPiece][prevTo][piece][to]
// rewards/punishes a move conditioned on the previous move played.
extern int16_t continuationHistory[12][64][12][64];

// stack of moves played to reach each ply, used to index continuation history
extern int playedMoveStack[maxPly + 1];

// PV length [ply]
extern int PVLength[maxPly];

// PV table [ply][ply]
extern int PVTable[maxPly][maxPly];

// follow PV and score PV (if follow = 1, follow pv, 0 we don't)
extern int followPV, scorePV;

extern const int fullDepthMoves;
extern const int reductionLimit;

// reset all search-side history tables. called from ucinewgame and at engine
// init. NOT called per search so history carries across moves in a game.
void clearSearchHeuristics();

// search position
void search(int depth);

// print move scores
void printMoveScores(moves *moveList);

#endif // SEARCH_H
