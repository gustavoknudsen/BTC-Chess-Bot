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

    // insertion sort (descending). adaptive on near-sorted input, which is
    // the common case once history and killers have warmed up. linear in N
    // when the list is already sorted; quadratic worst case.
    for (int i = 1; i < moveList->count; i++)
    {
        int curScore = moveScores[i];
        int curMove  = moveList->moves[i];
        int j = i - 1;
        while (j >= 0 && moveScores[j] < curScore)
        {
            moveScores[j + 1]      = moveScores[j];
            moveList->moves[j + 1] = moveList->moves[j];
            j--;
        }
        moveScores[j + 1]      = curScore;
        moveList->moves[j + 1] = curMove;
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

    // insertion sort (descending) as in sortMoves
    for (int i = 1; i < moveList->count; i++)
    {
        int curScore = moveScores[i];
        int curMove  = moveList->moves[i];
        int j = i - 1;
        while (j >= 0 && moveScores[j] < curScore)
        {
            moveScores[j + 1]      = moveScores[j];
            moveList->moves[j + 1] = moveList->moves[j];
            j--;
        }
        moveScores[j + 1]      = curScore;
        moveList->moves[j + 1] = curMove;
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


// negamax alpha beta search.
//   excludedMove != 0 means we are inside a singular-extension verification:
//   the search runs on the SAME position with one move forbidden in the move
//   loop, and must not write TT, do null move, RFP, razoring, LMP/futility,
//   nor recursively trigger another singular check.
static inline int negamax(int alpha, int beta, int depth, int excludedMove)
{
    PVLength[ply] = ply;

    int score = 0;
    int bestMove = 0;
    int hashFlag = hashFlagAlpha;

    int pvNode = beta - alpha > 1;

    // repetition / fifty-move draws (gated on ply so root always returns a move)
    if (ply && (isRepetition() || fifty >= 100))
        return 0;

    // mate distance pruning
    if (ply)
    {
        if (alpha < -mateValue + ply) alpha = -mateValue + ply;
        if (beta > mateValue - ply - 1) beta = mateValue - ply - 1;
        if (alpha >= beta) return alpha;
    }

    // TT probe. inside a singular verification we still want the entry for
    // move ordering, but we must NOT take a TT cutoff (the stored score
    // includes the move we are trying to exclude).
    int ttHit = 0, ttDepth = 0, ttFlag = 0, ttScore = 0;
    score = probeHash(alpha, beta, depth, &bestMove, &ttHit, &ttDepth, &ttFlag, &ttScore);
    if (excludedMove == 0 && ply && pvNode == 0 && score != noHashEntry)
        return score;

    // remember the TT-best move separately; bestMove gets reassigned in the
    // move loop, but the singular extension always applies to the TT move.
    int ttMove = bestMove;

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

    // reverse futility pruning (skip during singular verification)
	if (excludedMove == 0 && depth < 3 && !pvNode && !inCheck && abs(beta) < mateScore)
	{
		int evalMargin = 120 * depth;
		if (eval - evalMargin >= beta)
			return eval - evalMargin;
	}

    // null move pruning (skip during singular verification)
    if (excludedMove == 0 && depth >= 3 && inCheck == 0 && ply)
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

        score = -negamax(-beta, -beta + 1, depth - 1 - 2, 0);

        ply--;
        repetitionIndex--;

        undoBoard();

        if (stopped == 1)
            return 0;

        if (score >= beta)
            return beta;
    }

    // razoring (Strelka-style) (skip during singular verification)
    if (excludedMove == 0 && !pvNode && !inCheck && depth <= 3)
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

    // Singular extension verification.
    //
    // If the TT move is significantly better than all alternatives at a
    // reduced depth, it is "singular" and gets a 1-ply extension when we
    // actually search it below. The verification runs the SAME position with
    // the TT move excluded, against a null window just below ttScore. If the
    // search fails low (no other move reaches the window), the TT move is
    // singular.
    //
    // Conditions (conservative first pass):
    //   - not root, not inside another singular verification
    //   - depth >= 8
    //   - we have a TT hit with a non-empty best move
    //   - TT depth >= depth - 3 (entry is meaningful at this depth)
    //   - TT flag is lower-bound or exact (the stored score would cut off
    //     at the original depth)
    //   - TT score is not in mate territory (mate scores misbehave in
    //     null-window verification)
    int singularExtension = 0;
    if (excludedMove == 0
        && ply > 0
        && depth >= 8
        && ttHit
        && ttMove != 0
        && ttDepth >= depth - 3
        && (ttFlag == hashFlagBeta || ttFlag == hashFlagExact)
        && abs(ttScore) < mateScore)
    {
        int singularBeta  = ttScore - 2 * depth;
        int singularDepth = (depth - 1) / 2;

        int singScore = negamax(singularBeta - 1, singularBeta, singularDepth, ttMove);

        // verifier reuses this ply's PV/length slots; reset so its scratch
        // PV is not propagated upward
        PVLength[ply] = ply;

        if (stopped == 1)
            return 0;

        if (singScore < singularBeta)
            singularExtension = 1;
    }

    int movesSearched = 0;

    // remember the quiets and captures we tried so we can apply malus on cutoff
    int quietsTried[256];
    int quietsCount = 0;
    int capturesTried[256];
    int capturesCount = 0;

    for (int count = 0; count < moveList->count; count++)
    {
        int move = moveList->moves[count];

        // singular verification skips the TT move on the same position
        if (move == excludedMove)
            continue;

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

        // depth offset for this move: +1 ply if this is the TT-best move and
        // it passed the singular verification above. only the TT move can
        // extend; all other moves search at depth - 1.
        int extension = (move == ttMove) ? singularExtension : 0;
        int newDepth  = depth - 1 + extension;

        // LMP and frontier futility on quiet moves.
        // we only prune at non-pv, non-check nodes once at least one move
        // has been searched (so we have a real alpha baseline), and only on
        // quiet moves that do not give check. mate-territory alpha disables
        // both since we never want to drop a saving sequence in mate space.
        // also disabled during singular verification (must search every
        // non-excluded move to a meaningful depth).
        if (excludedMove == 0
            && movesSearched > 0 && !pvNode && !inCheck && !opponentInCheck
            && !getCapture(move) && !getPromoted(move)
            && abs(alpha) < mateScore)
        {
            // LMP: at shallow depths, drop quiet moves after we have searched
            // enough of them. lmpThreshold grows with depth so we are more
            // permissive deeper in the tree.
            if (depth <= 8 && movesSearched >= 3 + depth * depth)
            {
                ply--;
                repetitionIndex--;
                undoBoard();
                continue;
            }

            // frontier futility: if the static eval is so far below alpha
            // that even a depth-scaled margin cannot bridge the gap, skip
            // this quiet move.
            if (depth <= 6 && eval + 120 * depth <= alpha)
            {
                ply--;
                repetitionIndex--;
                undoBoard();
                continue;
            }
        }

        if (movesSearched == 0)
        {
            score = -negamax(-beta, -alpha, newDepth, 0);
        }
        else
        {
            if (movesSearched >= fullDepthMoves &&
                        depth >= reductionLimit &&
                        inCheck == 0 && pvNode == 0)
            {
                if (getPromoted(move) || getCapture(move))
                {
                    // captures and promos: small static reduction. check-givers
                    // get the lighter (depth - 3) treatment so a tactical line
                    // is not blindly chopped.
                    int reduction = opponentInCheck ? 2 : 3;
                    int adjustedDepth = newDepth - reduction;
                    if (adjustedDepth < 1) adjustedDepth = 1;
                    score = -negamax(-alpha - 1, -alpha, adjustedDepth, 0);
                }
                else
                {
                    // quiet moves: Ethereal-style LMR formula, then nudge the
                    // reduction by history so promising quiet moves are
                    // reduced less and quiet moves with bad history more.
                    double depthAdjustment = 0.7844 + std::log(depth) * std::log(movesSearched) / 2.4696;
                    int reduction = (int)depthAdjustment;

                    // history adjustment. mainHistory + 1-ply continuation
                    // history are each bounded to roughly +- HIST_MAX, so the
                    // combined score is in [-16384, +16384] and divides cleanly
                    // by 4096 to give a +- 4 ply reduction nudge.
                    int histScore = mainHistory[getPiece(move)][getTarget(move)] + getContHist(move);
                    reduction -= histScore / 4096;

                    // do not reduce quiet check-givers as hard
                    if (opponentInCheck)
                        reduction--;

                    if (reduction < 0) reduction = 0;

                    int adjustedDepth = newDepth - reduction;
                    if (adjustedDepth < 1) adjustedDepth = 1;
                    score = -negamax(-alpha - 1, -alpha, adjustedDepth, 0);
                }
            }
            else
            {
                score = alpha + 1;
            }

            // PVS re-search
            if (score > alpha)
            {
                score = -negamax(-alpha - 1, -alpha, newDepth, 0);

                if ((score > alpha) && (score < beta))
                    score = -negamax(-beta, -alpha, newDepth, 0);
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
                // do not pollute TT with the result of a singular verification
                if (excludedMove == 0)
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
        // inside singular verification, "no legal moves" just means every
        // move was the excluded one; that is not mate, return alpha
        if (excludedMove != 0)
            return alpha;

        if (inCheck)
            return -mateValue + ply;
        else
            return 0;
    }

    if (excludedMove == 0)
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

        // aspiration window. for the first few iterations we search with the
        // full window since the score has not stabilised yet. from depth 4
        // we narrow the window around the previous score; on a fail-high or
        // fail-low we double delta and widen the offending bound, retrying
        // at the same depth. once delta exceeds 800 we give up and fall back
        // to the full window so the search always converges.
        int delta = 50;
        if (currentDepth >= 4)
        {
            alpha = score - delta;
            beta  = score + delta;
            if (alpha < -infinity) alpha = -infinity;
            if (beta  >  infinity) beta  =  infinity;
        }
        else
        {
            alpha = -infinity;
            beta  =  infinity;
        }

        while (1)
        {
            score = negamax(alpha, beta, currentDepth, 0);

            // if search was aborted, do not retry aspiration or print
            // partial info; just bail out so the previous iteration's PV
            // stands as the bestmove.
            if (stopped == 1)
                break;

            // accept if score is strictly inside the window
            if (score > alpha && score < beta)
                break;

            // fail-low: widen alpha down
            if (score <= alpha)
            {
                alpha = score - delta;
                if (alpha < -infinity) alpha = -infinity;
            }
            // fail-high: widen beta up
            else
            {
                beta = score + delta;
                if (beta > infinity) beta = infinity;
            }

            delta *= 2;
            if (delta > 800)
            {
                alpha = -infinity;
                beta  =  infinity;
            }
        }

        if (stopped == 1)
            break;

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
