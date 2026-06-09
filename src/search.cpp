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
int16_t continuationHistory2[12][64][12][64];

// pawn-structure history [pawnKey][piece][to]. orders quiet moves by how well
// they did in similar pawn structures; an additive term in quiet move scoring.
#define PAWN_HIST_SIZE 2048
int16_t pawnHistory[PAWN_HIST_SIZE][12][64];
// pawn-history index for the node currently being ordered. set once per node in
// sortMoves so scoreMove does not recompute it per move.
static int curPawnHistIndex;

// low-ply history [ply][from][to]. a near-root quiet-move-ordering table that is
// reset every search (unlike the persistent main history), so it reflects this
// position's near-root cutoffs. only the first few plies are kept.
#define LOW_PLY_SIZE 5
int16_t lowPlyHistory[LOW_PLY_SIZE][64][64];

// counter-move table [prevPiece][prevTo]. carries across moves like history.
int counterMoves[12][64];

// move that led to each ply. playedMoveStack[ply] is the move just made to
// reach the position at search ply == ply. used for continuation history.
int playedMoveStack[maxPly + 1];

// static eval recorded per ply, used by the improving heuristic. EVAL_NONE
// marks plies where we were in check (no usable static eval).
#define EVAL_NONE 100000
int staticEvalStack[maxPly + 1];

// correction history. records the signed gap between a node's static eval and
// the value search actually returned, keyed by coarse features of the position
// (pawn structure, and each side's non-pawn piece placement). a later node that
// shares those features biases its static eval by the learned gap, so the
// pruning and reduction heuristics work off a more accurate eval. these carry
// across moves within a game and reset only on ucinewgame.
#define CORR_SIZE (1 << 14)   // entries per table, power of 2
#define CORR_LIMIT 1024       // per-entry bound; the gravity update saturates here
static int16_t pawnCorrHist[CORR_SIZE][2];        // [pawnKey][sideToMove]
static int16_t nonPawnCorrHist[2][CORR_SIZE][2];  // [pieceColor][nonPawnKey][sideToMove]

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
#define SCORE_COUNTER       600000
#define SCORE_BAD_CAPTURE   (-1000000)

// ProbCut: raised-beta margin in eval units (pawn = 126), shrunk when improving.
#define PROBCUT_MARGIN 179
#define PROBCUT_IMPROVING 46

// reset all search-side heuristics. called at engine init and on ucinewgame.
void clearSearchHeuristics()
{
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(mainHistory, 0, sizeof(mainHistory));
    memset(captureHistory, 0, sizeof(captureHistory));
    memset(continuationHistory, 0, sizeof(continuationHistory));
    memset(continuationHistory2, 0, sizeof(continuationHistory2));
    memset(counterMoves, 0, sizeof(counterMoves));
    memset(playedMoveStack, 0, sizeof(playedMoveStack));
    memset(pawnHistory, 0, sizeof(pawnHistory));
    memset(pawnCorrHist, 0, sizeof(pawnCorrHist));
    memset(nonPawnCorrHist, 0, sizeof(nonPawnCorrHist));
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

// correction-history table indices. the keys mix the relevant piece bitboards
// the same way the pawn-structure cache does; collisions are tolerable because
// the correction is only a heuristic nudge.
static inline int pawnCorrIndex()
{
    U64 k = bitboards[P] * 0x9E3779B97F4A7C15ULL + bitboards[p] * 0xC2B2AE3D27D4EB4FULL;
    return (int)((k >> 32) & (CORR_SIZE - 1));
}

static inline int nonPawnCorrIndex(int color)
{
    U64 k;
    if (color == white)
        k = bitboards[N] * 0x9E3779B97F4A7C15ULL + bitboards[B] * 0xC2B2AE3D27D4EB4FULL
          + bitboards[R] * 0x165667B19E3779F9ULL + bitboards[Q] * 0xD6E8FEB86659FD93ULL
          + bitboards[K] * 0xA0761D6478BD642FULL;
    else
        k = bitboards[n] * 0x9E3779B97F4A7C15ULL + bitboards[b] * 0xC2B2AE3D27D4EB4FULL
          + bitboards[r] * 0x165667B19E3779F9ULL + bitboards[q] * 0xD6E8FEB86659FD93ULL
          + bitboards[k] * 0xA0761D6478BD642FULL;
    return (int)((k >> 32) & (CORR_SIZE - 1));
}

// blended correction for the current side to move, in eval units. the pawn
// table is weighted a touch above each non-pawn table. the divisor caps the
// total correction near a pawn so a bad entry cannot dominate the eval.
#define CORR_WEIGHT_PAWN 6
#define CORR_WEIGHT_NONPAWN 5
#define CORR_NORM 128
static inline int correctionValue()
{
    int pc  = pawnCorrHist[pawnCorrIndex()][side];
    int wnp = nonPawnCorrHist[white][nonPawnCorrIndex(white)][side];
    int bnp = nonPawnCorrHist[black][nonPawnCorrIndex(black)][side];
    return (CORR_WEIGHT_PAWN * pc + CORR_WEIGHT_NONPAWN * (wnp + bnp)) / CORR_NORM;
}

// add the learned correction to a raw static eval, kept clear of mate scores.
static inline int correctStaticEval(int eval)
{
    eval += correctionValue();
    if (eval >  mateScore) eval =  mateScore;
    if (eval < -mateScore) eval = -mateScore;
    return eval;
}

// gravity update on a correction entry, bounded to [-CORR_LIMIT, CORR_LIMIT].
static inline void updateCorrEntry(int16_t *entry, int bonus)
{
    if (bonus >  CORR_LIMIT) bonus =  CORR_LIMIT;
    if (bonus < -CORR_LIMIT) bonus = -CORR_LIMIT;
    int e = *entry;
    e += bonus - e * abs(bonus) / CORR_LIMIT;
    *entry = (int16_t)e;
}

// fold this node's static-eval error into the correction tables. the bonus is
// the signed gap (search result minus corrected static eval) scaled by depth,
// with a slightly larger weight when no move improved alpha (a fail-low carries
// a cleaner upper-bound signal). bounded to a quarter of the entry range.
static inline void updateCorrectionHistory(int corrected, int bestScore, int depth, int hasBestMove)
{
    int bonus = (bestScore - corrected) * depth * (hasBestMove ? 16 : 24) / 128;
    if (bonus >  CORR_LIMIT / 4) bonus =  CORR_LIMIT / 4;
    if (bonus < -CORR_LIMIT / 4) bonus = -CORR_LIMIT / 4;
    updateCorrEntry(&pawnCorrHist[pawnCorrIndex()][side], bonus);
    updateCorrEntry(&nonPawnCorrHist[white][nonPawnCorrIndex(white)][side], bonus);
    updateCorrEntry(&nonPawnCorrHist[black][nonPawnCorrIndex(black)][side], bonus);
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

// 2-ply continuation history lookup. indexed by the move played 2 plies ago
// (our own previous move). returns 0 when there is no such move yet.
static inline int getContHist2(int currentMove)
{
    if (ply < 2) return 0;
    int prev2 = playedMoveStack[ply - 1];
    if (prev2 == 0) return 0;
    return continuationHistory2[getPiece(prev2)][getTarget(prev2)]
                               [getPiece(currentMove)][getTarget(currentMove)];
}

// update 2-ply continuation history for the current move at ply
static inline void updateContHist2(int currentMove, int bonus)
{
    if (ply < 2) return;
    int prev2 = playedMoveStack[ply - 1];
    if (prev2 == 0) return;
    updateHistoryEntry(&continuationHistory2[getPiece(prev2)][getTarget(prev2)]
                                             [getPiece(currentMove)][getTarget(currentMove)],
                       bonus);
}

// counter-move lookup. returns the stored refutation of the move that led to
// this node (opponent's last move), or 0 when none.
static inline int getCounterMove()
{
    if (ply <= 0) return 0;
    int prev = playedMoveStack[ply];
    if (prev == 0) return 0;
    return counterMoves[getPiece(prev)][getTarget(prev)];
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

// pawn-structure history index for the current position. keyed by the pawn
// bitboards the same way the pawn-structure cache is.
static inline int pawnHistIndex()
{
    U64 k = bitboards[P] * 0x9E3779B97F4A7C15ULL + bitboards[p] * 0xC2B2AE3D27D4EB4FULL;
    return (int)((k >> 32) & (PAWN_HIST_SIZE - 1));
}

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

    // counter-move: refutation of the opponent's last move, ordered just below
    // killers and above the history-scored remainder.
    if (getCounterMove() == move)
        return SCORE_COUNTER;

    // history-based scoring for quiet moves
    int quietScore = mainHistory[getPiece(move)][getTarget(move)]
                   + getContHist(move)
                   + getContHist2(move)
                   + pawnHistory[curPawnHistIndex][getPiece(move)][getTarget(move)];

    // near the root, weight the per-search low-ply table heavily, decaying with
    // ply. the 4x base matches its dominance over main history in the reference.
    if (ply < LOW_PLY_SIZE)
        quietScore += 4 * lowPlyHistory[ply][getSource(move)][getTarget(move)] / (1 + ply);

    return quietScore;
}

// sort moves (for better pruning). TT best move is bumped to top.
static inline void sortMoves(moves *moveList, int best)
{
    int moveScores[256];

    // pawn structure is fixed for this node, so resolve its history index once.
    curPawnHistIndex = pawnHistIndex();

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

    int eval = correctStaticEval(evaluate());

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
    int eval = correctStaticEval(evaluate());

    // best score seen across this node's children, used to fold the static-eval
    // error into correction history at node exit. tracked fail-soft (it can sit
    // below alpha) so a fail-low still carries a usable bound.
    int bestScore = -infinity;

    // improving heuristic. compare this node's static eval to our own static
    // eval two plies ago (falling back to four plies). improving = the eval is
    // trending up for us, which lets the pruning heuristics below be more
    // aggressive when it is not. in check there is no usable static eval.
    staticEvalStack[ply] = inCheck ? EVAL_NONE : eval;
    int improving = 0;
    if (!inCheck)
    {
        if (ply >= 2 && staticEvalStack[ply - 2] != EVAL_NONE)
            improving = eval > staticEvalStack[ply - 2];
        else if (ply >= 4 && staticEvalStack[ply - 4] != EVAL_NONE)
            improving = eval > staticEvalStack[ply - 4];
    }

    // reverse futility pruning (skip during singular verification). margin
    // shrinks when improving so a rising eval prunes one depth's worth sooner.
	if (excludedMove == 0 && depth < 3 && !pvNode && !inCheck && abs(beta) < mateScore)
	{
		int evalMargin = 168 * (depth - improving);
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

    // ProbCut. if a good enough capture, verified by a qsearch and then a
    // reduced search, beats a beta raised by a healthy margin, the node almost
    // certainly fails high and we can prune it. skipped during singular
    // verification, in check, at pv nodes, near mate, and when the TT already
    // says this node falls short of the raised beta.
    int probCutBeta = beta + PROBCUT_MARGIN - PROBCUT_IMPROVING * improving;
    if (excludedMove == 0 && !pvNode && !inCheck && depth >= 5 && abs(beta) < mateScore
        && !(ttHit && ttDepth >= depth - 3 && ttScore < probCutBeta))
    {
        // a candidate capture must gain enough material to lift the eval to
        // probCutBeta. the gap is in eval units (pawn 126); SEE is on its own
        // pawn-100 scale, so convert before thresholding.
        int seeThreshold = (probCutBeta - eval) * 100 / 126;

        moves moveList[1];
        generateCaptures(moveList);
        sortCaptures(moveList);

        for (int count = 0; count < moveList->count; count++)
        {
            int move = moveList->moves[count];

            if (!seeGe(move, seeThreshold))
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

            score = -quiescence(-probCutBeta, -probCutBeta + 1);
            if (score >= probCutBeta)
                score = -negamax(-probCutBeta, -probCutBeta + 1, depth - 4, 0);

            ply--;
            repetitionIndex--;
            undoBoard();

            if (stopped == 1)
                return 0;

            if (score >= probCutBeta)
            {
                recordHash(beta, depth - 3, hashFlagBeta, move, move);
                return beta;
            }
        }
    }

    // razoring (Strelka-style) (skip during singular verification)
    if (excludedMove == 0 && !pvNode && !inCheck && depth <= 3)
    {
        score = eval + 192;
        int newScore;

        if (score < beta)
        {
            if (depth == 1)
            {
                newScore = quiescence(alpha, beta);
                return (newScore > score) ? newScore : score;
            }

            score += 269;

            if (score < beta && depth <= 2)
            {
                newScore = quiescence(alpha, beta);
                if (newScore < beta)
                    return (newScore > score) ? newScore : score;
            }
        }
	}

    // Internal iterative reductions. With no TT move at sufficient depth,
    // shrink depth by 1 so this iteration is cheap and populates the TT;
    // the next iteration then searches with proper move ordering. Mutually
    // exclusive with singular extensions, which require a TT move.
    if (excludedMove == 0 && ply > 0 && depth >= 6 && bestMove == 0)
        depth--;

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
            // enough of them. the threshold grows with depth so we are more
            // permissive deeper in the tree, and halves when not improving so
            // a falling eval prunes the late quiets sooner.
            if (depth <= 8 && movesSearched >= (3 + depth * depth) / (2 - improving))
            {
                ply--;
                repetitionIndex--;
                undoBoard();
                continue;
            }

            // frontier futility: if the static eval is so far below alpha
            // that even a depth-scaled margin cannot bridge the gap, skip
            // this quiet move.
            if (depth <= 6 && eval + 184 * depth <= alpha)
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

                    // history adjustment. mainHistory + 1-ply + 2-ply
                    // continuation history, each bounded to roughly +- HIST_MAX.
                    // divisor kept at 4096 (matching Stockfish, which adds more
                    // history sources without rescaling the LMR divisor) so the
                    // larger sum can shift LMR more on strongly-good or
                    // strongly-bad quiet moves.
                    int histScore = mainHistory[getPiece(move)][getTarget(move)]
                                    + getContHist(move)
                                    + getContHist2(move);
                    reduction -= histScore / 4096;

                    // reduce one extra ply when our eval is not improving
                    if (!improving)
                        reduction++;

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

        if (score > bestScore)
            bestScore = score;

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

                    // record this quiet as the counter-move to the opponent's
                    // last move (the move that led to this node).
                    int prevMove = (ply > 0) ? playedMoveStack[ply] : 0;
                    if (prevMove != 0)
                        counterMoves[getPiece(prevMove)][getTarget(prevMove)] = move;

                    // 2-ply bonus scaled 3/4 vs 1-ply, matching Stockfish's
                    // 780/1040 weighting between (ss-1) and (ss-2) entries. the
                    // 2-ply signal is noisier so it accumulates more slowly.
                    int bonus2 = bonus * 3 / 4;

                    // pawn structure is fixed for this node; resolve once. the
                    // pawn-history malus is gentler (half) than the bonus.
                    int phIdx = pawnHistIndex();

                    updateHistoryEntry(&mainHistory[getPiece(move)][getTarget(move)], bonus);
                    updateContHist(move, bonus);
                    updateContHist2(move, bonus2);
                    updateHistoryEntry(&pawnHistory[phIdx][getPiece(move)][getTarget(move)], bonus);
                    if (ply < LOW_PLY_SIZE)
                        updateHistoryEntry(&lowPlyHistory[ply][getSource(move)][getTarget(move)], bonus * 663 / 1024);

                    for (int i = 0; i < quietsCount - 1; i++)
                    {
                        int badMove = quietsTried[i];
                        updateHistoryEntry(&mainHistory[getPiece(badMove)][getTarget(badMove)], -bonus);
                        updateContHist(badMove, -bonus);
                        updateContHist2(badMove, -bonus2);
                        updateHistoryEntry(&pawnHistory[phIdx][getPiece(badMove)][getTarget(badMove)], -bonus / 2);
                        if (ply < LOW_PLY_SIZE)
                            updateHistoryEntry(&lowPlyHistory[ply][getSource(badMove)][getTarget(badMove)], -bonus * 663 / 1024);
                    }
                }

                // a quiet move caused the cutoff: the position was better than
                // the static eval said, so nudge correction history upward.
                if (excludedMove == 0 && !inCheck && !getCapture(move) && bestScore > eval)
                    updateCorrectionHistory(eval, bestScore, depth, 1);

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

    // fold this node's static-eval error into correction history. update only
    // when the error direction is consistent: a raised alpha (real best move)
    // should mean the result beat the static eval, a fail-low should mean it
    // did not. skip captures (noisy) and singular verification.
    if (excludedMove == 0 && !inCheck && bestScore > -infinity
        && !(bestMove && getCapture(bestMove))
        && ((bestScore > eval) == (bestMove != 0)))
        updateCorrectionHistory(eval, bestScore, depth, bestMove != 0);

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
    // across moves and are only cleared on ucinewgame. low-ply history is the
    // exception: it is near-root and reset per search so it is never stale.
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(PVTable, 0, sizeof(PVTable));
    memset(PVLength, 0, sizeof(PVLength));
    memset(playedMoveStack, 0, sizeof(playedMoveStack));
    memset(staticEvalStack, 0, sizeof(staticEvalStack));
    memset(lowPlyHistory, 0, sizeof(lowPlyHistory));

    // bump TT generation so this search's stores are preferred over stale
    // entries from previous searches by the bucket replacement policy.
    ttGeneration++;

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

    // best-move stability time management. when the root best move has been
    // unchanged for several iterations we shrink the soft limit (the position
    // is settled, spend less). when the score just dropped sharply we extend
    // it (something went wrong, keep looking).
    int prevBest = 0;
    int prevScore = 0;
    int stableCount = 0;
    int scoreDrop = 0;

    for (int currentDepth = 1; currentDepth <= depth; currentDepth++)
    {
        if (stopped == 1)
            break;

        // iterative deepening time check. if we have used roughly half of
        // softLimit already, do not start a new iteration. iterations
        // typically take roughly twice as long as the previous one, so
        // crossing this threshold means the next iteration is unlikely to
        // finish within stoptime and we would just abort partway through.
        if (timeset && currentDepth > 1 && softLimit > 0)
        {
            int adjustedSoft = softLimit;
            if (stableCount >= 5)
                adjustedSoft = adjustedSoft * 70 / 100;
            if (scoreDrop)
                adjustedSoft = adjustedSoft * 130 / 100;
            if ((getTime() - starttime) * 2 > adjustedSoft)
                break;
        }

        followPV = 1;

        // aspiration window. for the first few iterations we search with the
        // full window since the score has not stabilised yet. from depth 4
        // we narrow the window around the previous score; on a fail-high or
        // fail-low we double delta and widen the offending bound, retrying
        // at the same depth. once delta exceeds 800 we give up and fall back
        // to the full window so the search always converges.
        int delta = 77;
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
            if (delta > 1230)
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

        // update best-move stability bookkeeping for the next iteration's
        // time decision. a sharp score drop (>= 50cp) flags trouble; an
        // unchanged root best move counts toward stability.
        int curBest = PVTable[0][0];
        scoreDrop = (currentDepth > 1 && score < prevScore - 50);
        if (currentDepth > 1 && curBest == prevBest)
            stableCount++;
        else
            stableCount = 0;
        prevBest  = curBest;
        prevScore = score;
    }

    printf("bestmove ");
    printMove(PVTable[0][0]);
    printf("\n");
}
