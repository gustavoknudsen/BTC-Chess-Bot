#include "search.h"
#include "bitboard.h"
#include "position.h"
#include "move.h"
#include "movegen.h"
#include "make_move.h"
#include "copy_make.h"
#include "evaluation.h"
#include "tt.h"
#include "timeman.h"
#include "see.h"

#include <stdint.h>
#include <stdlib.h>

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
int mvvlva[12][12] = {
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

// 2 killer moves [id][ply]
int killerMoves[2][maxPly];

// gravity-bounded history tables. carry across moves within a game; reset
// only on ucinewgame via clearSearchHeuristics.
int16_t mainHistory[12][64];
int16_t captureHistory[12][64][12];
int16_t continuationHistory[12][64][12][64];

// move that led to each ply. playedMoveStack[ply] is the move just made to
// reach the position at search ply == ply. used for continuation history.
int playedMoveStack[maxPly + 1];

// PV length [ply]
int PVLength[maxPly];

// PV table [ply][ply]
int PVTable[maxPly][maxPly];

// follow PV and score PV (if follow = 1, follow pv, 0 we don't)
int followPV, scorePV;

// gravity history bound. updates self-decay toward zero as |entry| grows.
// formula: entry += bonus - entry * |bonus| / HIST_MAX
#define HIST_MAX 8192

// move-score buckets, chosen so each tier sorts cleanly above the next
// without colliding even after history scores are added in.
#define SCORE_TT_BEST       10000000
#define SCORE_PV_FOLLOW     9000000
#define SCORE_GOOD_CAPTURE  1000000
#define SCORE_KILLER_1      800000
#define SCORE_KILLER_2      700000
#define SCORE_BAD_CAPTURE   (-1000000)

// reset all search-side heuristics. called at engine init and on ucinewgame.
void clearSearchHeuristics()
{
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(mainHistory, 0, sizeof(mainHistory));
    memset(captureHistory, 0, sizeof(captureHistory));
    memset(continuationHistory, 0, sizeof(continuationHistory));
    memset(playedMoveStack, 0, sizeof(playedMoveStack));
}

// gravity update on a 16-bit history entry, bounded to [-HIST_MAX, HIST_MAX]
static inline void updateHistoryEntry(int16_t *entry, int bonus)
{
    // clamp incoming bonus so a single update cannot exceed the bound
    if (bonus >  HIST_MAX) bonus =  HIST_MAX;
    if (bonus < -HIST_MAX) bonus = -HIST_MAX;
    int e = *entry;
    e += bonus - e * abs(bonus) / HIST_MAX;
    *entry = (int16_t)e;
}

// depth-scaled history bonus. quadratic in depth, clamped.
static inline int historyBonus(int depth)
{
    int b = 16 * depth * depth + 32 * depth - 16;
    if (b > 1200) b = 1200;
    if (b < 0) b = 0;
    return b;
}

// find the enemy piece captured on a move's target square. returns -1 if the
// to-square is empty (e.g., en passant or quiet move). used to index capture
// history. for en passant we return a pawn so capture history still indexes
// something sensible.
static inline int getCapturedPiece(int move)
{
    if (getEnpassant(move))
        return (side == white) ? p : P;

    int to = getTarget(move);
    int startEnemy = (side == white) ? p : P;
    int endEnemy   = (side == white) ? k : K;
    for (int bp = startEnemy; bp <= endEnemy; bp++)
    {
        if (getBit(bitboards[bp], to))
            return bp;
    }
    return -1;
}

// 1-ply continuation history lookup. returns 0 at the root (no previous move).
static inline int getContHist(int currentMove)
{
    if (ply <= 0) return 0;
    int prev = playedMoveStack[ply];
    if (prev == 0) return 0;
    return continuationHistory[getPiece(prev)][getTarget(prev)]
                              [getPiece(currentMove)][getTarget(currentMove)];
}

// update continuation history for the current move at ply
static inline void updateContHist(int currentMove, int bonus)
{
    if (ply <= 0) return;
    int prev = playedMoveStack[ply];
    if (prev == 0) return;
    updateHistoryEntry(&continuationHistory[getPiece(prev)][getTarget(prev)]
                                            [getPiece(currentMove)][getTarget(currentMove)],
                       bonus);
}

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

    1. TT best move
    2. PV move (when following pv from prior iteration)
    3. Good captures (SEE >= 0)
       scored by MVV/LVA + scaled capture history
    4. Killer move 1
    5. Killer move 2
    6. Quiet moves
       scored by main history + 1-ply continuation history
    7. Bad captures (SEE < 0)
       scored by MVV/LVA, kept below quiets
*/

// score move function (for move ordering). does NOT include the TT best move
// bonus; that is layered on by sortMoves.
static inline int scoreMove(int move)
{
    if (scorePV)
    {
        if (PVTable[0][ply] == move)
        {
            scorePV = 0;
            return SCORE_PV_FOLLOW;
        }
    }

    if (getCapture(move))
    {
        // identify the captured piece for MVV/LVA and capture history index
        int captured;
        if (getEnpassant(move))
            captured = (side == white) ? p : P;
        else
        {
            captured = (side == white) ? p : P;
            int startEnemy = (side == white) ? p : P;
            int endEnemy   = (side == white) ? k : K;
            for (int bp = startEnemy; bp <= endEnemy; bp++)
            {
                if (getBit(bitboards[bp], getTarget(move)))
                {
                    captured = bp;
                    break;
                }
            }
        }

        int mvvlvaScore = mvvlva[getPiece(move)][captured];
        int capHist = captureHistory[getPiece(move)][getTarget(move)][captured];

        // SEE classifies into good vs bad captures
        if (seeGe(move, 0))
            return SCORE_GOOD_CAPTURE + mvvlvaScore * 100 + capHist;
        else
            return SCORE_BAD_CAPTURE + mvvlvaScore * 100 + capHist;
    }

    // quiet move
    if (killerMoves[0][ply] == move)
        return SCORE_KILLER_1;
    if (killerMoves[1][ply] == move)
        return SCORE_KILLER_2;

    // history-based scoring for quiet moves
    return mainHistory[getPiece(move)][getTarget(move)] + getContHist(move);
}

// sort moves (for better pruning). TT best move is bumped to top.
static inline void sortMoves(moves *moveList, int best)
{
    int moveScores[256];

    for (int count = 0; count < moveList->count; count++)
    {
        if (best != 0 && best == moveList->moves[count])
            moveScores[count] = SCORE_TT_BEST;
        else
            moveScores[count] = scoreMove(moveList->moves[count]);
    }

    // selection-style descending sort (small lists, fine; still O(n^2))
    for (int current = 0; current < moveList->count; current++)
    {
        for (int next = current + 1; next < moveList->count; next++)
        {
            if (moveScores[current] < moveScores[next])
            {
                int tempScore = moveScores[current];
                moveScores[current] = moveScores[next];
                moveScores[next] = tempScore;

                int tempMove = moveList->moves[current];
                moveList->moves[current] = moveList->moves[next];
                moveList->moves[next] = tempMove;
            }
        }
    }
}

// score move list using a captures-only ordering (MVV/LVA + capture history),
// with bad SEE captures sent to the bottom. used only inside quiescence.
static inline void sortCaptures(moves *moveList)
{
    int moveScores[256];

    for (int count = 0; count < moveList->count; count++)
    {
        int move = moveList->moves[count];

        int captured = (side == white) ? p : P;
        if (getEnpassant(move))
        {
            captured = (side == white) ? p : P;
        }
        else
        {
            int startEnemy = (side == white) ? p : P;
            int endEnemy   = (side == white) ? k : K;
            for (int bp = startEnemy; bp <= endEnemy; bp++)
            {
                if (getBit(bitboards[bp], getTarget(move)))
                {
                    captured = bp;
                    break;
                }
            }
        }

        int mvvlvaScore = mvvlva[getPiece(move)][captured];
        int capHist = captureHistory[getPiece(move)][getTarget(move)][captured];

        if (seeGe(move, 0))
            moveScores[count] = SCORE_GOOD_CAPTURE + mvvlvaScore * 100 + capHist;
        else
            moveScores[count] = SCORE_BAD_CAPTURE + mvvlvaScore * 100 + capHist;
    }

    for (int current = 0; current < moveList->count; current++)
    {
        for (int next = current + 1; next < moveList->count; next++)
        {
            if (moveScores[current] < moveScores[next])
            {
                int tempScore = moveScores[current];
                moveScores[current] = moveScores[next];
                moveScores[next] = tempScore;

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
    for (int index = 0; index < repetitionIndex; index++)
    {
        if (repetitionTable[index] == hashKey)
            return 1;
    }
    return 0;
}

// quiescence search (to stop the horizon effect).
// uses captures-only movegen, MVV/LVA + SEE ordering, and skips losing
// captures (SEE < 0) below the stand-pat.
static inline int quiescence(int alpha, int beta)
{
    if ((nodes & 2047) == 0)
        communicate();

    nodes++;

    if (ply > maxPly - 1)
        return evaluate();

    int eval = evaluate();

    // stand-pat
    if (eval >= beta)
        return beta;
    if (eval > alpha)
        alpha = eval;

    moves moveList[1];
    generateCaptures(moveList);
    sortCaptures(moveList);

    for (int count = 0; count < moveList->count; count++)
    {
        int move = moveList->moves[count];

        // SEE pruning: drop captures the static exchange says are losing.
        // promotions/en passant pass through (seeGe returns true at thr=0).
        if (!seeGe(move, 0))
            continue;

        copyBoard();

        ply++;
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        if (makeMove(move, allMoves) == 0)
        {
            ply--;
            repetitionIndex--;
            continue;
        }

        playedMoveStack[ply] = move;

        int score = -quiescence(-beta, -alpha);

        ply--;
        repetitionIndex--;

        undoBoard();

        if (stopped == 1)
            return 0;

        if (score > alpha)
        {
            alpha = score;
            if (score >= beta)
                return beta;
        }
    }

    return alpha;
}

extern const int fullDepthMoves = 5;
extern const int reductionLimit = 2;

// contempt in centipawns. positive means both sides slightly prefer to play
// on rather than draw. the draw is scored as -DRAW_CONTEMPT from the side-to-
// move's perspective, so any continuation worth more than -DRAW_CONTEMPT
// (which includes nearly all positions where we are not already losing
// significantly) is preferred to the draw. when actually losing, the engine
// still takes the draw (since the draw score -10 beats e.g. a -300 line).
#define DRAW_CONTEMPT 30

// negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth)
{
    PVLength[ply] = ply;

    int score = 0;
    int bestMove = 0;
    int hashFlag = hashFlagAlpha;

    int pvNode = beta - alpha > 1;

    // repetition / fifty-move draws (gated on ply so root always returns a move).
    // instead of a plain return 0, we return a contempt-adjusted draw score so
    // both sides prefer to keep playing in slightly favourable positions.
    if (ply && (isRepetition() || fifty >= 100))
    {
        int drawScore = -DRAW_CONTEMPT;

        if (!pvNode)
        {
            // in non-pv nodes we can fail-cutoff against the draw score
            if (drawScore <= alpha) return alpha;
            if (drawScore >= beta)  return beta;
            // otherwise tighten alpha but keep searching (rare path; depth
            // was zero at the recurse so the search returns from the leaf
            // before any further moves anyway)
            alpha = drawScore;
        }
        else
        {
            // in pv nodes only tighten alpha if the draw improves on it
            if (drawScore > alpha)
            {
                alpha = drawScore;
                if (alpha >= beta) return beta;
            }
        }
    }

    // mate distance pruning
    if (ply)
    {
        if (alpha < -mateValue + ply) alpha = -mateValue + ply;
        if (beta > mateValue - ply - 1) beta = mateValue - ply - 1;
        if (alpha >= beta) return alpha;
    }

    // always probe TT so bestMove gets populated for move ordering
    score = probeHash(alpha, beta, depth, &bestMove);
    if (ply && pvNode == 0 && score != noHashEntry)
        return score;

    if ((nodes & 2047) == 0)
        communicate();

    if (depth == 0)
        return quiescence(alpha, beta);

    if (ply > maxPly - 1)
        return evaluate();

    nodes++;

    int inCheck = isUnderAttack((side == white) ? getLSFBIndex(bitboards[K]) :
                                                        getLSFBIndex(bitboards[k]),
                                                        side ^ 1);

    if (inCheck)
        depth++;

    int legalMoves = 0;
    int eval = evaluate();

    // reverse futility pruning
	if (depth < 3 && !pvNode && !inCheck && abs(beta) < mateScore)
	{
		int evalMargin = 120 * depth;
		if (eval - evalMargin >= beta)
			return eval - evalMargin;
	}

    // null move pruning
    if (depth >= 3 && inCheck == 0 && ply)
    {
        copyBoard();

        ply++;
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        if (enpassant != noSq)
            hashKey ^= enpassantKeys[enpassant];
        enpassant = noSq;

        side ^= 1;
        hashKey ^= sideKey;

        // null move advances ply; record a null prev-move so continuation
        // history does not get fed garbage on the next ply
        playedMoveStack[ply] = 0;

        score = -negamax(-beta, -beta + 1, depth - 1 - 2);

        ply--;
        repetitionIndex--;

        undoBoard();

        if (stopped == 1)
            return 0;

        if (score >= beta)
            return beta;
    }

    // razoring (Strelka-style)
    if (!pvNode && !inCheck && depth <= 3)
    {
        score = eval + 125;
        int newScore;

        if (score < beta)
        {
            if (depth == 1)
            {
                newScore = quiescence(alpha, beta);
                return (newScore > score) ? newScore : score;
            }

            score += 175;

            if (score < beta && depth <= 2)
            {
                newScore = quiescence(alpha, beta);
                if (newScore < beta)
                    return (newScore > score) ? newScore : score;
            }
        }
	}

    moves moveList[1];
    generateMoves(moveList);

    if (followPV)
        enablePVScoring(moveList);

    sortMoves(moveList, bestMove);

    int movesSearched = 0;

    // remember the quiets and captures we tried so we can apply malus on cutoff
    int quietsTried[256];
    int quietsCount = 0;
    int capturesTried[256];
    int capturesCount = 0;

    for (int count = 0; count < moveList->count; count++)
    {
        int move = moveList->moves[count];

        copyBoard();

        ply++;
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        if (makeMove(moveList->moves[count], allMoves) == 0)
        {
            ply--;
            repetitionIndex--;
            continue;
        }

        playedMoveStack[ply] = move;

        legalMoves++;

        int opponentInCheck = isUnderAttack((side == white) ? getLSFBIndex(bitboards[K]) :
                                                        getLSFBIndex(bitboards[k]),
                                                        side ^ 1);

        if (movesSearched == 0)
        {
            score = -negamax(-beta, -alpha, depth - 1);
        }
        else
        {
            if (movesSearched >= fullDepthMoves &&
                        depth >= reductionLimit &&
                        inCheck == 0 && pvNode == 0)
            {
                if (getPromoted(move) || getCapture(move))
                {
                    if (opponentInCheck)
                        score = -negamax(-alpha - 1, -alpha, depth - 1 - 2);
                    else
                        score = -negamax(-alpha - 1, -alpha, depth - 1 - 3);
                }
                else
                {
                    // Ethereal-style LMR formula
                    double depthAdjustment = 0.7844 + std::log(depth) * std::log(movesSearched) / 2.4696;
                    int adjustedDepth = static_cast<int>(std::max(1.0, depth - 1 - depthAdjustment));
                    score = -negamax(-alpha - 1, -alpha, adjustedDepth);
                }
            }
            else
            {
                score = alpha + 1;
            }

            // PVS re-search
            if (score > alpha)
            {
                score = -negamax(-alpha - 1, -alpha, depth - 1);

                if ((score > alpha) && (score < beta))
                    score = -negamax(-beta, -alpha, depth - 1);
            }
        }

        ply--;
        repetitionIndex--;

        undoBoard();

        if (stopped == 1)
            return 0;

        // bookkeep the move for cutoff bonus/malus
        if (getCapture(move))
        {
            if (capturesCount < 256) capturesTried[capturesCount++] = move;
        }
        else
        {
            if (quietsCount < 256) quietsTried[quietsCount++] = move;
        }

        movesSearched++;

        if (score > alpha)
        {
            hashFlag = hashFlagExact;
            bestMove = move;

            alpha = score;

            PVTable[ply][ply] = move;
            for (int nextPly = ply + 1; nextPly < PVLength[ply + 1]; nextPly++)
                PVTable[ply][nextPly] = PVTable[ply + 1][nextPly];
            PVLength[ply] = PVLength[ply + 1];

            if (score >= beta)
            {
                recordHash(beta, depth, hashFlagBeta, move, bestMove);

                int bonus = historyBonus(depth);

                if (getCapture(move))
                {
                    // capture cutoff: bonus to its capture history, malus to
                    // earlier non-best captures
                    int captured = getCapturedPiece(move);
                    if (captured >= 0)
                        updateHistoryEntry(&captureHistory[getPiece(move)][getTarget(move)][captured], bonus);

                    for (int i = 0; i < capturesCount - 1; i++)
                    {
                        int badMove = capturesTried[i];
                        int badCap = getCapturedPiece(badMove);
                        if (badCap >= 0)
                            updateHistoryEntry(&captureHistory[getPiece(badMove)][getTarget(badMove)][badCap], -bonus);
                    }
                }
                else
                {
                    // quiet cutoff: store killer, bonus to main + continuation
                    // history, malus to earlier non-best quiets
                    if (killerMoves[0][ply] != move)
                    {
                        killerMoves[1][ply] = killerMoves[0][ply];
                        killerMoves[0][ply] = move;
                    }

                    updateHistoryEntry(&mainHistory[getPiece(move)][getTarget(move)], bonus);
                    updateContHist(move, bonus);

                    for (int i = 0; i < quietsCount - 1; i++)
                    {
                        int badMove = quietsTried[i];
                        updateHistoryEntry(&mainHistory[getPiece(badMove)][getTarget(badMove)], -bonus);
                        updateContHist(badMove, -bonus);
                    }
                }

                return beta;
            }
        }
    }

    // checkmate / stalemate
    if (legalMoves == 0)
    {
        if (inCheck)
            return -mateValue + ply;
        else
            return 0;
    }

    recordHash(alpha, depth, hashFlag, bestMove, bestMove);

    return alpha;
}

// search position
void search(int depth)
{
    int score = 0;

    nodes = 0;
    stopped = 0;

    followPV = 0;
    scorePV = 0;

    // killers and PV are ply-indexed; reset per-search. histories persist
    // across moves and are only cleared on ucinewgame.
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(PVTable, 0, sizeof(PVTable));
    memset(PVLength, 0, sizeof(PVLength));
    memset(playedMoveStack, 0, sizeof(playedMoveStack));

    // single-legal-move fast path. if there is only one legal reply, play it
    // immediately without searching. saves clock time in forced positions.
    {
        moves probe[1];
        generateMoves(probe);

        int legalCount = 0;
        int onlyLegal = 0;
        for (int i = 0; i < probe->count; i++)
        {
            int mv = probe->moves[i];
            copyBoard();
            if (makeMove(mv, allMoves))
            {
                undoBoard();
                legalCount++;
                onlyLegal = mv;
                if (legalCount > 1) break;
            }
        }

        if (legalCount == 1)
        {
            printf("info string only one legal move\n");
            printf("bestmove ");
            printMove(onlyLegal);
            printf("\n");
            return;
        }
    }

    int alpha = -infinity;
    int beta = infinity;

    for (int currentDepth = 1; currentDepth <= depth; currentDepth++)
    {
        if (stopped == 1)
            break;

        // iterative deepening time check. if we have used roughly half of
        // softLimit already, do not start a new iteration. iterations
        // typically take roughly twice as long as the previous one, so
        // crossing this threshold means the next iteration is unlikely to
        // finish within stoptime and we would just abort partway through.
        if (timeset && currentDepth > 1 && softLimit > 0 &&
            (getTime() - starttime) * 2 > softLimit)
        {
            break;
        }

        followPV = 1;

        score = negamax(alpha, beta, currentDepth);

        // if search was aborted, do not retry aspiration or print
        // partial info; just bail out so the previous iteration's PV
        // stands as the bestmove.
        if (stopped == 1)
            break;

        // aspiration window fail -> retry full window
        if ((score <= alpha) || (score >= beta))
        {
            alpha = -infinity;
            beta = infinity;
            continue;
        }

        alpha = score - 50;
        beta = score + 50;

        if (PVLength[0])
        {
            if (score > mateScore || score < -mateScore)
                printf("info score mate %d ", (score > 0) ? ((mateValue-score)/2+1) : (-mateValue+(mateValue-score)/2));
            else
                printf("info score cp %d ", score);

            printf("depth %d nodes %lld pv ", currentDepth, nodes);

            for (int count = 0; count < PVLength[0]; count++)
            {
                printMove(PVTable[0][count]);
                printf(" ");
            }
            printf("\n");
        }
    }

    printf("bestmove ");
    printMove(PVTable[0][0]);
    printf("\n");
}
